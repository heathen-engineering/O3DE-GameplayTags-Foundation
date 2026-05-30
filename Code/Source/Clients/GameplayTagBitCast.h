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

// Shared bit-reinterpretation helpers used by GameplayTagCondition.cpp and
// GameplayTagOperation.cpp.  Defined as inline to avoid ODR violations in
// unity builds where both TUs are merged into a single compilation unit.

#include <bit>
#include <AzCore/base.h>

namespace Heathen::BitCast
{
    inline AZ::s64 UlongToLong  (AZ::u64 u) { return std::bit_cast<AZ::s64>(u); }
    inline AZ::u64 LongToUlong  (AZ::s64 s) { return std::bit_cast<AZ::u64>(s); }
    inline double  UlongToDouble(AZ::u64 u) { return std::bit_cast<double> (u); }
    inline AZ::u64 DoubleToUlong(double   d) { return std::bit_cast<AZ::u64>(d); }
} // namespace Heathen::BitCast
