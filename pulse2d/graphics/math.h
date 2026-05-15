/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

//////////////////////////////////////////////////////////
// box2d-lite - Heavily modified for ETL and Teensy 4.1 //
//////////////////////////////////////////////////////////

/*
 * Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erin Catto makes no representations about the suitability
 * of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#pragma once

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

/****************************************************************************
 * Math
 *
 * 2D math primitives for the physics engine. Vec2 is a two-component
 * float vector; Mat22 is a 2x2 rotation matrix constructed from an angle.
 *
 * Free functions: dot(), cross(), abs_val(), sign(), min_val(), max_val(),
 * clamp(), swap_val(), and random().
 *
 ****************************************************************************/

namespace pulse2d::graphics {

constexpr float k_pi = 3.14159265358979323846264f;

/**
 * @brief
 * Two-component float vector used for every 2D quantity in the engine:
 * position, velocity, force, impulse, contact normal, and anchor offset.
 *
 * Every impulse application in the solver is a multiply-add of the form:
 *
 *   velocity += inv_mass * impulse
 *
 * where impulse is a Vec2. Keeping Vec2 as a plain struct with fully inlined
 * operators avoids call overhead on the Teensy's Cortex-M7, where each
 * function boundary costs real pipeline cycles at 600 MHz.
 */
class Vec2
{
  public:
    Vec2()
        : x(0.0f)
        , y(0.0f)
    {
    }
    Vec2(float x, float y)
        : x(x)
        , y(y)
    {
    }

  public:
    void set(float x_, float y_)
    {
        x = x_;
        y = y_;
    }

    Vec2 operator-() { return Vec2(-x, -y); }

    void operator+=(const Vec2& v)
    {
        x += v.x;
        y += v.y;
    }

    void operator-=(const Vec2& v)
    {
        x -= v.x;
        y -= v.y;
    }

    void operator*=(float a)
    {
        x *= a;
        y *= a;
    }

    float length() const { return sqrtf(x * x + y * y); }

    float x, y;
};

/**
 * @brief
 * 2x2 rotation matrix used to transform vectors between world space and a
 * body's local coordinate frame.
 *
 * Constructed from an angle in radians it stores cos/sin in column layout:
 *
 * clang-format off
 *
 *   col1 = [ cos,  sin ]    col2 = [ -sin, cos ]
 *
 * clang-format on
 *
 * transpose() reverses the rotation (equivalent to the matrix inverse for
 * a pure rotation), used throughout collide.cc to convert world-space normals
 * into a body's local frame: local_v = rot.transpose() * world_v.
 * invert() handles the general 2x2 case; used by Joint::pre_step() to
 * compute M = K^{-1} each step.
 */
class Mat22
{
  public:
    Mat22() = default;
    explicit Mat22(float angle)
    {
        float c = cosf(angle), s = sinf(angle);
        col1.x = c;
        col2.x = -s;
        col1.y = s;
        col2.y = c;
    }

    explicit Mat22(const Vec2& col1, const Vec2& col2)
        : col1(col1)
        , col2(col2)
    {
    }

  public:
    Mat22 transpose() const
    {
        return Mat22(Vec2(col1.x, col2.x), Vec2(col1.y, col2.y));
    }

    Mat22 invert() const
    {
        float a = col1.x, b = col2.x, c = col1.y, d = col2.y;
        Mat22 B;
        float det = a * d - b * c;
        assert(det != 0.0f);
        det = 1.0f / det;
        B.col1.x = det * d;
        B.col2.x = -det * b;
        B.col1.y = -det * c;
        B.col2.y = det * a;
        return B;
    }

    Vec2 col1, col2;
};

inline float dot(const Vec2& a, const Vec2& b)
{
    return a.x * b.x + a.y * b.y;
}

inline float cross(const Vec2& a, const Vec2& b)
{
    return a.x * b.y - a.y * b.x;
}

inline Vec2 cross(const Vec2& a, float s)
{
    return Vec2(s * a.y, -s * a.x);
}

inline Vec2 cross(float s, const Vec2& a)
{
    return Vec2(-s * a.y, s * a.x);
}

inline Vec2 operator*(const Mat22& A, const Vec2& v)
{
    return Vec2(
        A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y);
}

inline Vec2 operator+(const Vec2& a, const Vec2& b)
{
    return Vec2(a.x + b.x, a.y + b.y);
}

inline Vec2 operator-(const Vec2& a, const Vec2& b)
{
    return Vec2(a.x - b.x, a.y - b.y);
}

inline Vec2 operator*(float s, const Vec2& v)
{
    return Vec2(s * v.x, s * v.y);
}

inline Mat22 operator+(const Mat22& A, const Mat22& B)
{
    return Mat22(A.col1 + B.col1, A.col2 + B.col2);
}

inline Mat22 operator*(const Mat22& A, const Mat22& B)
{
    return Mat22(A * B.col1, A * B.col2);
}

inline float abs_val(float a)
{
    return a > 0.0f ? a : -a;
}

inline Vec2 abs_val(const Vec2& a)
{
    return Vec2(fabsf(a.x), fabsf(a.y));
}

inline Mat22 abs_val(const Mat22& A)
{
    return Mat22(abs_val(A.col1), abs_val(A.col2));
}

inline float sign(float x)
{
    return x < 0.0f ? -1.0f : 1.0f;
}

inline float min_val(float a, float b)
{
    return a < b ? a : b;
}

inline float max_val(float a, float b)
{
    return a > b ? a : b;
}

inline float clamp(float a, float low, float high)
{
    return max_val(low, min_val(a, high));
}

template<typename T>
inline void swap_val(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

// random number in range [-1,1]
inline float random()
{
#if defined(__IMXRT1062__)
    float r = (float)rand() / (float)RAND_MAX;
#else
    float r = (float)arc4random() / 4294967295.0f;
#endif
    return 2.0f * r - 1.0f;
}

inline float random(float lo, float hi)
{
#if defined(__IMXRT1062__)
    float r = (float)rand() / (float)RAND_MAX;
#else
    float r = (float)arc4random() / 4294967295.0f;
#endif
    return (hi - lo) * r + lo;
}

} // namespace pulse2d
