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

#include "GameplayTagCondition.h"
#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"
#include "GameplayTagBitCast.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContext.h>

namespace Heathen
{
    using namespace BitCast;

    // -------------------------------------------------------------------------
    // Evaluate
    // -------------------------------------------------------------------------

    bool GameplayTagCondition::Evaluate(const GameplayTagCollection& collection) const
    {
        // ── Tag-identity ops: treat tag's stored value as a tag id ─────────────
        if (comparison == GameplayTagComparisonOp::IsMemberOf ||
            comparison == GameplayTagComparisonOp::IsParentOf  ||
            comparison == GameplayTagComparisonOp::IsExactly)
        {
            if (!compareTag.IsValid())
                return false;

            const AZ::u64 lhsId = collection.GetValue(tag); // stored value treated as tag id
            if (lhsId == 0)
                return false;

            switch (comparison)
            {
            case GameplayTagComparisonOp::IsMemberOf:
                return GameplayTagRegistry::IsDescendantOf(lhsId, compareTag.GetId());
            case GameplayTagComparisonOp::IsParentOf:
                return GameplayTagRegistry::IsDescendantOf(compareTag.GetId(), lhsId);
            case GameplayTagComparisonOp::IsExactly:
                return lhsId == compareTag.GetId();
            default:
                return false;
            }
        }

        // ── Presence checks ────────────────────────────────────────────────────
        if (comparison == GameplayTagComparisonOp::Exists)
            return collection.Contains(tag, exactMatch);
        if (comparison == GameplayTagComparisonOp::NotExists)
            return !collection.Contains(tag, exactMatch);

        // ── Numeric comparisons ────────────────────────────────────────────────
        // RHS: if compareTag is valid, read its value from the collection dynamically.
        const AZ::u64 lhsRaw = collection.GetValue(tag);
        const AZ::u64 rhsRaw = compareTag.IsValid() ? collection.GetValue(compareTag) : compareValue;

        switch (compareValueType)
        {
        case GameplayTagValueType::Signed:
        {
            const AZ::s64 lhs = UlongToLong(lhsRaw);
            const AZ::s64 rhs = UlongToLong(rhsRaw);
            switch (comparison)
            {
            case GameplayTagComparisonOp::Equal:        return lhs == rhs;
            case GameplayTagComparisonOp::NotEqual:     return lhs != rhs;
            case GameplayTagComparisonOp::Less:         return lhs <  rhs;
            case GameplayTagComparisonOp::LessEqual:    return lhs <= rhs;
            case GameplayTagComparisonOp::Greater:      return lhs >  rhs;
            case GameplayTagComparisonOp::GreaterEqual: return lhs >= rhs;
            default:                                    return false;
            }
        }
        case GameplayTagValueType::Decimal:
        {
            const double lhs = UlongToDouble(lhsRaw);
            const double rhs = UlongToDouble(rhsRaw);
            switch (comparison)
            {
            case GameplayTagComparisonOp::Equal:        return lhs == rhs;
            case GameplayTagComparisonOp::NotEqual:     return lhs != rhs;
            case GameplayTagComparisonOp::Less:         return lhs <  rhs;
            case GameplayTagComparisonOp::LessEqual:    return lhs <= rhs;
            case GameplayTagComparisonOp::Greater:      return lhs >  rhs;
            case GameplayTagComparisonOp::GreaterEqual: return lhs >= rhs;
            default:                                    return false;
            }
        }
        default: // Unsigned or Tag (compareTag already baked into rhsRaw above)
            switch (comparison)
            {
            case GameplayTagComparisonOp::Equal:        return lhsRaw == rhsRaw;
            case GameplayTagComparisonOp::NotEqual:     return lhsRaw != rhsRaw;
            case GameplayTagComparisonOp::Less:         return lhsRaw <  rhsRaw;
            case GameplayTagComparisonOp::LessEqual:    return lhsRaw <= rhsRaw;
            case GameplayTagComparisonOp::Greater:      return lhsRaw >  rhsRaw;
            case GameplayTagComparisonOp::GreaterEqual: return lhsRaw >= rhsRaw;
            default:                                    return false;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Set / Get accessors
    // -------------------------------------------------------------------------

    void GameplayTagCondition::SetTag(const AZStd::string& name) { tag = GameplayTag(name); }
    GameplayTag GameplayTagCondition::GetTag() const { return tag; }

    void GameplayTagCondition::SetComparison(GameplayTagComparisonOp op) { comparison = op; }
    GameplayTagComparisonOp GameplayTagCondition::GetComparison() const { return comparison; }

    void GameplayTagCondition::SetCompareValue(AZ::u64 value) { compareValue = value; }
    AZ::u64 GameplayTagCondition::GetCompareValue() const { return compareValue; }

    void GameplayTagCondition::SetCompareTag(const AZStd::string& tagName) { compareTag = GameplayTag(tagName); }
    GameplayTag GameplayTagCondition::GetCompareTag() const { return compareTag; }

    void GameplayTagCondition::SetExactMatch(bool em) { exactMatch = em; }
    bool GameplayTagCondition::GetExactMatch() const { return exactMatch; }

    void GameplayTagCondition::SetLogicOp(GameplayTagLogicOp op) { logicOp = op; }
    GameplayTagLogicOp GameplayTagCondition::GetLogicOp() const { return logicOp; }

    void GameplayTagCondition::SetCompareValueType(GameplayTagValueType type) { compareValueType = type; }
    GameplayTagValueType GameplayTagCondition::GetCompareValueType() const { return compareValueType; }

    // -------------------------------------------------------------------------
    // Factory helpers
    // -------------------------------------------------------------------------

    GameplayTagCondition GameplayTagCondition::MakeExists(
        const AZStd::string& tagName, bool em, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag        = GameplayTag(tagName);
        c.comparison = GameplayTagComparisonOp::Exists;
        c.exactMatch = em;
        c.logicOp    = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeNotExists(
        const AZStd::string& tagName, bool em, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag        = GameplayTag(tagName);
        c.comparison = GameplayTagComparisonOp::NotExists;
        c.exactMatch = em;
        c.logicOp    = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeEqual(
        const AZStd::string& tagName, AZ::u64 cmpValue, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag          = GameplayTag(tagName);
        c.comparison   = GameplayTagComparisonOp::Equal;
        c.compareValue = cmpValue;
        c.logicOp      = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeNotEqual(
        const AZStd::string& tagName, AZ::u64 cmpValue, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag          = GameplayTag(tagName);
        c.comparison   = GameplayTagComparisonOp::NotEqual;
        c.compareValue = cmpValue;
        c.logicOp      = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeLess(
        const AZStd::string& tagName, AZ::u64 cmpValue, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag          = GameplayTag(tagName);
        c.comparison   = GameplayTagComparisonOp::Less;
        c.compareValue = cmpValue;
        c.logicOp      = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeLessEqual(
        const AZStd::string& tagName, AZ::u64 cmpValue, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag          = GameplayTag(tagName);
        c.comparison   = GameplayTagComparisonOp::LessEqual;
        c.compareValue = cmpValue;
        c.logicOp      = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeGreater(
        const AZStd::string& tagName, AZ::u64 cmpValue, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag          = GameplayTag(tagName);
        c.comparison   = GameplayTagComparisonOp::Greater;
        c.compareValue = cmpValue;
        c.logicOp      = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeGreaterEqual(
        const AZStd::string& tagName, AZ::u64 cmpValue, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag          = GameplayTag(tagName);
        c.comparison   = GameplayTagComparisonOp::GreaterEqual;
        c.compareValue = cmpValue;
        c.logicOp      = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeIsMemberOf(
        const AZStd::string& tagName, const AZStd::string& compareTagName, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag        = GameplayTag(tagName);
        c.comparison = GameplayTagComparisonOp::IsMemberOf;
        c.compareTag = GameplayTag(compareTagName);
        c.logicOp    = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeIsParentOf(
        const AZStd::string& tagName, const AZStd::string& compareTagName, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag        = GameplayTag(tagName);
        c.comparison = GameplayTagComparisonOp::IsParentOf;
        c.compareTag = GameplayTag(compareTagName);
        c.logicOp    = op;
        return c;
    }

    GameplayTagCondition GameplayTagCondition::MakeIsExactly(
        const AZStd::string& tagName, const AZStd::string& compareTagName, GameplayTagLogicOp op)
    {
        GameplayTagCondition c;
        c.tag        = GameplayTag(tagName);
        c.comparison = GameplayTagComparisonOp::IsExactly;
        c.compareTag = GameplayTag(compareTagName);
        c.logicOp    = op;
        return c;
    }

    // -------------------------------------------------------------------------
    // Reflect
    // -------------------------------------------------------------------------

    void GameplayTagCondition::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Enum<GameplayTagValueType>()
                ->Value("Unsigned", GameplayTagValueType::Unsigned)
                ->Value("Signed",   GameplayTagValueType::Signed)
                ->Value("Decimal",  GameplayTagValueType::Decimal)
                ->Value("Tag",      GameplayTagValueType::Tag);

            serialize->Enum<GameplayTagComparisonOp>()
                ->Value("Exists",        GameplayTagComparisonOp::Exists)
                ->Value("Not Exists",    GameplayTagComparisonOp::NotExists)
                ->Value("Equal",         GameplayTagComparisonOp::Equal)
                ->Value("Not Equal",     GameplayTagComparisonOp::NotEqual)
                ->Value("Less",          GameplayTagComparisonOp::Less)
                ->Value("Less Equal",    GameplayTagComparisonOp::LessEqual)
                ->Value("Greater",       GameplayTagComparisonOp::Greater)
                ->Value("Greater Equal", GameplayTagComparisonOp::GreaterEqual)
                ->Value("Is Member Of",  GameplayTagComparisonOp::IsMemberOf)
                ->Value("Is Parent Of",  GameplayTagComparisonOp::IsParentOf)
                ->Value("Is Exactly",    GameplayTagComparisonOp::IsExactly);

            serialize->Enum<GameplayTagLogicOp>()
                ->Value("And", GameplayTagLogicOp::And)
                ->Value("Or",  GameplayTagLogicOp::Or)
                ->Value("Xor", GameplayTagLogicOp::Xor);

            serialize->Class<GameplayTagCondition>()
                ->Version(2) // bumped: added compareTag and compareValueType fields
                ->Field("Tag",              &GameplayTagCondition::tag)
                ->Field("Comparison",       &GameplayTagCondition::comparison)
                ->Field("CompareValue",     &GameplayTagCondition::compareValue)
                ->Field("CompareTag",       &GameplayTagCondition::compareTag)
                ->Field("ExactMatch",       &GameplayTagCondition::exactMatch)
                ->Field("LogicOp",          &GameplayTagCondition::logicOp)
                ->Field("CompareValueType", &GameplayTagCondition::compareValueType);

            if (auto* edit = serialize->GetEditContext())
            {
                edit->Enum<GameplayTagValueType>("Gameplay Tag Value Type",
                        "Controls how the stored u64 is interpreted for numeric comparisons")
                    ->Value("Unsigned (u64)", GameplayTagValueType::Unsigned)
                    ->Value("Signed (s64)",   GameplayTagValueType::Signed)
                    ->Value("Decimal (f64)",  GameplayTagValueType::Decimal)
                    ->Value("Tag (dynamic)",  GameplayTagValueType::Tag);

                edit->Enum<GameplayTagComparisonOp>("Gameplay Tag Comparison Op",
                        "How the tag's stored value is compared")
                    ->Value("Exists",         GameplayTagComparisonOp::Exists)
                    ->Value("Not Exists",     GameplayTagComparisonOp::NotExists)
                    ->Value("==",             GameplayTagComparisonOp::Equal)
                    ->Value("!=",             GameplayTagComparisonOp::NotEqual)
                    ->Value("<",              GameplayTagComparisonOp::Less)
                    ->Value("<=",             GameplayTagComparisonOp::LessEqual)
                    ->Value(">",              GameplayTagComparisonOp::Greater)
                    ->Value(">=",             GameplayTagComparisonOp::GreaterEqual)
                    ->Value("Is Member Of",   GameplayTagComparisonOp::IsMemberOf)
                    ->Value("Is Parent Of",   GameplayTagComparisonOp::IsParentOf)
                    ->Value("Is Exactly",     GameplayTagComparisonOp::IsExactly);

                edit->Enum<GameplayTagLogicOp>("Gameplay Tag Logic Op",
                        "How this condition chains to the next")
                    ->Value("AND", GameplayTagLogicOp::And)
                    ->Value("OR",  GameplayTagLogicOp::Or)
                    ->Value("XOR", GameplayTagLogicOp::Xor);

                edit->Class<GameplayTagCondition>("Gameplay Tag Condition",
                        "Evaluates one tag in a collection. Chain multiple conditions using logicOp.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagCondition::tag,
                        "Tag", "The tag to test")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagCondition::comparison,
                        "Comparison", "How to compare the tag value")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagCondition::compareValue,
                        "Compare Value",
                        "RHS for numeric comparisons. Ignored when Compare Tag is set or when using tag-identity ops.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagCondition::compareTag,
                        "Compare Tag",
                        "RHS tag for tag-identity ops (IsMemberOf/IsParentOf/IsExactly). "
                        "When valid and using a numeric op, the tag's collection value is used as the RHS instead.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagCondition::exactMatch,
                        "Exact Match",
                        "For Exists/Not Exists: true = exact tag only; false = also match descendants.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagCondition::logicOp,
                        "Logic Op",
                        "How this condition chains to the NEXT condition (ignored on the last element).")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagCondition::compareValueType,
                        "Value Type",
                        "Controls how the stored u64 is interpreted for numeric comparisons (Unsigned/Signed/Decimal/Tag).");
            }
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            GameplayTagValueTypeReflect(*behavior);
            GameplayTagComparisonOpReflect(*behavior);
            GameplayTagLogicOpReflect(*behavior);

            behavior->Class<GameplayTagCondition>("Gameplay Tag Condition")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Constructor<>()
                ->Method("Evaluate", &GameplayTagCondition::Evaluate,
                    {{{ "Collection", "The tag collection to test against" }}},
                    "Returns true if this condition is satisfied")
                // Set / Get
                ->Method("Set Tag", &GameplayTagCondition::SetTag,
                    {{{ "Tag Name", "Dot-delimited tag name e.g. Status.Burning" }}})
                ->Method("Get Tag", &GameplayTagCondition::GetTag)
                ->Method("Set Comparison", &GameplayTagCondition::SetComparison,
                    {{{ "Op", "Comparison operator" }}})
                ->Method("Get Comparison", &GameplayTagCondition::GetComparison)
                ->Method("Set Compare Value", &GameplayTagCondition::SetCompareValue,
                    {{{ "Value", "Constant RHS value for numeric comparisons" }}})
                ->Method("Get Compare Value", &GameplayTagCondition::GetCompareValue)
                ->Method("Set Compare Tag", &GameplayTagCondition::SetCompareTag,
                    {{{ "Tag Name", "Tag whose collection value is used as RHS, or the reference tag for identity ops" }}})
                ->Method("Get Compare Tag", &GameplayTagCondition::GetCompareTag)
                ->Method("Set Exact Match", &GameplayTagCondition::SetExactMatch,
                    {{{ "Exact Match", "True = exact tag only; false = also match descendants" }}})
                ->Method("Get Exact Match", &GameplayTagCondition::GetExactMatch)
                ->Method("Set Logic Op", &GameplayTagCondition::SetLogicOp,
                    {{{ "Logic Op", "AND / OR / XOR" }}})
                ->Method("Get Logic Op", &GameplayTagCondition::GetLogicOp)
                ->Method("Set Value Type", &GameplayTagCondition::SetCompareValueType,
                    {{{ "Type", "Unsigned / Signed / Decimal / Tag" }}})
                ->Method("Get Value Type", &GameplayTagCondition::GetCompareValueType)
                // Factories
                ->Method("Make Exists Condition", &GameplayTagCondition::MakeExists,
                    {{
                        { "Tag Name",    "Dot-delimited tag name" },
                        { "Exact Match", "True = exact; false = also match descendants",
                            behavior->MakeDefaultValue(true) },
                        { "Logic Op",    "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag exists")
                ->Method("Make Not Exists Condition", &GameplayTagCondition::MakeNotExists,
                    {{
                        { "Tag Name",    "Dot-delimited tag name" },
                        { "Exact Match", "True = exact; false = also match descendants",
                            behavior->MakeDefaultValue(true) },
                        { "Logic Op",    "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag is absent")
                ->Method("Make Equal Condition", &GameplayTagCondition::MakeEqual,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Value the tag must equal" },
                        { "Logic Op", "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag value == Value")
                ->Method("Make Not Equal Condition", &GameplayTagCondition::MakeNotEqual,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Value the tag must not equal" },
                        { "Logic Op", "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag value != Value")
                ->Method("Make Less Than Condition", &GameplayTagCondition::MakeLess,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Upper bound (exclusive)" },
                        { "Logic Op", "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag value < Value")
                ->Method("Make Less Or Equal Condition", &GameplayTagCondition::MakeLessEqual,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Upper bound (inclusive)" },
                        { "Logic Op", "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag value <= Value")
                ->Method("Make Greater Than Condition", &GameplayTagCondition::MakeGreater,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Lower bound (exclusive)" },
                        { "Logic Op", "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag value > Value")
                ->Method("Make Greater Or Equal Condition", &GameplayTagCondition::MakeGreaterEqual,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Lower bound (inclusive)" },
                        { "Logic Op", "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag value >= Value")
                ->Method("Make Is Member Of Condition", &GameplayTagCondition::MakeIsMemberOf,
                    {{
                        { "Tag Name",         "Tag whose stored value (as tag id) is tested" },
                        { "Compare Tag Name", "The ancestor tag to test membership against" },
                        { "Logic Op",         "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag's stored id is a descendant of compareTag")
                ->Method("Make Is Parent Of Condition", &GameplayTagCondition::MakeIsParentOf,
                    {{
                        { "Tag Name",         "Tag whose stored value (as tag id) is tested" },
                        { "Compare Tag Name", "The child tag to test parenthood against" },
                        { "Logic Op",         "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag's stored id is an ancestor of compareTag")
                ->Method("Make Is Exactly Condition", &GameplayTagCondition::MakeIsExactly,
                    {{
                        { "Tag Name",         "Tag whose stored value (as tag id) is tested" },
                        { "Compare Tag Name", "The exact tag id to match" },
                        { "Logic Op",         "How this condition chains to the next",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagLogicOp::And)) }
                    }},
                    "Creates a condition that passes when the tag's stored id equals compareTag exactly");
        }
    }

    // -------------------------------------------------------------------------
    // EvaluateConditions — C-style precedence: AND > OR > XOR
    // -------------------------------------------------------------------------

    bool EvaluateConditions(
        const AZStd::vector<GameplayTagCondition>& conditions,
        const GameplayTagCollection& collection)
    {
        if (conditions.empty())
            return true;

        const size_t n = conditions.size();

        // Step 1: evaluate each condition.
        AZStd::vector<bool> vals(n);
        for (size_t i = 0; i < n; ++i)
            vals[i] = conditions[i].Evaluate(collection);

        // Step 2: AND pass — fold AND chains, collect OR/XOR separators.
        AZStd::vector<bool>              andTokens;
        AZStd::vector<GameplayTagLogicOp> andOps;
        {
            bool acc = vals[0];
            for (size_t i = 0; i < n - 1; ++i)
            {
                if (conditions[i].logicOp == GameplayTagLogicOp::And)
                    acc = acc && vals[i + 1];
                else
                {
                    andTokens.push_back(acc);
                    andOps.push_back(conditions[i].logicOp);
                    acc = vals[i + 1];
                }
            }
            andTokens.push_back(acc);
        }

        // Step 3: OR pass.
        AZStd::vector<bool> orTokens;
        {
            bool acc = andTokens[0];
            for (size_t i = 0; i < andOps.size(); ++i)
            {
                if (andOps[i] == GameplayTagLogicOp::Or)
                    acc = acc || andTokens[i + 1];
                else
                {
                    orTokens.push_back(acc);
                    acc = andTokens[i + 1];
                }
            }
            orTokens.push_back(acc);
        }

        // Step 4: XOR pass.
        bool result = orTokens[0];
        for (size_t i = 1; i < orTokens.size(); ++i)
            result = result ^ orTokens[i];
        return result;
    }

} // namespace Heathen
