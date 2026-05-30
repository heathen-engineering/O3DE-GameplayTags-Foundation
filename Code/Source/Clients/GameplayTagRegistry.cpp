/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GameplayTagRegistry.h"
#include <xxHash/xxhash.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Heathen
{
    // Static lookup table — built once; valid chars return true.
    // ASCII only, indices 0-127.
    static const bool s_validTagChars[128] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0-15
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 16-31
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0, // 32-47  (46='.')
        1,1,1,1,1,1,1,1,1,1,              // 48-57  '0'-'9'
        0,0,0,0,0,0,0,                    // 58-64
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 65-77  'A'-'M'
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 78-90  'N'-'Z'
        0,0,0,0,                          // 91-94
        1,                                // 95     '_'
        0,                                // 96
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 97-109  'a'-'m'
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 110-122 'n'-'z'
        0,0,0,0,0                         // 123-127
    };

    AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> GameplayTagRegistry::m_descendants;
    AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> GameplayTagRegistry::m_defaultDescendants;
    AZStd::unordered_map<AZ::u64, AZStd::string>                  GameplayTagRegistry::m_nameMap;
    AZStd::unordered_map<AZ::u64, AZStd::string>                  GameplayTagRegistry::m_defaultNameMap;

    // -------------------------------------------------------------------------

    void GameplayTagRegistry::FireRegistryChanged()
    {
        GameplayTagNotificationBus::Broadcast(&GameplayTagNotifications::OnRegistryChanged);
    }

    // -------------------------------------------------------------------------

    void GameplayTagRegistry::Reflect(AZ::ReflectContext* context)
    {
        if (auto behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // String-based IsRegistered overload (used in SC for user-authored checks)
            using IsRegisteredByString = bool(*)(const AZStd::string&);

            behavior->Class<GameplayTagRegistry>("Gameplay Tag Registry")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Method("Hash", &GameplayTagRegistry::Hash,
                    {{{ "Tag String", "The dot-path tag string to hash" }}},
                    "Returns the u64 xxHash3 of a tag string")
                ->Method("Register From String Data", &GameplayTagRegistry::RegisterFromStringData,
                    {{{ "Tag String", "Newline-delimited list of dot-path tags to register" }}})
                ->Method("Is Registered", static_cast<IsRegisteredByString>(&GameplayTagRegistry::IsRegistered),
                    {{{ "Tag String", "The dot-path tag string to check" }}},
                    "Returns true if the tag string names a registered tag")
                ->Method("Unregister From String Data", &GameplayTagRegistry::UnregisterFromStringData,
                    {{{ "Tag String", "Newline-delimited list of dot-path tags to unregister" }}})
                ->Method("Unregister All", &GameplayTagRegistry::UnregisterAll,
                    "Clears user-registered tags; project-default tags survive")
                ->Method("Get Name", &GameplayTagRegistry::GetName,
                    {{{ "Id", "Pre-hashed tag id (u64)" }}},
                    "Returns the dot-path name for a registered tag id, or empty string if unknown")
                ->Method("Get All Names", &GameplayTagRegistry::GetAllNames,
                    "Returns the dot-path names of all registered tags")
                ->Method("Get All Ids", &GameplayTagRegistry::GetAllIds,
                    "Returns the hashed ids of all registered tags");
        }
    }

    // -------------------------------------------------------------------------
    // Hashing
    // -------------------------------------------------------------------------

    AZ::u64 GameplayTagRegistry::Hash(const AZStd::string& text)
    {
        return XXH3_64bits(text.data(), text.size());
    }

    // -------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------

    void GameplayTagRegistry::RegisterFromStringData(const AZStd::string& tag_string)
    {
        AZStd::vector<AZStd::string> tags = ParseTagString(tag_string);
        if (tags.empty())
            return;

        for (const AZStd::string& tag : tags)
        {
            // Decompose into all prefix nodes.
            // e.g. "Effects.Buff.Strength" → ["Effects", "Effects.Buff", "Effects.Buff.Strength"]
            AZStd::vector<AZStd::string> prefixStrings;
            AZStd::string build;
            size_t start = 0;

            for (size_t i = 0; i <= tag.size(); ++i)
            {
                if (i == tag.size() || tag[i] == '.')
                {
                    if (i > start)
                    {
                        if (!build.empty())
                            build += '.';
                        build += tag.substr(start, i - start);
                        prefixStrings.push_back(build);
                    }
                    start = i + 1;
                }
            }

            for (size_t i = 0; i < prefixStrings.size(); ++i)
            {
                const AZ::u64 prefixHash = Hash(prefixStrings[i]);

                // Ensure the node exists in the map even if it has no descendants (leaf).
                m_descendants.emplace(prefixHash, AZStd::unordered_set<AZ::u64>{});

                // Record name string.
                m_nameMap[prefixHash] = prefixStrings[i];

                // All deeper prefix nodes are descendants of this node.
                for (size_t j = i + 1; j < prefixStrings.size(); ++j)
                    m_descendants[prefixHash].insert(Hash(prefixStrings[j]));
            }
        }

        FireRegistryChanged();
    }

    void GameplayTagRegistry::UnregisterFromStringData(const AZStd::string& tag_string)
    {
        AZStd::vector<AZStd::string> tags = ParseTagString(tag_string);
        if (tags.empty())
            return;

        // Build the full removal set: each input tag plus all its descendants.
        AZStd::unordered_set<AZ::u64> removalSet;
        for (const AZStd::string& tag : tags)
        {
            const AZ::u64 id = Hash(tag);
            removalSet.insert(id);

            auto it = m_descendants.find(id);
            if (it != m_descendants.end())
                for (const AZ::u64 descendant : it->second)
                    removalSet.insert(descendant);
        }

        // Remove entries from m_descendants and m_nameMap.
        for (const AZ::u64 id : removalSet)
        {
            m_descendants.erase(id);
            m_nameMap.erase(id);
        }

        // Scrub removal set from remaining ancestor sets.
        AZStd::vector<AZ::u64> toRemove;
        for (auto& [ancestorId, descendantSet] : m_descendants)
        {
            for (const AZ::u64 removed : removalSet)
                descendantSet.erase(removed);
            if (descendantSet.empty())
                toRemove.push_back(ancestorId);
        }
        for (const AZ::u64 id : toRemove)
            m_descendants.erase(id);

        FireRegistryChanged();
    }

    void GameplayTagRegistry::UnregisterAll()
    {
        // Restore to project defaults; user-registered tags are discarded.
        m_descendants = m_defaultDescendants;
        m_nameMap     = m_defaultNameMap;
        FireRegistryChanged();
    }

    void GameplayTagRegistry::MergeDefaultTags(
        const AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>>& defaults)
    {
        for (const auto& [parent, children] : defaults)
        {
            auto& destDefault = m_defaultDescendants[parent];
            auto& destLive    = m_descendants[parent];
            for (AZ::u64 child : children)
            {
                destDefault.insert(child);
                destLive.insert(child);
            }
        }
        // Note: MergeDefaultTags only receives a descendants map, not name strings.
        // Name strings are registered separately via RegisterFromStringData when the
        // editor system component activates and loads the .gptags source files.
        FireRegistryChanged();
    }

    // -------------------------------------------------------------------------
    // Presence / validation
    // -------------------------------------------------------------------------

    bool GameplayTagRegistry::IsRegistered(const AZStd::string& tag_string)
    {
        if (!ValidateTag(tag_string))
            return false;
        return m_descendants.find(Hash(tag_string)) != m_descendants.end();
    }

    bool GameplayTagRegistry::IsRegistered(AZ::u64 id)
    {
        return m_descendants.find(id) != m_descendants.end();
    }

    bool GameplayTagRegistry::ValidateTag(const AZStd::string& tag_string)
    {
        if (tag_string.empty())
            return false;
        if (tag_string.front() == '.' || tag_string.back() == '.')
            return false;
        bool last_was_dot = false;
        for (char c : tag_string)
        {
            if (static_cast<unsigned char>(c) >= 128)
                return false;
            if (!s_validTagChars[static_cast<int>(c)])
                return false;
            const bool is_dot = (c == '.');
            if (is_dot && last_was_dot)
                return false;
            last_was_dot = is_dot;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    // Name lookup
    // -------------------------------------------------------------------------

    AZStd::string GameplayTagRegistry::GetName(AZ::u64 id)
    {
        auto it = m_nameMap.find(id);
        return (it != m_nameMap.end()) ? it->second : AZStd::string{};
    }

    AZStd::vector<AZStd::string> GameplayTagRegistry::GetAllNames()
    {
        AZStd::vector<AZStd::string> result;
        result.reserve(m_nameMap.size());
        for (const auto& [id, name] : m_nameMap)
            result.push_back(name);
        return result;
    }

    AZStd::vector<AZ::u64> GameplayTagRegistry::GetAllIds()
    {
        AZStd::vector<AZ::u64> result;
        result.reserve(m_nameMap.size());
        for (const auto& [id, name] : m_nameMap)
            result.push_back(id);
        return result;
    }

    // -------------------------------------------------------------------------
    // Hierarchy queries
    // -------------------------------------------------------------------------

    bool GameplayTagRegistry::IsDescendantOf(AZ::u64 tag, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
            return false;
        return it->second.find(tag) != it->second.end();
    }

    bool GameplayTagRegistry::ContainsAnyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
            return false;
        const auto& descendants = it->second;
        if (collection.size() < descendants.size())
        {
            for (const auto& [id, val] : collection)
                if (descendants.find(id) != descendants.end())
                    return true;
        }
        else
        {
            for (const AZ::u64 descendant : descendants)
                if (collection.find(descendant) != collection.end())
                    return true;
        }
        return false;
    }

    bool GameplayTagRegistry::ContainsOnlyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
            return false;
        const auto& descendants = it->second;
        if (collection.size() > descendants.size())
            return false;
        for (const auto& [id, val] : collection)
            if (descendants.find(id) == descendants.end())
                return false;
        return true;
    }

    bool GameplayTagRegistry::ContainsAllDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
            return false;
        const auto& descendants = it->second;
        if (collection.size() < descendants.size())
            return false;
        for (const AZ::u64 descendant : descendants)
            if (collection.find(descendant) == collection.end())
                return false;
        return true;
    }

    AZStd::unordered_set<AZ::u64> GameplayTagRegistry::GetDescendants(AZ::u64 tag)
    {
        auto it = m_descendants.find(tag);
        return (it != m_descendants.end()) ? it->second : AZStd::unordered_set<AZ::u64>{};
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    AZStd::vector<AZStd::string> GameplayTagRegistry::ParseTagString(const AZStd::string& tag_string)
    {
        AZStd::vector<AZStd::string> result;
        AZStd::string current;

        for (char c : tag_string)
        {
            if (c == '\n' || c == '\r')
            {
                if (!current.empty())
                {
                    if (ValidateTag(current))
                        result.push_back(current);
                    else
                        AZ_Warning("GameplayTagRegistry", false, "Invalid tag rejected: %s", current.c_str());
                    current.clear();
                }
            }
            else
            {
                current.push_back(c);
            }
        }

        if (!current.empty())
        {
            if (ValidateTag(current))
                result.push_back(current);
            else
                AZ_Warning("GameplayTagRegistry", false, "Invalid tag rejected: %s", current.c_str());
        }

        return result;
    }

} // namespace Heathen
