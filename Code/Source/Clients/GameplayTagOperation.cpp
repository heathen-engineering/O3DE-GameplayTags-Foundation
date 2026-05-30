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

#include "GameplayTagOperation.h"
#include "GameplayTagBitCast.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContext.h>

namespace Heathen
{
    using namespace BitCast;

    // -------------------------------------------------------------------------
    // Set / Get accessors
    // -------------------------------------------------------------------------

    void                  GameplayTagOperation::SetTag(const AZStd::string& name) { tag = GameplayTag(name); }
    GameplayTag           GameplayTagOperation::GetTag() const { return tag; }
    void                  GameplayTagOperation::SetArithmetic(GameplayTagArithmetic op) { arithmetic = op; }
    GameplayTagArithmetic GameplayTagOperation::GetArithmetic() const { return arithmetic; }
    void                  GameplayTagOperation::SetValue(AZ::u64 v) { value = v; }
    AZ::u64               GameplayTagOperation::GetValue() const { return value; }
    void                  GameplayTagOperation::SetValueTag(const AZStd::string& tagName) { valueTag = GameplayTag(tagName); }
    GameplayTag           GameplayTagOperation::GetValueTag() const { return valueTag; }
    void                  GameplayTagOperation::SetValueType(GameplayTagValueType type) { valueType = type; }
    GameplayTagValueType  GameplayTagOperation::GetValueType() const { return valueType; }
    void                  GameplayTagOperation::AddCondition(const GameplayTagCondition& c) { conditions.push_back(c); }
    AZStd::vector<GameplayTagCondition> GameplayTagOperation::GetConditions() const { return conditions; }
    void                  GameplayTagOperation::ClearConditions() { conditions.clear(); }

    // -------------------------------------------------------------------------
    // Factory helpers
    // -------------------------------------------------------------------------

    GameplayTagOperation GameplayTagOperation::MakeSet(const AZStd::string& tagName, AZ::u64 v)
    {
        GameplayTagOperation op;
        op.tag        = GameplayTag(tagName);
        op.arithmetic = GameplayTagArithmetic::Set;
        op.value      = v;
        return op;
    }

    GameplayTagOperation GameplayTagOperation::MakeAdd(const AZStd::string& tagName, AZ::u64 v)
    {
        GameplayTagOperation op;
        op.tag        = GameplayTag(tagName);
        op.arithmetic = GameplayTagArithmetic::Add;
        op.value      = v;
        return op;
    }

    GameplayTagOperation GameplayTagOperation::MakeSubtract(const AZStd::string& tagName, AZ::u64 v)
    {
        GameplayTagOperation op;
        op.tag        = GameplayTag(tagName);
        op.arithmetic = GameplayTagArithmetic::Subtract;
        op.value      = v;
        return op;
    }

    GameplayTagOperation GameplayTagOperation::MakeMultiply(const AZStd::string& tagName, AZ::u64 v)
    {
        GameplayTagOperation op;
        op.tag        = GameplayTag(tagName);
        op.arithmetic = GameplayTagArithmetic::Multiply;
        op.value      = v;
        return op;
    }

    GameplayTagOperation GameplayTagOperation::MakeDivide(const AZStd::string& tagName, AZ::u64 v)
    {
        GameplayTagOperation op;
        op.tag        = GameplayTag(tagName);
        op.arithmetic = GameplayTagArithmetic::Divide;
        op.value      = v;
        return op;
    }

    GameplayTagOperation GameplayTagOperation::MakeMin(const AZStd::string& tagName, AZ::u64 v)
    {
        GameplayTagOperation op;
        op.tag        = GameplayTag(tagName);
        op.arithmetic = GameplayTagArithmetic::Min;
        op.value      = v;
        return op;
    }

    GameplayTagOperation GameplayTagOperation::MakeMax(const AZStd::string& tagName, AZ::u64 v)
    {
        GameplayTagOperation op;
        op.tag        = GameplayTag(tagName);
        op.arithmetic = GameplayTagArithmetic::Max;
        op.value      = v;
        return op;
    }

    // -------------------------------------------------------------------------
    // ShouldApply / Apply
    // -------------------------------------------------------------------------

    bool GameplayTagOperation::ShouldApply(const GameplayTagCollection& collection) const
    {
        return EvaluateConditions(conditions, collection);
    }

    bool GameplayTagOperation::Apply(GameplayTagCollection& collection) const
    {
        if (!ShouldApply(collection))
            return false;

        // Resolve the operand: dynamic from valueTag, or constant.
        const AZ::u64 operandRaw = valueTag.IsValid() ? collection.GetValue(valueTag) : value;

        switch (valueType)
        {
        case GameplayTagValueType::Signed:
        {
            const AZ::s64 current = UlongToLong(collection.GetValue(tag));
            const AZ::s64 operand = UlongToLong(operandRaw);
            AZ::s64 result = current;

            switch (arithmetic)
            {
            case GameplayTagArithmetic::Set:      result = operand;                                  break;
            case GameplayTagArithmetic::Add:      result = current + operand;                        break;
            case GameplayTagArithmetic::Subtract: result = current - operand;                        break;
            case GameplayTagArithmetic::Multiply: result = current * operand;                        break;
            case GameplayTagArithmetic::Divide:   result = (operand != 0) ? current / operand : current; break;
            case GameplayTagArithmetic::Min:      result = (current < operand) ? current : operand;  break;
            case GameplayTagArithmetic::Max:      result = (current > operand) ? current : operand;  break;
            default:                              break;
            }

            collection.Apply(tag, GameplayTagArithmetic::Set, LongToUlong(result));
            break;
        }
        case GameplayTagValueType::Decimal:
        {
            const double current = UlongToDouble(collection.GetValue(tag));
            const double operand = UlongToDouble(operandRaw);
            double result = current;

            switch (arithmetic)
            {
            case GameplayTagArithmetic::Set:      result = operand;                                  break;
            case GameplayTagArithmetic::Add:      result = current + operand;                        break;
            case GameplayTagArithmetic::Subtract: result = current - operand;                        break;
            case GameplayTagArithmetic::Multiply: result = current * operand;                        break;
            case GameplayTagArithmetic::Divide:   result = (operand != 0.0) ? current / operand : current; break;
            case GameplayTagArithmetic::Min:      result = (current < operand) ? current : operand;  break;
            case GameplayTagArithmetic::Max:      result = (current > operand) ? current : operand;  break;
            default:                              break;
            }

            collection.Apply(tag, GameplayTagArithmetic::Set, DoubleToUlong(result));
            break;
        }
        default: // Unsigned or Tag (operandRaw already resolved above)
            collection.Apply(tag, arithmetic, operandRaw);
            break;
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // Reflect
    // -------------------------------------------------------------------------

    void GameplayTagOperation::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GameplayTagOperation>()
                ->Version(2) // bumped: added valueTag and valueType fields
                ->Field("Tag",        &GameplayTagOperation::tag)
                ->Field("Arithmetic", &GameplayTagOperation::arithmetic)
                ->Field("Value",      &GameplayTagOperation::value)
                ->Field("ValueTag",   &GameplayTagOperation::valueTag)
                ->Field("ValueType",  &GameplayTagOperation::valueType)
                ->Field("Conditions", &GameplayTagOperation::conditions);

            if (auto* edit = serialize->GetEditContext())
            {
                edit->Class<GameplayTagOperation>("Gameplay Tag Operation",
                        "Applies an arithmetic operation to a tag in a collection, "
                        "optionally guarded by conditions.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagOperation::tag,
                        "Tag", "The tag to operate on")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagOperation::arithmetic,
                        "Arithmetic", "Set / Add / Subtract / Multiply / Divide / Min / Max")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagOperation::value,
                        "Value", "Constant operand. Set(tag, 0) removes the tag. Ignored when Value Tag is set.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagOperation::valueTag,
                        "Value Tag",
                        "When valid, the operand is resolved from this tag's collection value at apply time.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagOperation::valueType,
                        "Value Type",
                        "Controls how the operand is interpreted (Unsigned / Signed / Decimal).")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagOperation::conditions,
                        "Conditions",
                        "Optional guard conditions. All must be satisfied for the operation to apply. "
                        "Leave empty to apply unconditionally.");
            }
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<GameplayTagOperation>("Gameplay Tag Operation")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Constructor<>()
                // Core
                ->Method("Should Apply", &GameplayTagOperation::ShouldApply,
                    {{{ "Collection", "The collection to test conditions against" }}},
                    "Returns true if all conditions are satisfied (or there are none)")
                ->Method("Apply", &GameplayTagOperation::Apply,
                    {{{ "Collection", "The collection to modify" }}},
                    "Applies the operation if conditions pass. Returns true if applied.")
                // Set / Get
                ->Method("Set Tag", &GameplayTagOperation::SetTag,
                    {{{ "Tag Name", "Dot-delimited tag name e.g. Stats.Health" }}})
                ->Method("Get Tag", &GameplayTagOperation::GetTag)
                ->Method("Set Arithmetic", &GameplayTagOperation::SetArithmetic,
                    {{{ "Op", "Arithmetic operation (Set, Add, Subtract, Multiply, Divide, Min, Max)" }}})
                ->Method("Get Arithmetic", &GameplayTagOperation::GetArithmetic)
                ->Method("Set Value", &GameplayTagOperation::SetValue,
                    {{{ "Value", "Constant operand. Set(tag, 0) removes the tag." }}})
                ->Method("Get Value", &GameplayTagOperation::GetValue)
                ->Method("Set Value Tag", &GameplayTagOperation::SetValueTag,
                    {{{ "Tag Name", "Tag whose collection value is used as operand at apply time" }}})
                ->Method("Get Value Tag", &GameplayTagOperation::GetValueTag)
                ->Method("Set Value Type", &GameplayTagOperation::SetValueType,
                    {{{ "Type", "Unsigned / Signed / Decimal" }}})
                ->Method("Get Value Type", &GameplayTagOperation::GetValueType)
                ->Method("Add Condition", &GameplayTagOperation::AddCondition,
                    {{{ "Condition", "Guard condition to append" }}},
                    "Appends a guard condition; the operation only applies when all conditions pass")
                ->Method("Get Conditions", &GameplayTagOperation::GetConditions)
                ->Method("Clear Conditions", &GameplayTagOperation::ClearConditions,
                    "Removes all guard conditions (operation becomes unconditional)")
                // Factories
                ->Method("Make Set Operation", &GameplayTagOperation::MakeSet,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Value to set (0 removes the tag)",
                            behavior->MakeDefaultValue(static_cast<AZ::u64>(1)) }
                    }},
                    "Creates an operation that sets the tag to Value")
                ->Method("Make Add Operation", &GameplayTagOperation::MakeAdd,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Amount to add",
                            behavior->MakeDefaultValue(static_cast<AZ::u64>(1)) }
                    }},
                    "Creates an operation that adds Value to the tag")
                ->Method("Make Subtract Operation", &GameplayTagOperation::MakeSubtract,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Amount to subtract (unsigned: clamped to 0)",
                            behavior->MakeDefaultValue(static_cast<AZ::u64>(1)) }
                    }},
                    "Creates an operation that subtracts Value from the tag")
                ->Method("Make Multiply Operation", &GameplayTagOperation::MakeMultiply,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Multiplier" }
                    }},
                    "Creates an operation that multiplies the tag value by Value")
                ->Method("Make Divide Operation", &GameplayTagOperation::MakeDivide,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Divisor (divide by 0 leaves the value unchanged)" }
                    }},
                    "Creates an operation that divides the tag value by Value")
                ->Method("Make Min Operation", &GameplayTagOperation::MakeMin,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Cap: tag becomes min(current, Value)" }
                    }},
                    "Creates an operation that clamps the tag value to at most Value")
                ->Method("Make Max Operation", &GameplayTagOperation::MakeMax,
                    {{
                        { "Tag Name", "Dot-delimited tag name" },
                        { "Value",    "Floor: tag becomes max(current, Value)" }
                    }},
                    "Creates an operation that ensures the tag value is at least Value");
        }
    }

} // namespace Heathen
