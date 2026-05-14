/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 *
 * This file is part of pulse2d.
 * This software is released under the MIT License. You may use,
 * distribute, and modify this code under the terms of the license.
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <doctest/doctest.h>

#include <pulse2d/graphics/math.h>

using namespace pulse2d::graphics;

TEST_CASE("math.h: Vec2 default constructor is zero")
{
    Vec2 v;
    CHECK(v.x == 0.0f);
    CHECK(v.y == 0.0f);
}

TEST_CASE("math.h: Vec2 parameterized constructor")
{
    Vec2 v(3.0f, -4.0f);
    CHECK(v.x == 3.0f);
    CHECK(v.y == -4.0f);
}

TEST_CASE("math.h: Vec2::set")
{
    Vec2 v;
    v.set(1.5f, -2.5f);
    CHECK(v.x == 1.5f);
    CHECK(v.y == -2.5f);
}

TEST_CASE("math.h: Vec2 unary negation")
{
    Vec2 v(3.0f, -4.0f);
    Vec2 neg = -v;
    CHECK(neg.x == -3.0f);
    CHECK(neg.y == 4.0f);
}

TEST_CASE("math.h: Vec2::operator+=")
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    a += b;
    CHECK(a.x == 4.0f);
    CHECK(a.y == 6.0f);
}

TEST_CASE("math.h: Vec2::operator-=")
{
    Vec2 a(5.0f, 6.0f);
    Vec2 b(1.0f, 2.0f);
    a -= b;
    CHECK(a.x == 4.0f);
    CHECK(a.y == 4.0f);
}

TEST_CASE("math.h: Vec2::operator*=")
{
    Vec2 v(2.0f, 3.0f);
    v *= 2.0f;
    CHECK(v.x == 4.0f);
    CHECK(v.y == 6.0f);
}

TEST_CASE("math.h: Vec2::length of a 3-4-5 right triangle")
{
    Vec2 v(3.0f, 4.0f);
    CHECK(v.length() == doctest::Approx(5.0f));
}

TEST_CASE("math.h: Vec2::length of zero vector is zero")
{
    Vec2 v(0.0f, 0.0f);
    CHECK(v.length() == 0.0f);
}

TEST_CASE("math.h: Vec2 operator+ produces a new vector")
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    Vec2 c = a + b;
    CHECK(c.x == 4.0f);
    CHECK(c.y == 6.0f);
}

TEST_CASE("math.h: Vec2 operator- produces a new vector")
{
    Vec2 a(5.0f, 6.0f);
    Vec2 b(1.0f, 2.0f);
    Vec2 c = a - b;
    CHECK(c.x == 4.0f);
    CHECK(c.y == 4.0f);
}

TEST_CASE("math.h: scalar * Vec2")
{
    Vec2 v(2.0f, 3.0f);
    Vec2 r = 4.0f * v;
    CHECK(r.x == 8.0f);
    CHECK(r.y == 12.0f);
}

TEST_CASE("math.h: Mat22 from angle 0 is identity")
{
    Mat22 m(0.0f);
    CHECK(m.col1.x == doctest::Approx(1.0f));
    CHECK(m.col1.y == doctest::Approx(0.0f));
    CHECK(m.col2.x == doctest::Approx(0.0f));
    CHECK(m.col2.y == doctest::Approx(1.0f));
}

TEST_CASE("math.h: Mat22 from pi/2 is a 90 degree rotation")
{
    Mat22 m(k_pi / 2.0f);
    // col1 = (cos, sin) = (0, 1)
    // col2 = (-sin, cos) = (-1, 0)
    CHECK(m.col1.x == doctest::Approx(0.0f).epsilon(1e-6f));
    CHECK(m.col1.y == doctest::Approx(1.0f).epsilon(1e-6f));
    CHECK(m.col2.x == doctest::Approx(-1.0f).epsilon(1e-6f));
    CHECK(m.col2.y == doctest::Approx(0.0f).epsilon(1e-6f));
}

TEST_CASE("math.h: Mat22 from two column vectors")
{
    Mat22 m(Vec2(1.0f, 2.0f), Vec2(3.0f, 4.0f));
    CHECK(m.col1.x == 1.0f);
    CHECK(m.col1.y == 2.0f);
    CHECK(m.col2.x == 3.0f);
    CHECK(m.col2.y == 4.0f);
}

TEST_CASE("math.h: Mat22::transpose swaps off-diagonal elements")
{
    Mat22 m(Vec2(1.0f, 2.0f), Vec2(3.0f, 4.0f));
    Mat22 t = m.transpose();
    CHECK(t.col1.x == 1.0f);
    CHECK(t.col1.y == 3.0f);
    CHECK(t.col2.x == 2.0f);
    CHECK(t.col2.y == 4.0f);
}

TEST_CASE("math.h: Mat22 transpose of rotation matrix is its inverse")
{
    // R * R^T = I for any rotation matrix
    Mat22 r(k_pi / 4.0f);
    Mat22 rt = r.transpose();
    Mat22 id = r * rt;
    CHECK(id.col1.x == doctest::Approx(1.0f).epsilon(1e-6f));
    CHECK(id.col1.y == doctest::Approx(0.0f).epsilon(1e-6f));
    CHECK(id.col2.x == doctest::Approx(0.0f).epsilon(1e-6f));
    CHECK(id.col2.y == doctest::Approx(1.0f).epsilon(1e-6f));
}

TEST_CASE("math.h: Mat22::invert of identity is identity")
{
    Mat22 identity(Vec2(1.0f, 0.0f), Vec2(0.0f, 1.0f));
    Mat22 inv = identity.invert();
    CHECK(inv.col1.x == doctest::Approx(1.0f));
    CHECK(inv.col1.y == doctest::Approx(0.0f));
    CHECK(inv.col2.x == doctest::Approx(0.0f));
    CHECK(inv.col2.y == doctest::Approx(1.0f));
}

TEST_CASE("math.h: Mat22::invert satisfies M * M^-1 = I")
{
    Mat22 m(Vec2(2.0f, 1.0f), Vec2(1.0f, 3.0f));
    Mat22 inv = m.invert();
    Mat22 id = m * inv;
    CHECK(id.col1.x == doctest::Approx(1.0f).epsilon(1e-5f));
    CHECK(id.col1.y == doctest::Approx(0.0f).epsilon(1e-5f));
    CHECK(id.col2.x == doctest::Approx(0.0f).epsilon(1e-5f));
    CHECK(id.col2.y == doctest::Approx(1.0f).epsilon(1e-5f));
}

TEST_CASE("math.h: Mat22 * Vec2 with identity leaves vector unchanged")
{
    Mat22 id(0.0f);
    Vec2 v(2.0f, 3.0f);
    Vec2 result = id * v;
    CHECK(result.x == doctest::Approx(2.0f));
    CHECK(result.y == doctest::Approx(3.0f));
}

TEST_CASE("math.h: Mat22 * Vec2 rotates correctly at pi/2")
{
    // rotating (1, 0) by 90 degrees counter-clockwise should give (0, 1)
    Mat22 r(k_pi / 2.0f);
    Vec2 v(1.0f, 0.0f);
    Vec2 result = r * v;
    CHECK(result.x == doctest::Approx(0.0f).epsilon(1e-6f));
    CHECK(result.y == doctest::Approx(1.0f).epsilon(1e-6f));
}

TEST_CASE("math.h: Mat22 + Mat22")
{
    Mat22 a(Vec2(1.0f, 2.0f), Vec2(3.0f, 4.0f));
    Mat22 b(Vec2(5.0f, 6.0f), Vec2(7.0f, 8.0f));
    Mat22 c = a + b;
    CHECK(c.col1.x == 6.0f);
    CHECK(c.col1.y == 8.0f);
    CHECK(c.col2.x == 10.0f);
    CHECK(c.col2.y == 12.0f);
}

TEST_CASE("math.h: dot product of perpendicular vectors is zero")
{
    Vec2 a(1.0f, 0.0f);
    Vec2 b(0.0f, 1.0f);
    CHECK(dot(a, b) == 0.0f);
}

TEST_CASE("math.h: dot product of parallel vectors")
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    CHECK(dot(a, b) == 11.0f); // 1*3 + 2*4
}

TEST_CASE("math.h: dot product is commutative")
{
    Vec2 a(3.0f, 5.0f);
    Vec2 b(7.0f, 2.0f);
    CHECK(dot(a, b) == doctest::Approx(dot(b, a)));
}

TEST_CASE("math.h: cross(Vec2, Vec2) of unit axes is 1")
{
    Vec2 x(1.0f, 0.0f);
    Vec2 y(0.0f, 1.0f);
    CHECK(cross(x, y) == doctest::Approx(1.0f));
}

TEST_CASE("math.h: cross(Vec2, Vec2) is anticommutative")
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    CHECK(cross(a, b) == doctest::Approx(-cross(b, a)));
}

TEST_CASE("math.h: cross(Vec2, float) result")
{
    // cross(a, s) = { s * a.y, -s * a.x }
    Vec2 a(1.0f, 2.0f);
    Vec2 result = cross(a, 3.0f);
    CHECK(result.x == doctest::Approx(6.0f));
    CHECK(result.y == doctest::Approx(-3.0f));
}

TEST_CASE("math.h: cross(float, Vec2) result")
{
    // cross(s, a) = { -s * a.y, s * a.x }
    Vec2 a(1.0f, 2.0f);
    Vec2 result = cross(3.0f, a);
    CHECK(result.x == doctest::Approx(-6.0f));
    CHECK(result.y == doctest::Approx(3.0f));
}

TEST_CASE("math.h: abs_val float")
{
    CHECK(abs_val(-3.0f) == 3.0f);
    CHECK(abs_val(3.0f) == 3.0f);
    CHECK(abs_val(0.0f) == 0.0f);
}

TEST_CASE("math.h: abs_val Vec2")
{
    Vec2 v(-2.0f, 3.0f);
    Vec2 a = abs_val(v);
    CHECK(a.x == 2.0f);
    CHECK(a.y == 3.0f);
}

TEST_CASE("math.h: abs_val Mat22")
{
    Mat22 m(Vec2(-1.0f, 2.0f), Vec2(3.0f, -4.0f));
    Mat22 a = abs_val(m);
    CHECK(a.col1.x == 1.0f);
    CHECK(a.col1.y == 2.0f);
    CHECK(a.col2.x == 3.0f);
    CHECK(a.col2.y == 4.0f);
}

TEST_CASE("math.h: sign of negative is -1, non-negative is +1")
{
    CHECK(sign(-5.0f) == -1.0f);
    CHECK(sign(5.0f) == 1.0f);
    CHECK(sign(0.0f) == 1.0f);
}

TEST_CASE("math.h: min_val returns the smaller value")
{
    CHECK(min_val(2.0f, 5.0f) == 2.0f);
    CHECK(min_val(5.0f, 2.0f) == 2.0f);
    CHECK(min_val(-1.0f, 1.0f) == -1.0f);
}

TEST_CASE("math.h: max_val returns the larger value")
{
    CHECK(max_val(2.0f, 5.0f) == 5.0f);
    CHECK(max_val(5.0f, 2.0f) == 5.0f);
    CHECK(max_val(-1.0f, 1.0f) == 1.0f);
}

TEST_CASE("math.h: clamp within range is unchanged")
{
    CHECK(clamp(5.0f, 0.0f, 10.0f) == 5.0f);
}

TEST_CASE("math.h: clamp below range returns low")
{
    CHECK(clamp(-1.0f, 0.0f, 10.0f) == 0.0f);
}

TEST_CASE("math.h: clamp above range returns high")
{
    CHECK(clamp(11.0f, 0.0f, 10.0f) == 10.0f);
}

TEST_CASE("math.h: swap_val exchanges two values")
{
    float a = 1.0f;
    float b = 2.0f;
    swap_val(a, b);
    CHECK(a == 2.0f);
    CHECK(b == 1.0f);
}

TEST_CASE("math.h: swap_val works with Vec2")
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b(3.0f, 4.0f);
    swap_val(a, b);
    CHECK(a.x == 3.0f);
    CHECK(b.x == 1.0f);
}

TEST_CASE("math.h: random() stays in [-1, 1]")
{
    for (int i = 0; i < 200; ++i) {
        // NOLINTNEXTLINE
        float r = pulse2d::graphics::random();
        CHECK(r >= -1.0f);
        CHECK(r <= 1.0f);
    }
}

TEST_CASE("math.h: random(lo, hi) stays in [lo, hi]")
{
    for (int i = 0; i < 200; ++i) {
        // NOLINTNEXTLINE
        float r = pulse2d::graphics::random(2.0f, 5.0f);
        CHECK(r >= 2.0f);
        CHECK(r <= 5.0f);
    }
}
