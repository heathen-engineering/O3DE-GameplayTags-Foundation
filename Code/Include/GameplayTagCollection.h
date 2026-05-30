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

#include <bit>
#include <AzCore/EBus/Event.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utility/pair.h>
#include "GameplayTag.h"

namespace Heathen
{
    /// Arithmetic operation applied to a tag's u64 value inside a GameplayTagCollection.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(GameplayTagArithmetic, AZ::u8,
        Set,
        Add,
        Subtract,
        Multiply,
        Divide,
        Min,
        Max
    );

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(GameplayTagArithmetic)
    AZ_TYPE_INFO_SPECIALIZE(Heathen::GameplayTagArithmetic, "{F3A7B2C9-D4E1-4F8A-B6C3-2E9D5A7F1B4E}");

    /// <summary>
    /// A collection of GameplayTags each with an associated u64 value.
    ///
    /// Value invariant: a value of 0 is never stored — tags are removed when their value
    /// reaches 0.  Simple presence (AddTag) stores a value of 1.
    ///
    /// Registry-aware operations (Contains with exactMatch=false, GetMatchingTags, etc.)
    /// require tags to be registered in GameplayTagRegistry. Unregistered tags are treated
    /// as not found with no error or exception.
    ///
    /// The Changed event fires after any mutation.  It does NOT fire when the collection
    /// is copy-constructed or copy-assigned — subscribers are not transferred on copy.
    /// </summary>
    class GameplayTagCollection
    {
    public:
        AZ_TYPE_INFO(GameplayTagCollection, "{A8C2F5B3-6E3A-4F2A-9C2D-9B7F7C4A1E11}");
        AZ_CLASS_ALLOCATOR(GameplayTagCollection, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        // -----------------------------------------------------------------------
        // Copy / move
        // AZ::Event<> is non-copyable, so we must explicitly handle copy/assign
        // to ensure subscribers are NOT transferred when the collection is copied
        // (e.g., for save states).
        // -----------------------------------------------------------------------

        GameplayTagCollection() = default;
        ~GameplayTagCollection() = default;

        GameplayTagCollection(const GameplayTagCollection& other);
        GameplayTagCollection& operator=(const GameplayTagCollection& other);

        GameplayTagCollection(GameplayTagCollection&&) = default;
        GameplayTagCollection& operator=(GameplayTagCollection&&) = default;

        // -----------------------------------------------------------------------
        // Changed event
        // Connect with: collection.GetChangedEvent().Connect(handler)
        // The handler receives a const reference to the collection after mutation.
        // -----------------------------------------------------------------------

        AZ::Event<const GameplayTagCollection&>& GetChangedEvent() { return m_changed; }

        // -----------------------------------------------------------------------
        // Mutation
        // -----------------------------------------------------------------------

        /// Adds the tag with a value of 1. Shortcut for Apply(tag, Arithmetic::Set, 1).
        void AddTag(const GameplayTag& tag);

        /// Adds each name in the range as a tag with value 1.
        void AddStringRange(const AZStd::vector<AZStd::string>& names);

        /// Adds each tag in the range with value 1.
        void AddTagRange(const AZStd::vector<GameplayTag>& tags);

        /// Adds all tags from another collection (value 1 for each, regardless of their stored value).
        void AddCollectionRange(const GameplayTagCollection& other);

        /// Removes the tag unconditionally. Shortcut for Apply(tag, Arithmetic::Set, 0).
        void RemoveTag(const GameplayTag& tag);

        /// Removes all tags from the collection. Fires Changed.
        void Clear();

        /// Applies an arithmetic operation to the tag's current value.
        /// If the result is 0 the tag is removed.  Invalid tags (id == 0) are ignored.
        /// Fires Changed if the value actually changes.
        void Apply(const GameplayTag& tag, GameplayTagArithmetic operation, AZ::u64 value);

        // -----------------------------------------------------------------------
        // Read
        // -----------------------------------------------------------------------

        /// Returns the raw u64 value stored for the tag, or 0 if absent.
        AZ::u64 GetValue(const GameplayTag& tag) const;

        // -----------------------------------------------------------------------
        // Typed value accessors
        // Underlying storage is always u64.
        // float/int occupy the lower 32 bits; long/double use all 64 bits.
        // A stored value of 0 is treated as "not present" (returns 0.0f / 0 etc.).
        // -----------------------------------------------------------------------

        float  GetFloat (const GameplayTag& tag) const;
        int    GetInt   (const GameplayTag& tag) const;
        AZ::s64 GetLong (const GameplayTag& tag) const;
        double GetDouble(const GameplayTag& tag) const;

        void SetFloat (const GameplayTag& tag, float   value);
        void SetInt   (const GameplayTag& tag, int     value);
        void SetLong  (const GameplayTag& tag, AZ::s64 value);
        void SetDouble(const GameplayTag& tag, double  value);

        /// Stores the xxHash64 of tagPath as the value of tag.
        /// Useful for enum-like patterns: collection.SetTagValue("Player.Class", "Classes.Melee.Warrior")
        /// Retrieve with: GameplayTagRegistry::GetName(collection.GetValue(tag))
        void SetTagValue(const GameplayTag& tag, const AZStd::string& tagPath);

        // -----------------------------------------------------------------------
        // Enumeration
        // -----------------------------------------------------------------------

        /// Returns all (tag, value) pairs currently in the collection.
        AZStd::vector<AZStd::pair<GameplayTag, AZ::u64>> GetAll() const;

        // -----------------------------------------------------------------------
        // Presence checks — single tag
        // -----------------------------------------------------------------------

        /// exactMatch = true  : the tag must be present exactly.
        /// exactMatch = false : the tag OR any of its descendants must be present.
        ///                      Requires the tag to be registered; returns false if not.
        bool Contains(const GameplayTag& tag, bool exactMatch) const;

        // -----------------------------------------------------------------------
        // Presence checks — collection vs collection
        // -----------------------------------------------------------------------

        bool ContainsAll (const GameplayTagCollection& other, bool exactMatch) const;
        bool ContainsAny (const GameplayTagCollection& other, bool exactMatch) const;
        bool ContainsNone(const GameplayTagCollection& other, bool exactMatch) const;

        // -----------------------------------------------------------------------
        // Query
        // -----------------------------------------------------------------------

        AZStd::vector<GameplayTag> GetMatchingTags (const GameplayTag& tag, bool exactMatch) const;
        AZStd::vector<GameplayTag> GetExcludingTags(const GameplayTag& tag, bool exactMatch) const;
        GameplayTagCollection      GetShared        (const GameplayTagCollection& other, bool exactMatch) const;
        GameplayTagCollection      GetExclusive     (const GameplayTagCollection& other, bool exactMatch) const;

        /// Registry passthrough — returns all registered descendants of ancestor.
        static AZStd::vector<GameplayTag> GetDescendants(const GameplayTag& ancestor);

        /// Creates a collection pre-populated from an array of tag name strings.
        static GameplayTagCollection Make(const AZStd::vector<AZStd::string>& names);

        // -----------------------------------------------------------------------
        // Utility
        // -----------------------------------------------------------------------

        bool    IsEmpty() const;
        AZ::u32 Count()   const;

        // -----------------------------------------------------------------------
        // C++ only — not exposed to BehaviorContext
        // -----------------------------------------------------------------------

        /// Returns true if every registered descendant of ancestor is present.
        bool ContainsAllDescendants(const GameplayTag& ancestor) const;

    private:
        // key   = tag hash (u64)
        // value = raw u64 value
        // invariant: value of 0 is never stored; key is erased when result reaches 0
        AZStd::unordered_map<AZ::u64, AZ::u64> m_tags;

        // Fires after every mutation that changes m_tags.
        AZ::Event<const GameplayTagCollection&> m_changed;

        // Bit-reinterpretation helpers (C++20 std::bit_cast — no UB, no memcpy)
        static float   UintToFloat (AZ::u32  u) { return std::bit_cast<float>  (u); }
        static AZ::u32 FloatToUint (float    f) { return std::bit_cast<AZ::u32>(f); }
        static double  UlongToDouble(AZ::u64 u) { return std::bit_cast<double> (u); }
        static AZ::u64 DoubleToUlong(double  d) { return std::bit_cast<AZ::u64>(d); }
    };

} // namespace Heathen
