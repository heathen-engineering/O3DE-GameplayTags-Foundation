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

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include "GameplayTag.h"

namespace Heathen
{
    class GameplayTagCollection;

    // ---------------------------------------------------------------------------
    // GameplayTagValueType
    // Controls how the stored u64 value is interpreted when performing numeric
    // comparisons in a GameplayTagCondition or when applying typed arithmetic
    // in a GameplayTagOperation.
    // ---------------------------------------------------------------------------

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(GameplayTagValueType, AZ::u8,
        Unsigned, ///< u64, stored and compared directly (default)
        Signed,   ///< s64, stored as two's-complement bits in u64
        Decimal,  ///< double, stored as IEEE 754 bits in u64
        Tag       ///< compareTag's stored value is used as the RHS at evaluation time
    );

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(GameplayTagValueType)
    AZ_TYPE_INFO_SPECIALIZE(Heathen::GameplayTagValueType, "{B1C2D3E4-F5A6-7B8C-9D0E-1A2B3C4D5E6F}");

    // ---------------------------------------------------------------------------
    // GameplayTagComparisonOp
    // ---------------------------------------------------------------------------

    /// How the tag's stored value is compared in a GameplayTagCondition.
    /// Exists / NotExists treat the tag as a boolean flag.
    /// Numeric operators compare the stored value against compareValue
    /// (interpreted according to compareValueType).
    /// Tag-identity operators treat the stored value as a tag id for hierarchy checks.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(GameplayTagComparisonOp, AZ::u8,
        Exists,       ///< tag is present (value != 0); exactMatch applies
        NotExists,    ///< tag is absent  (value == 0); exactMatch applies
        Equal,        ///< stored value == compareValue
        NotEqual,     ///< stored value != compareValue
        Less,         ///< stored value <  compareValue
        LessEqual,    ///< stored value <= compareValue
        Greater,      ///< stored value >  compareValue
        GreaterEqual, ///< stored value >= compareValue
        IsMemberOf,   ///< stored value (as tag id) is a descendant of compareTag
        IsParentOf,   ///< stored value (as tag id) is an ancestor of compareTag
        IsExactly     ///< stored value (as tag id) equals compareTag exactly
    );

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(GameplayTagComparisonOp)
    AZ_TYPE_INFO_SPECIALIZE(Heathen::GameplayTagComparisonOp, "{E5F6A7B8-C9D0-4E1F-A2B3-C4D5E6F7A8B9}");

    // ---------------------------------------------------------------------------
    // GameplayTagLogicOp
    // ---------------------------------------------------------------------------

    /// How this condition chains to the NEXT condition in a vector.
    /// The logicOp of the LAST condition in the vector is ignored.
    /// Evaluation uses C-style precedence: AND binds tightest, then OR, then XOR.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(GameplayTagLogicOp, AZ::u8,
        And, ///< this result AND next
        Or,  ///< this result OR  next
        Xor  ///< this result XOR next
    );

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(GameplayTagLogicOp)
    AZ_TYPE_INFO_SPECIALIZE(Heathen::GameplayTagLogicOp, "{F6A7B8C9-D0E1-4F2A-B3C4-D5E6F7A8B9C0}");

    // ---------------------------------------------------------------------------
    // GameplayTagCondition
    // ---------------------------------------------------------------------------

    ///<summary>
    /// A single predicate that tests one tag in a GameplayTagCollection.
    ///
    /// For Exists/NotExists: exactMatch controls whether descendants satisfy the check.
    /// For numeric ops (Equal, Less, …): compareValueType controls interpretation.
    ///   - Unsigned: u64 comparison
    ///   - Signed:   s64 comparison (bit-reinterpret)
    ///   - Decimal:  double comparison (bit-reinterpret)
    ///   - Tag:      RHS is read from compareTag's value in the collection
    /// For tag-identity ops (IsMemberOf, IsParentOf, IsExactly):
    ///   the tag's stored value is treated as a tag id; compareTag provides the RHS.
    ///
    /// Chain multiple conditions with logicOp; call EvaluateConditions() to reduce
    /// using C-style precedence (AND > OR > XOR). Returns true when the vector is empty.
    ///</summary>
    struct GameplayTagCondition
    {
        AZ_TYPE_INFO(GameplayTagCondition, "{A7B8C9D0-E1F2-4A3B-C4D5-E6F7A8B9C0D1}")
        AZ_CLASS_ALLOCATOR(GameplayTagCondition, AZ::SystemAllocator, 0)

        GameplayTag              tag;
        GameplayTagComparisonOp  comparison       = GameplayTagComparisonOp::Exists;
        AZ::u64                  compareValue     = 1;
        GameplayTag              compareTag;           ///< RHS for Tag-type and tag-identity ops
        bool                     exactMatch       = true;
        GameplayTagLogicOp       logicOp          = GameplayTagLogicOp::And;
        GameplayTagValueType     compareValueType = GameplayTagValueType::Unsigned;

        /// Evaluates this single condition against the given collection.
        bool Evaluate(const GameplayTagCollection& collection) const;

        // -----------------------------------------------------------------------
        // Set / Get (script-friendly accessors)
        // -----------------------------------------------------------------------

        void SetTag(const AZStd::string& name);
        GameplayTag GetTag() const;

        void SetComparison(GameplayTagComparisonOp op);
        GameplayTagComparisonOp GetComparison() const;

        void SetCompareValue(AZ::u64 value);
        AZ::u64 GetCompareValue() const;

        void SetCompareTag(const AZStd::string& tagName);
        GameplayTag GetCompareTag() const;

        void SetExactMatch(bool exactMatch);
        bool GetExactMatch() const;

        void SetLogicOp(GameplayTagLogicOp op);
        GameplayTagLogicOp GetLogicOp() const;

        void SetCompareValueType(GameplayTagValueType type);
        GameplayTagValueType GetCompareValueType() const;

        // -----------------------------------------------------------------------
        // Factory helpers
        // -----------------------------------------------------------------------

        static GameplayTagCondition MakeExists(
            const AZStd::string& tagName,
            bool exactMatch = true,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static GameplayTagCondition MakeNotExists(
            const AZStd::string& tagName,
            bool exactMatch = true,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static GameplayTagCondition MakeEqual(
            const AZStd::string& tagName, AZ::u64 compareValue,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static GameplayTagCondition MakeNotEqual(
            const AZStd::string& tagName, AZ::u64 compareValue,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static GameplayTagCondition MakeLess(
            const AZStd::string& tagName, AZ::u64 compareValue,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static GameplayTagCondition MakeLessEqual(
            const AZStd::string& tagName, AZ::u64 compareValue,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static GameplayTagCondition MakeGreater(
            const AZStd::string& tagName, AZ::u64 compareValue,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static GameplayTagCondition MakeGreaterEqual(
            const AZStd::string& tagName, AZ::u64 compareValue,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        /// Tag-identity ops: stored value (as tag id) is a descendant of compareTag.
        static GameplayTagCondition MakeIsMemberOf(
            const AZStd::string& tagName,
            const AZStd::string& compareTagName,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        /// Tag-identity ops: stored value (as tag id) is an ancestor of compareTag.
        static GameplayTagCondition MakeIsParentOf(
            const AZStd::string& tagName,
            const AZStd::string& compareTagName,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        /// Tag-identity ops: stored value (as tag id) equals compareTag exactly.
        static GameplayTagCondition MakeIsExactly(
            const AZStd::string& tagName,
            const AZStd::string& compareTagName,
            GameplayTagLogicOp logicOp = GameplayTagLogicOp::And);

        static void Reflect(AZ::ReflectContext* context);
    };

    // ---------------------------------------------------------------------------
    // EvaluateConditions — free function
    // ---------------------------------------------------------------------------

    ///<summary>
    /// Reduces a vector of GameplayTagConditions to a single bool using
    /// C-style operator precedence: AND > OR > XOR.
    /// Returns true when the vector is empty (unconditional).
    ///</summary>
    bool EvaluateConditions(
        const AZStd::vector<GameplayTagCondition>& conditions,
        const GameplayTagCollection& collection);

} // namespace Heathen
