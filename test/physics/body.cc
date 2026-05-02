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
#include <luya/physics/math.h>

#include <float.h>

using namespace luya::physics;

TEST_CASE("physics/body.cc: Body default constructor is static")
{
    Body b;
    // A default body has infinite mass so it never moves
    CHECK(b.mass == FLT_MAX);
    CHECK(b.inv_mass == 0.0f);
    CHECK(b.I == FLT_MAX);
    CHECK(b.inv_i == 0.0f);
}

TEST_CASE("physics/body.cc: Body default constructor zeroes kinematic state")
{
    Body b;
    CHECK(b.position.x == 0.0f);
    CHECK(b.position.y == 0.0f);
    CHECK(b.rotation == 0.0f);
    CHECK(b.velocity.x == 0.0f);
    CHECK(b.velocity.y == 0.0f);
    CHECK(b.angular_velocity == 0.0f);
    CHECK(b.force.x == 0.0f);
    CHECK(b.force.y == 0.0f);
    CHECK(b.torque == 0.0f);
}

TEST_CASE("physics/body.cc: Body default friction is 0.2")
{
    Body b;
    CHECK(b.friction == doctest::Approx(0.2f));
}

TEST_CASE("physics/body.cc: Body::set stores full dimensions and mass")
{
    Body b;
    b.set({ 0.5f, 0.5f }, 2.0f);
    CHECK(b.width.x == 0.5f);
    CHECK(b.width.y == 0.5f);
    CHECK(b.mass == 2.0f);
}

TEST_CASE("physics/body.cc: Body::set computes inverse mass")
{
    Body b;
    b.set({ 0.5f, 0.5f }, 4.0f);
    CHECK(b.inv_mass == doctest::Approx(0.25f));
}

TEST_CASE(
    "physics/body.cc: Body::set computes moment of inertia from box formula")
{
    Body b;
    // I = mass * (w.x^2 + w.y^2) / 12  (w = full dimensions)
    // with width = (1, 1) and mass = 12:  I = 12 * (1 + 1) / 12 = 2
    b.set({ 1.0f, 1.0f }, 12.0f);
    CHECK(b.I == doctest::Approx(2.0f));
    CHECK(b.inv_i == doctest::Approx(0.5f));
}

TEST_CASE("physics/body.cc: Body::set with FLT_MAX mass stays static")
{
    Body b;
    b.set({ 1.0f, 1.0f }, FLT_MAX);
    CHECK(b.inv_mass == 0.0f);
    CHECK(b.I == FLT_MAX);
    CHECK(b.inv_i == 0.0f);
}

TEST_CASE("physics/body.cc: Body::set resets kinematic state to zero")
{
    Body b;
    b.velocity = { 5.0f, 3.0f };
    b.angular_velocity = 1.0f;
    b.position = { 10.0f, 20.0f };
    b.rotation = 1.0f;

    b.set({ 0.5f, 0.5f }, 1.0f);

    CHECK(b.velocity.x == 0.0f);
    CHECK(b.velocity.y == 0.0f);
    CHECK(b.angular_velocity == 0.0f);
    CHECK(b.position.x == 0.0f);
    CHECK(b.position.y == 0.0f);
    CHECK(b.rotation == 0.0f);
    CHECK(b.force.x == 0.0f);
    CHECK(b.torque == 0.0f);
}

TEST_CASE("physics/body.cc: Body::set is safe to call a second time")
{
    Body b;
    b.set({ 0.5f, 0.5f }, 1.0f);
    b.position = { 10.0f, 5.0f };
    b.velocity = { 3.0f, 3.0f };

    b.set({ 2.0f, 1.0f }, 5.0f);

    CHECK(b.width.x == 2.0f);
    CHECK(b.mass == 5.0f);
    CHECK(b.position.x == 0.0f);
    CHECK(b.velocity.x == 0.0f);
}

TEST_CASE("physics/body.cc: Body::add_force accumulates into force")
{
    Body b;
    b.set({ 0.5f, 0.5f }, 1.0f);
    b.add_force({ 1.0f, 0.0f });
    b.add_force({ 2.0f, 3.0f });
    CHECK(b.force.x == doctest::Approx(3.0f));
    CHECK(b.force.y == doctest::Approx(3.0f));
}

TEST_CASE("physics/body.cc: Body::add_force on static body accumulates but has "
          "no effect on velocity")
{
    // inv_mass = 0 so forces never change velocity
    Body b;
    b.add_force({ 100.0f, 0.0f });
    CHECK(b.force.x == 100.0f);
    CHECK(b.inv_mass == 0.0f);
    // velocity unchanged - step() will multiply by inv_mass = 0
    CHECK(b.velocity.x == 0.0f);
}

TEST_CASE("physics/body.cc: Body::set inv_mass is exactly 1/mass")
{
    Body b;
    b.set({ 0.5f, 0.5f }, 8.0f);
    CHECK(b.inv_mass == doctest::Approx(1.0f / 8.0f));
}

TEST_CASE("physics/body.cc: Body::set moment of inertia scales with mass")
{
    Body b1, b2;
    b1.set({ 1.0f, 1.0f }, 1.0f);
    b2.set({ 1.0f, 1.0f }, 2.0f);
    // I = mass * (w.x^2 + w.y^2) / 12 - doubling mass doubles I
    CHECK(b2.I == doctest::Approx(b1.I * 2.0f));
}

TEST_CASE("physics/body.cc: Body::set moment of inertia scales with box size")
{
    Body b1, b2;
    b1.set({ 1.0f, 1.0f }, 1.0f);
    b2.set({ 2.0f, 2.0f }, 1.0f);
    // wider box has higher moment of inertia
    CHECK(b2.I > b1.I);
}
