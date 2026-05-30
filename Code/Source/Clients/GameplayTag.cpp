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

#include "GameplayTag.h"
#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"

namespace Heathen
{
    GameplayTag::GameplayTag(AZ::u64 id) : m_id(id) {}

    GameplayTag::GameplayTag(const AZStd::string& name)
    {
        m_id = GameplayTagRegistry::ValidateTag(name) ? GameplayTagRegistry::Hash(name) : 0;
    }

    AZ::u64       GameplayTag::GetId()    const { return m_id; }
    bool          GameplayTag::IsValid()  const { return m_id != 0; }

    AZStd::string GameplayTag::GetName() const
    {
        return GameplayTagRegistry::GetName(m_id);
    }

    bool GameplayTag::IsDescendantOf(const GameplayTag& ancestor) const
    {
        return GameplayTagRegistry::IsDescendantOf(m_id, ancestor.m_id);
    }

    bool GameplayTag::IsChildOf(const GameplayTag& parent) const
    {
        return IsDescendantOf(parent);
    }

    bool GameplayTag::IsParentOf(const GameplayTag& child) const
    {
        return GameplayTagRegistry::IsDescendantOf(child.m_id, m_id);
    }

    GameplayTagCollection GameplayTag::GetDescendants() const
    {
        AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(m_id);
        GameplayTagCollection result;
        for (const AZ::u64 id : descendants)
            result.AddTag(GameplayTag(id));
        return result;
    }

    GameplayTag GameplayTag::Make(const AZStd::string& name)     { return GameplayTag(name); }
    GameplayTag GameplayTag::FromName(const AZStd::string& name) { return GameplayTag(name); }
    GameplayTag GameplayTag::Cast(AZ::u64 id)                    { return GameplayTag(id); }

    bool GameplayTag::operator==(const GameplayTag& other) const { return m_id == other.m_id; }
    bool GameplayTag::operator!=(const GameplayTag& other) const { return m_id != other.m_id; }

    void GameplayTag::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GameplayTag>()
                ->Version(1)
                ->Field("Id", &GameplayTag::m_id);
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<GameplayTag>("Gameplay Tag")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constructor<>()
                ->Constructor<AZ::u64>()
                ->Constructor<const AZStd::string&>()
                ->Method("Make", &GameplayTag::Make,
                    {{{ "Tag String", "Dot-delimited tag path e.g. Effects.Buff.Strength" }}},
                    "Creates a GameplayTag by hashing the dot-path name")
                ->Method("From Name", &GameplayTag::FromName,
                    {{{ "Tag String", "Dot-delimited tag path e.g. Effects.Buff.Strength" }}},
                    "Alias for Make — creates a GameplayTag by hashing the dot-path name")
                ->Method("Cast", &GameplayTag::Cast,
                    {{{ "Id", "Pre-computed u64 hash id" }}},
                    "Creates a GameplayTag from a raw u64 hash id")
                ->Method("Get Id", &GameplayTag::GetId,
                    "Returns the raw u64 hash id")
                ->Method("Get Name", &GameplayTag::GetName,
                    "Returns the registered dot-path name, or empty string if unknown")
                ->Method("Is Valid", &GameplayTag::IsValid,
                    "Returns true if this tag has a non-zero id")
                ->Method("Is Descendant Of", &GameplayTag::IsDescendantOf,
                    {{{ "Ancestor", "The ancestor tag to test against" }}},
                    "Returns true if this tag is a descendant of ancestor")
                ->Method("Is Child Of", &GameplayTag::IsChildOf,
                    {{{ "Parent", "The parent tag to test against" }}},
                    "Alias for Is Descendant Of")
                ->Method("Is Parent Of", &GameplayTag::IsParentOf,
                    {{{ "Child", "The child tag to test" }}},
                    "Returns true if this tag is an ancestor of child")
                ->Method("Get Descendants", &GameplayTag::GetDescendants,
                    "Returns a collection containing all registered descendants")
                ->Method("Equal", &GameplayTag::operator==,
                    {{{ "Other", "The tag to compare against" }}})
                ->Method("Not Equal", &GameplayTag::operator!=,
                    {{{ "Other", "The tag to compare against" }}});
        }
    }

} // namespace Heathen
