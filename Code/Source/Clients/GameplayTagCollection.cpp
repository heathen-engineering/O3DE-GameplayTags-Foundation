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

#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"

namespace Heathen
{
    // -------------------------------------------------------------------------
    // Copy / move — do not transfer Changed event subscribers
    // -------------------------------------------------------------------------

    GameplayTagCollection::GameplayTagCollection(const GameplayTagCollection& other)
        : m_tags(other.m_tags)
        // m_changed intentionally default-constructed (empty subscriber list)
    {
    }

    GameplayTagCollection& GameplayTagCollection::operator=(const GameplayTagCollection& other)
    {
        if (this != &other)
            m_tags = other.m_tags;
        // Do NOT copy m_changed — subscribers belong to the original instance
        return *this;
    }

    // -------------------------------------------------------------------------
    // Reflect
    // -------------------------------------------------------------------------

    void GameplayTagCollection::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Enum<GameplayTagArithmetic>()
                ->Value("Set",      GameplayTagArithmetic::Set)
                ->Value("Add",      GameplayTagArithmetic::Add)
                ->Value("Subtract", GameplayTagArithmetic::Subtract)
                ->Value("Multiply", GameplayTagArithmetic::Multiply)
                ->Value("Divide",   GameplayTagArithmetic::Divide)
                ->Value("Min",      GameplayTagArithmetic::Min)
                ->Value("Max",      GameplayTagArithmetic::Max);

            serialize->Class<GameplayTagCollection>()
                ->Version(1)
                ->Field("Tags", &GameplayTagCollection::m_tags);
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            GameplayTagArithmeticReflect(*behavior);

            behavior->Class<GameplayTagCollection>("Gameplay Tag Collection")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Constructor<>()
                // ── Construction ──────────────────────────────────────────────
                ->Method("Make", &GameplayTagCollection::Make,
                    {{{ "Tag Names", "Array of dot-delimited tag name strings" }}},
                    "Creates a new collection pre-populated from an array of tag name strings")
                // ── Mutation ──────────────────────────────────────────────────
                ->Method("Add Tag", &GameplayTagCollection::AddTag,
                    {{{ "Tag", "The tag to add with a value of 1" }}})
                ->Method("Add String Range", &GameplayTagCollection::AddStringRange,
                    {{{ "Tag Names", "Array of dot-delimited tag name strings to add" }}},
                    "Adds each name in the array as a tag with value 1")
                ->Method("Add Tag Range", &GameplayTagCollection::AddTagRange,
                    {{{ "Tags", "Array of GameplayTags to add with value 1" }}},
                    "Adds each tag in the array with value 1")
                ->Method("Add Collection Range", &GameplayTagCollection::AddCollectionRange,
                    {{{ "Other", "Collection whose tags to merge into this collection" }}},
                    "Adds all tags from another collection (each with value 1)")
                ->Method("Remove Tag", &GameplayTagCollection::RemoveTag,
                    {{{ "Tag", "The tag to remove unconditionally" }}})
                ->Method("Clear", &GameplayTagCollection::Clear)
                ->Method("Apply", &GameplayTagCollection::Apply,
                    {{
                        { "Tag",       "The tag to apply the operation to" },
                        { "Operation", "The arithmetic operation to apply",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagArithmetic::Set)) },
                        { "Value",     "The value to use in the operation",
                            behavior->MakeDefaultValue(static_cast<AZ::u64>(1)) }
                    }},
                    "Applies an arithmetic operation to a tag's value")
                // ── Read ─────────────────────────────────────────────────────
                ->Method("Get Value", &GameplayTagCollection::GetValue,
                    {{{ "Tag", "The tag whose stored u64 value to retrieve (0 if absent)" }}},
                    "Returns the raw u64 value stored for the tag, or 0 if absent")
                ->Method("Get Float", &GameplayTagCollection::GetFloat,
                    {{{ "Tag", "The tag to read" }}},
                    "Reads the lower 32 bits of the stored value as a float")
                ->Method("Get Int", &GameplayTagCollection::GetInt,
                    {{{ "Tag", "The tag to read" }}},
                    "Reads the lower 32 bits of the stored value as a signed int")
                ->Method("Get Long", &GameplayTagCollection::GetLong,
                    {{{ "Tag", "The tag to read" }}},
                    "Reads the full 64-bit stored value as a signed long")
                ->Method("Get Double", &GameplayTagCollection::GetDouble,
                    {{{ "Tag", "The tag to read" }}},
                    "Reads the full 64-bit stored value as a double")
                ->Method("Set Float", &GameplayTagCollection::SetFloat,
                    {{{ "Tag", "The tag to write" }, { "Value", "Float value to store" }}})
                ->Method("Set Int", &GameplayTagCollection::SetInt,
                    {{{ "Tag", "The tag to write" }, { "Value", "Int value to store" }}})
                ->Method("Set Long", &GameplayTagCollection::SetLong,
                    {{{ "Tag", "The tag to write" }, { "Value", "Long value to store" }}})
                ->Method("Set Double", &GameplayTagCollection::SetDouble,
                    {{{ "Tag", "The tag to write" }, { "Value", "Double value to store" }}})
                ->Method("Set Tag Value", &GameplayTagCollection::SetTagValue,
                    {{{ "Tag", "The tag to write into" }, { "Tag Path", "Dot-path whose hash is stored as the value" }}},
                    "Stores the hash of tagPath as the value of tag — useful for enum-like patterns")
                ->Method("Get All", &GameplayTagCollection::GetAll,
                    "Returns all (tag, value) pairs currently in the collection")
                // ── Presence ─────────────────────────────────────────────────
                ->Method("Contains", &GameplayTagCollection::Contains,
                    {{
                        { "Tag",         "The tag to check for" },
                        { "Exact Match", "True = exact match only, False = include descendants",
                            behavior->MakeDefaultValue(true) }
                    }},
                    "Returns true if this collection contains the tag")
                ->Method("Contains All", &GameplayTagCollection::ContainsAll,
                    {{{ "Other",       "The collection of tags that must all be present" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Contains Any", &GameplayTagCollection::ContainsAny,
                    {{{ "Other",       "The collection to check at least one tag from" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Contains None", &GameplayTagCollection::ContainsNone,
                    {{{ "Other",       "The collection to check no tags from" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                // ── Query ────────────────────────────────────────────────────
                ->Method("Get Matching Tags", &GameplayTagCollection::GetMatchingTags,
                    {{{ "Tag",         "The tag to match against" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Excluding Tags", &GameplayTagCollection::GetExcludingTags,
                    {{{ "Tag",         "The tag to exclude" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Shared", &GameplayTagCollection::GetShared,
                    {{{ "Other",       "The collection to intersect with" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Exclusive", &GameplayTagCollection::GetExclusive,
                    {{{ "Other",       "The collection to find symmetric difference with" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Descendants", &GameplayTagCollection::GetDescendants,
                    {{{ "Ancestor", "The ancestor tag to get descendants of" }}})
                ->Method("Is Empty", &GameplayTagCollection::IsEmpty)
                ->Method("Count",    &GameplayTagCollection::Count);
        }
    }

    // -------------------------------------------------------------------------
    // Mutation
    // -------------------------------------------------------------------------

    void GameplayTagCollection::AddTag(const GameplayTag& tag)
    {
        Apply(tag, GameplayTagArithmetic::Set, 1);
    }

    void GameplayTagCollection::AddStringRange(const AZStd::vector<AZStd::string>& names)
    {
        for (const AZStd::string& name : names)
            AddTag(GameplayTag(name));
    }

    void GameplayTagCollection::AddTagRange(const AZStd::vector<GameplayTag>& tags)
    {
        for (const GameplayTag& t : tags)
            AddTag(t);
    }

    void GameplayTagCollection::AddCollectionRange(const GameplayTagCollection& other)
    {
        for (const auto& [id, val] : other.m_tags)
            AddTag(GameplayTag(id));
    }

    GameplayTagCollection GameplayTagCollection::Make(const AZStd::vector<AZStd::string>& names)
    {
        GameplayTagCollection collection;
        collection.AddStringRange(names);
        return collection;
    }

    void GameplayTagCollection::RemoveTag(const GameplayTag& tag)
    {
        Apply(tag, GameplayTagArithmetic::Set, 0);
    }

    void GameplayTagCollection::Clear()
    {
        if (!m_tags.empty())
        {
            m_tags.clear();
            m_changed.Signal(*this);
        }
    }

    void GameplayTagCollection::Apply(const GameplayTag& tag, GameplayTagArithmetic operation, AZ::u64 value)
    {
        if (!tag.IsValid())
            return;

        const AZ::u64 id      = tag.GetId();
        const auto    it      = m_tags.find(id);
        const AZ::u64 current = (it != m_tags.end()) ? it->second : 0;
        AZ::u64       result  = 0;

        switch (operation)
        {
        case GameplayTagArithmetic::Set:      result = value;                                   break;
        case GameplayTagArithmetic::Add:      result = current + value;                         break;
        case GameplayTagArithmetic::Subtract: result = (current > value) ? current - value : 0; break;
        case GameplayTagArithmetic::Multiply: result = current * value;                          break;
        case GameplayTagArithmetic::Divide:   result = (value != 0) ? current / value : current; break;
        case GameplayTagArithmetic::Min:      result = (current < value) ? current : value;      break;
        case GameplayTagArithmetic::Max:      result = (current > value) ? current : value;      break;
        default:                              result = current;                                   break;
        }

        if (result == 0)
        {
            if (it != m_tags.end())
                m_tags.erase(it);
            else
                return; // nothing changed
        }
        else
        {
            if (it != m_tags.end() && it->second == result)
                return; // nothing changed
            m_tags[id] = result;
        }

        m_changed.Signal(*this);
    }

    // -------------------------------------------------------------------------
    // Typed value accessors
    // -------------------------------------------------------------------------

    float GameplayTagCollection::GetFloat(const GameplayTag& tag) const
    {
        return UintToFloat(static_cast<AZ::u32>(GetValue(tag) & 0xFFFFFFFFu));
    }

    int GameplayTagCollection::GetInt(const GameplayTag& tag) const
    {
        return static_cast<int>(static_cast<AZ::u32>(GetValue(tag) & 0xFFFFFFFFu));
    }

    AZ::s64 GameplayTagCollection::GetLong(const GameplayTag& tag) const
    {
        return static_cast<AZ::s64>(GetValue(tag));
    }

    double GameplayTagCollection::GetDouble(const GameplayTag& tag) const
    {
        return UlongToDouble(GetValue(tag));
    }

    void GameplayTagCollection::SetFloat(const GameplayTag& tag, float value)
    {
        Apply(tag, GameplayTagArithmetic::Set, static_cast<AZ::u64>(FloatToUint(value)));
    }

    void GameplayTagCollection::SetInt(const GameplayTag& tag, int value)
    {
        Apply(tag, GameplayTagArithmetic::Set, static_cast<AZ::u64>(static_cast<AZ::u32>(value)));
    }

    void GameplayTagCollection::SetLong(const GameplayTag& tag, AZ::s64 value)
    {
        Apply(tag, GameplayTagArithmetic::Set, static_cast<AZ::u64>(value));
    }

    void GameplayTagCollection::SetDouble(const GameplayTag& tag, double value)
    {
        Apply(tag, GameplayTagArithmetic::Set, DoubleToUlong(value));
    }

    void GameplayTagCollection::SetTagValue(const GameplayTag& tag, const AZStd::string& tagPath)
    {
        if (!GameplayTagRegistry::ValidateTag(tagPath))
        {
            AZ_Warning("GameplayTagCollection", false,
                "SetTagValue: '%s' is not a valid tag path — call ignored.", tagPath.c_str());
            return;
        }
        Apply(tag, GameplayTagArithmetic::Set, GameplayTagRegistry::Hash(tagPath));
    }

    // -------------------------------------------------------------------------
    // Enumeration
    // -------------------------------------------------------------------------

    AZStd::vector<AZStd::pair<GameplayTag, AZ::u64>> GameplayTagCollection::GetAll() const
    {
        AZStd::vector<AZStd::pair<GameplayTag, AZ::u64>> result;
        result.reserve(m_tags.size());
        for (const auto& [id, val] : m_tags)
            result.emplace_back(GameplayTag(id), val);
        return result;
    }

    // -------------------------------------------------------------------------
    // Presence checks
    // -------------------------------------------------------------------------

    bool GameplayTagCollection::Contains(const GameplayTag& tag, bool exactMatch) const
    {
        if (!tag.IsValid())
            return false;

        const AZ::u64 id = tag.GetId();
        if (m_tags.find(id) != m_tags.end())
            return true;
        if (exactMatch)
            return false;

        return GameplayTagRegistry::ContainsAnyDescendantOf(m_tags, id);
    }

    bool GameplayTagCollection::ContainsAll(const GameplayTagCollection& other, bool exactMatch) const
    {
        if (other.m_tags.empty())
            return true;
        for (const auto& [id, val] : other.m_tags)
            if (!Contains(GameplayTag(id), exactMatch))
                return false;
        return true;
    }

    bool GameplayTagCollection::ContainsAny(const GameplayTagCollection& other, bool exactMatch) const
    {
        if (other.m_tags.empty())
            return false;
        for (const auto& [id, val] : other.m_tags)
            if (Contains(GameplayTag(id), exactMatch))
                return true;
        return false;
    }

    bool GameplayTagCollection::ContainsNone(const GameplayTagCollection& other, bool exactMatch) const
    {
        if (other.m_tags.empty())
            return true;
        for (const auto& [id, val] : other.m_tags)
            if (Contains(GameplayTag(id), exactMatch))
                return false;
        return true;
    }

    // -------------------------------------------------------------------------
    // Query
    // -------------------------------------------------------------------------

    AZStd::vector<GameplayTag> GameplayTagCollection::GetMatchingTags(const GameplayTag& tag, bool exactMatch) const
    {
        AZStd::vector<GameplayTag> result;
        if (!tag.IsValid())
            return result;

        const AZ::u64 id = tag.GetId();
        if (m_tags.find(id) != m_tags.end())
            result.push_back(tag);

        if (!exactMatch)
        {
            const AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(id);
            for (const AZ::u64 descendantId : descendants)
                if (m_tags.find(descendantId) != m_tags.end())
                    result.push_back(GameplayTag(descendantId));
        }

        return result;
    }

    AZStd::vector<GameplayTag> GameplayTagCollection::GetExcludingTags(const GameplayTag& tag, bool exactMatch) const
    {
        AZStd::vector<GameplayTag> result;

        if (!tag.IsValid())
        {
            for (const auto& [id, val] : m_tags)
                result.push_back(GameplayTag(id));
            return result;
        }

        const AZ::u64 id = tag.GetId();

        if (exactMatch)
        {
            for (const auto& [entryId, val] : m_tags)
                if (entryId != id)
                    result.push_back(GameplayTag(entryId));
            return result;
        }

        AZStd::unordered_set<AZ::u64> exclusionSet = GameplayTagRegistry::GetDescendants(id);
        exclusionSet.insert(id);
        for (const auto& [entryId, val] : m_tags)
            if (exclusionSet.find(entryId) == exclusionSet.end())
                result.push_back(GameplayTag(entryId));
        return result;
    }

    GameplayTagCollection GameplayTagCollection::GetShared(const GameplayTagCollection& other, bool exactMatch) const
    {
        GameplayTagCollection result;
        if (m_tags.empty() || other.m_tags.empty())
            return result;
        for (const auto& [id, val] : m_tags)
        {
            const GameplayTag t(id);
            if (other.Contains(t, exactMatch))
                result.Apply(t, GameplayTagArithmetic::Set, val);
        }
        return result;
    }

    GameplayTagCollection GameplayTagCollection::GetExclusive(const GameplayTagCollection& other, bool exactMatch) const
    {
        GameplayTagCollection result;
        for (const auto& [id, val] : m_tags)
        {
            const GameplayTag t(id);
            if (!other.Contains(t, exactMatch))
                result.Apply(t, GameplayTagArithmetic::Set, val);
        }
        for (const auto& [id, val] : other.m_tags)
        {
            const GameplayTag t(id);
            if (!Contains(t, exactMatch))
                result.Apply(t, GameplayTagArithmetic::Set, val);
        }
        return result;
    }

    AZStd::vector<GameplayTag> GameplayTagCollection::GetDescendants(const GameplayTag& ancestor)
    {
        AZStd::vector<GameplayTag> result;
        if (!ancestor.IsValid())
            return result;
        const AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(ancestor.GetId());
        for (const AZ::u64 id : descendants)
            result.push_back(GameplayTag(id));
        return result;
    }

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    bool    GameplayTagCollection::IsEmpty() const { return m_tags.empty(); }
    AZ::u32 GameplayTagCollection::Count()   const { return static_cast<AZ::u32>(m_tags.size()); }

    AZ::u64 GameplayTagCollection::GetValue(const GameplayTag& tag) const
    {
        if (!tag.IsValid())
            return 0;
        const auto it = m_tags.find(tag.GetId());
        return (it != m_tags.end()) ? it->second : 0;
    }

    bool GameplayTagCollection::ContainsAllDescendants(const GameplayTag& ancestor) const
    {
        if (!ancestor.IsValid())
            return false;
        const AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(ancestor.GetId());
        if (descendants.empty())
            return false;
        for (const AZ::u64 id : descendants)
            if (m_tags.find(id) == m_tags.end())
                return false;
        return true;
    }

} // namespace Heathen
