/*****************************************************************************
 * Copyright (c) Jahan Addison
 *
 * This software is dual-licensed under the Apache License, Version 2.0 or
 * the GNU General Public License, Version 3.0 or later.
 *
 * You may use this work, in part or in whole, under the terms of either
 * license.
 *
 * See the LICENSE.Apache-v2 and LICENSE.GPL-v3 files in the project root
 * for the full text of these licenses.
 ****************************************************************************/

#include <doctest/doctest.h>

#include <luya/physics/body.h>
#include <luya/physics/joint.h>
#include <luya/physics/math.h>
#include <luya/physics/world.h>

#include <math.h>

using namespace luya::physics;

// Two dynamic boxes placed symmetrically around a pin at the origin.
// With no gravity, only the joint acts.
struct Joint_Fixture
{
    Body a;
    Body b;
    Joint pin;
    World world;

    Joint_Fixture()
        : world({ 0.0f, 0.0f }, 10)
    {
        a.set_mass({ 0.5f, 0.5f }, 1.0f);
        a.position = { -1.0f, 0.0f };

        b.set_mass({ 0.5f, 0.5f }, 1.0f);
        b.position = { 1.0f, 0.0f };

        // anchor at origin: 1 unit from each body's center
        pin.set(&a, &b, { 0.0f, 0.0f });

        world.add(&a);
        world.add(&b);
        world.add(&pin);
    }

    ~Joint_Fixture() = default;
};

TEST_CASE_FIXTURE(Joint_Fixture, "joint.cc: Joint::set stores both bodies")
{
    CHECK(pin.body1 != nullptr);
    CHECK(pin.body2 != nullptr);
}

TEST_CASE_FIXTURE(Joint_Fixture,
    "joint.cc: Joint::set computes correct local anchors")
{
    // a is at (-1, 0) and anchor is at (0, 0), rotation is 0
    // local_anchor for a = rot_t * (anchor - a.position) = (1, 0)
    // local_anchor for b = rot_t * (anchor - b.position) = (-1, 0)
    CHECK(pin.local_anchor1.x == doctest::Approx(1.0f));
    CHECK(pin.local_anchor1.y == doctest::Approx(0.0f));
    CHECK(pin.local_anchor2.x == doctest::Approx(-1.0f));
    CHECK(pin.local_anchor2.y == doctest::Approx(0.0f));
}

TEST_CASE_FIXTURE(Joint_Fixture,
    "joint.cc: Joint::set resets accumulated impulse to zero")
{
    CHECK(pin.P.x == 0.0f);
    CHECK(pin.P.y == 0.0f);
}

TEST_CASE_FIXTURE(Joint_Fixture,
    "joint.cc: Joint::set initializes default bias_factor and softness")
{
    CHECK(pin.bias_factor == doctest::Approx(0.2f));
    CHECK(pin.softness == doctest::Approx(0.0f));
}

TEST_CASE_FIXTURE(Joint_Fixture,
    "joint.cc: Joint::set local anchors are in body-local space")
{
    // rotate a by 90 degrees and re-set; the local anchor should change
    Joint pin2;
    Body c;
    c.set_mass({ 0.5f, 0.5f }, 1.0f);
    c.position = { -1.0f, 0.0f };
    c.rotation = k_pi / 2.0f;

    Body d;
    d.set_mass({ 0.5f, 0.5f }, 1.0f);
    d.position = { 1.0f, 0.0f };

    pin2.set(&c, &d, { 0.0f, 0.0f });

    // With c rotated 90 degrees, the anchor offset (1, 0) in world space
    // should be expressed as (0, -1) in c's local frame
    // (rot_t rotates world vector back to local)
    CHECK(pin2.local_anchor1.x == doctest::Approx(0.0f).epsilon(1e-5f));
    CHECK(pin2.local_anchor1.y == doctest::Approx(-1.0f).epsilon(1e-5f));
}

TEST_CASE_FIXTURE(Joint_Fixture,
    "joint.cc: Joint::pre_step builds a non-degenerate effective mass "
    "matrix")
{
    float inv_dt = 60.0f;
    pin.pre_step(inv_dt);

    // M = K^-1 where K is the generalized mass of the constraint.
    // det(M) != 0 means it is invertible and the constraint can be solved.
    float det = pin.M.col1.x * pin.M.col2.y - pin.M.col2.x * pin.M.col1.y;
    CHECK(det != doctest::Approx(0.0f).epsilon(1e-10f));
}

TEST_CASE_FIXTURE(Joint_Fixture,
    "joint.cc: Joint::pre_step rotates local anchors to world space")
{
    pin.pre_step(60.0f);
    // body rotations are 0, so r1 == local_anchor1 and r2 == local_anchor2
    CHECK(pin.r1.x == doctest::Approx(pin.local_anchor1.x).epsilon(1e-5f));
    CHECK(pin.r1.y == doctest::Approx(pin.local_anchor1.y).epsilon(1e-5f));
    CHECK(pin.r2.x == doctest::Approx(pin.local_anchor2.x).epsilon(1e-5f));
    CHECK(pin.r2.y == doctest::Approx(pin.local_anchor2.y).epsilon(1e-5f));
}

TEST_CASE_FIXTURE(Joint_Fixture,
    "joint.cc: Joint::pre_step bias is zero when constraint is already "
    "satisfied")
{
    // At construction both anchor points are already coincident at the origin,
    // so dp = p2 - p1 = 0 and bias should be zero
    World::position_correction = true;
    pin.pre_step(60.0f);
    CHECK(pin.bias.x == doctest::Approx(0.0f).epsilon(1e-6f));
    CHECK(pin.bias.y == doctest::Approx(0.0f).epsilon(1e-6f));
}

TEST_CASE("joint.cc: Joint::pre_step bias is non-zero when anchor "
          "points have drifted")
{
    World::position_correction = true;

    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { -2.0f, 0.0f }; // further from anchor than at set() time

    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.position = { 2.0f, 0.0f };

    Joint pin;
    // set with a and b at their current positions, then move a to drift
    pin.set(&a, &b, { 0.0f, 0.0f });
    a.position = { -3.0f, 0.0f }; // drift a 1 unit further

    pin.pre_step(60.0f);

    // p1 = a.position + r1 is now at (-2, 0) instead of (0, 0)
    // dp = p2 - p1 != 0, so bias should be non-zero
    float bias_len = sqrtf(pin.bias.x * pin.bias.x + pin.bias.y * pin.bias.y);
    CHECK(bias_len > 0.0f);
}

TEST_CASE("joint.cc: Joint constraint keeps anchor points close after "
          "applied velocity")
{
    // Give both bodies velocities pulling them apart from the joint anchor.
    // After running the simulation for many steps the constraint should have
    // corrected most of the drift - the two world-space anchor points should
    // still be close to each other.

    World world({ 0.0f, 0.0f }, 20);

    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { -1.0f, 0.0f };
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.position = { 1.0f, 0.0f };

    Joint pin;
    pin.set(&a, &b, { 0.0f, 0.0f });

    world.add(&a);
    world.add(&b);
    world.add(&pin);

    // push both bodies away from the anchor
    a.velocity = { -8.0f, 0.0f };
    b.velocity = { 8.0f, 0.0f };

    for (int i = 0; i < 60; ++i)
        world.step(1.0f / 60.0f);

    // compute the world-space pin point on each body
    // rotation should be small after a pure x-velocity push so local frame
    // is approximately the world frame
    Mat22 rot_a(a.rotation);
    Mat22 rot_b(b.rotation);
    Vec2 p1 = a.position + rot_a * pin.local_anchor1;
    Vec2 p2 = b.position + rot_b * pin.local_anchor2;

    float error = (p2 - p1).length();
    CHECK(error < 1.0f); // constraint pulled them back - error is bounded
}

TEST_CASE("joint.cc: Joint with stiffer bias_factor corrects drift faster")
{
    // Seed a 1-unit position error directly by teleporting b after set().
    // The bias term corrects position error at rate (1 - bias_factor)^step,
    // so a higher factor converges faster. After 5 steps:
    //   factor=0.8: error * (0.2)^5 ≈ 0.03 %
    //   factor=0.1: error * (0.9)^5 ≈ 59 %

    auto run = [](float factor) -> float {
        World world({ 0.0f, 0.0f }, 10);
        Body a, b;
        a.set_mass({ 0.5f, 0.5f }, 1.0f);
        a.position = { -1.0f, 0.0f };
        b.set_mass({ 0.5f, 0.5f }, 1.0f);
        b.position = { 1.0f, 0.0f };

        Joint pin;
        pin.set(&a, &b, { 0.0f, 0.0f }); // local_anchor2 = {-1, 0}
        pin.bias_factor = factor;

        // Teleport b to introduce ~1.5-unit error at the pin.
        // local_anchor2 stays {-1, 0}, so world_pin2 = 2.5 - 1 = 1.5 ≠ 0.
        b.position = { 2.5f, 0.0f };

        world.add(&a);
        world.add(&b);
        world.add(&pin);

        for (int i = 0; i < 5; ++i)
            world.step(1.0f / 60.0f);

        Mat22 rot_a(a.rotation);
        Mat22 rot_b(b.rotation);
        Vec2 p1 = a.position + rot_a * pin.local_anchor1;
        Vec2 p2 = b.position + rot_b * pin.local_anchor2;
        return (p2 - p1).length();
    };

    float error_low = run(0.1f);
    float error_high = run(0.8f);
    CHECK(error_high < error_low);
}
