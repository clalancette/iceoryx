// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
// Copyright (c) 2021 by Apex.AI Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#ifndef IOX_HOOFS_CXX_NEWTYPE_COMPARABLE_HPP
#define IOX_HOOFS_CXX_NEWTYPE_COMPARABLE_HPP
#include "iceoryx_hoofs/internal/cxx/newtype/internal.hpp"

namespace iox
{
namespace cxx
{
namespace newtype
{
template <typename T>
struct Comparable
{
    friend bool operator==(const T& lhs, const T& rhs) noexcept
    {
        return internal::newTypeAccessor(lhs) == internal::newTypeAccessor(rhs);
    }

    friend bool operator!=(const T& lhs, const T& rhs) noexcept
    {
        return !(lhs == rhs);
    }

  protected:
    ~Comparable() = default;
};

} // namespace newtype
} // namespace cxx
} // namespace iox

#endif
