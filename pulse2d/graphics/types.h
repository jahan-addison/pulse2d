/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <concepts>
#include <utility>

#pragma once

namespace pulse2d::graphics::detail {

template<typename T>
concept Equality_Comparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
};

template<Equality_Comparable T>
inline void assign(T* lhs, T const* rhs)
{
    // cppcheck-suppress[duplicateConditionalAssign]
    if (*lhs != *rhs) {
        *lhs = *rhs;
    }
}

template<typename T>
inline void assign(T* lhs, T const* rhs)
{
    *lhs = *rhs;
}

} // namespace detail