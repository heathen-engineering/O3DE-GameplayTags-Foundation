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

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace Heathen
{
    ///<summary>
    /// A standardised registry with understanding of tree structure.
    /// Tracks tag hierarchy, name strings, and fires OnRegistryChanged
    /// on GameplayTagNotificationBus whenever the registered set changes.
    ///</summary>
    class GameplayTagRegistry
    {
    public:
        AZ_TYPE_INFO(GameplayTagRegistry, "{A1B2C3D4-E5F6-4789-9ABC-DEF012345678}");

        static void Reflect(AZ::ReflectContext* context);

        // -----------------------------------------------------------------------
        // Hashing
        // -----------------------------------------------------------------------

        static AZ::u64 Hash(const AZStd::string& text);

        // -----------------------------------------------------------------------
        // Registration
        // -----------------------------------------------------------------------

        /// Register tags from a newline-delimited list of dot-path strings.
        /// Fires OnRegistryChanged.
        static void RegisterFromStringData(const AZStd::string& tag_string);

        /// Remove tags found in the newline-delimited list and all their descendants.
        /// Fires OnRegistryChanged.
        static void UnregisterFromStringData(const AZStd::string& tag_string);

        /// Clear all user-registered tags, restoring the set to project defaults.
        /// Default tags (loaded from .tagbin assets) are unaffected.
        /// Fires OnRegistryChanged.
        static void UnregisterAll();

        /// Merge a pre-computed descendants map as project-default tags.
        /// Called once at startup for each .tagbin product.
        /// The merged result persists across UnregisterAll() calls.
        /// Fires OnRegistryChanged.
        static void MergeDefaultTags(
            const AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>>& defaults);

        // -----------------------------------------------------------------------
        // Presence / validation
        // -----------------------------------------------------------------------

        /// Returns true if the dot-path string names a registered tag.
        static bool IsRegistered(const AZStd::string& tag_string);

        /// Returns true if the pre-hashed id belongs to a registered tag.
        static bool IsRegistered(AZ::u64 id);

        /// Returns true if the dot-path passes the format rules (alphanumeric/underscore
        /// segments separated by '.', no empty segments, no leading/trailing dots).
        static bool ValidateTag(const AZStd::string& tag_string);

        // -----------------------------------------------------------------------
        // Name lookup
        // -----------------------------------------------------------------------

        /// Returns the dot-path string for a registered tag id, or an empty string
        /// if the id is unknown. Used for debug display and @@Tag.Path() interpolation.
        static AZStd::string GetName(AZ::u64 id);

        /// Returns the dot-path strings for all registered tags.
        static AZStd::vector<AZStd::string> GetAllNames();

        /// Returns the hashed ids for all registered tags.
        static AZStd::vector<AZ::u64> GetAllIds();

        // -----------------------------------------------------------------------
        // Hierarchy queries
        // -----------------------------------------------------------------------

        /// Returns true if 'tag' is a descendant of 'ancestor'.
        static bool IsDescendantOf(AZ::u64 tag, AZ::u64 ancestor);

        /// Returns the full set of descendant ids for a tag.
        static AZStd::unordered_set<AZ::u64> GetDescendants(AZ::u64 tag);

        // Internal helpers used by GameplayTagCollection::Contains (non-exact path)
        static bool ContainsAnyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor);
        static bool ContainsOnlyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor);
        static bool ContainsAllDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor);

    private:
        /// Runtime-mutable working set — user-registered tags live here.
        static AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> m_descendants;

        /// Project-default tags compiled from registered .gptags files (.tagbin products).
        /// Never cleared by UnregisterAll(); only grows via MergeDefaultTags().
        static AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> m_defaultDescendants;

        /// id → full dot-path name string. Populated whenever tags are registered.
        static AZStd::unordered_map<AZ::u64, AZStd::string> m_nameMap;

        /// Default name map — survives UnregisterAll().
        static AZStd::unordered_map<AZ::u64, AZStd::string> m_defaultNameMap;

        static AZStd::vector<AZStd::string> ParseTagString(const AZStd::string& tag_string);

        static void FireRegistryChanged();
    };

} // namespace Heathen

// =============================================================================
// GameplayTagNotificationBus
// Broadcast when the registry changes (tags registered / unregistered).
// Used by editor tag-picker widgets, ScriptCanvas wrappers, and runtime
// systems that cache tag id sets from the registry.
// =============================================================================
namespace Heathen
{
    class GameplayTagNotifications : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /// Called after any operation that changes the set of registered tags.
        virtual void OnRegistryChanged() {}
    };

    using GameplayTagNotificationBus = AZ::EBus<GameplayTagNotifications>;

} // namespace Heathen
