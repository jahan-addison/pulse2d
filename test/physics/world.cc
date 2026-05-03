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

using namespace luya::physics;

// A dynamic box falling toward a static floor.
struct World_Fixture
{
    Body floor;
    Body box;
    World world;

    World_Fixture()
        : world({ 0.0f, -10.0f }, 10)
    {
        floor.width = { 10.0f, 0.5f };
        floor.position = { 0.0f, -5.0f }; // static - default-constructed

        box.set({ 0.5f, 0.5f }, 1.0f);
        box.position = { 0.0f, 3.0f }; // well above the floor

        world.add(&floor);
        world.add(&box);
    }

    ~World_Fixture() = default;
};

TEST_CASE("world.cc: World::add registers bodies")
{
    World world({ 0.0f, -10.0f }, 10);
    Body a, b;
    world.add(&a);
    world.add(&b);
    CHECK(world.bodies.size() == 2);
}

TEST_CASE("world.cc: World::add registers joints")
{
    World world({ 0.0f, -10.0f }, 10);
    Body a, b;
    a.set({ 0.5f, 0.5f }, 1.0f);
    b.set({ 0.5f, 0.5f }, 1.0f);
    b.position = { 2.0f, 0.0f };
    world.add(&a);
    world.add(&b);
    Joint j;
    j.set(&a, &b, { 1.0f, 0.0f });
    world.add(&j);
    CHECK(world.joints.size() == 1);
}

TEST_CASE("world.cc: World::clear empties bodies, joints, and arbiters")
{
    World world({ 0.0f, -10.0f }, 10);

    Body a, b;
    a.set({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.set({ 0.5f, 0.5f }, 1.0f);
    b.position = { 0.0f, 0.0f }; // overlapping - forces a contact

    world.add(&a);
    world.add(&b);
    world.step(1.0f / 60.0f); // produces at least one arbiter
    REQUIRE(!world.arbiters.empty());

    world.clear();

    CHECK(world.bodies.empty());
    CHECK(world.joints.empty());
    CHECK(world.arbiters.empty());
}

TEST_CASE_FIXTURE(World_Fixture,
    "world.cc: World::step applies gravity to dynamic body")
{
    float vy_before = box.velocity.y;
    world.step(1.0f / 60.0f);
    // gravity is -10 m/s^2; after one step velocity.y must decrease
    CHECK(box.velocity.y < vy_before);
}

TEST_CASE_FIXTURE(World_Fixture,
    "world.cc: World::step clears force after integration")
{
    box.add_force({ 5.0f, 0.0f });
    world.step(1.0f / 60.0f);
    CHECK(box.force.x == 0.0f);
    CHECK(box.force.y == 0.0f);
}

TEST_CASE_FIXTURE(World_Fixture,
    "world.cc: World::step clears torque after integration")
{
    box.torque = 2.0f;
    world.step(1.0f / 60.0f);
    CHECK(box.torque == 0.0f);
}

TEST_CASE_FIXTURE(World_Fixture,
    "world.cc: World::step add_force changes velocity proportionally")
{
    // With no other forces or contacts, a horizontal push should
    // change velocity.x by F * inv_mass * dt
    box.position = { 0.0f, 100.0f }; // far from the floor - no contacts
    box.add_force({ 10.0f, 0.0f });

    float expected_dvx = 10.0f * box.inv_mass * (1.0f / 60.0f);
    world.step(1.0f / 60.0f);

    // velocity.x should have increased by roughly expected_dvx
    // (gravity only acts in y, so x is clean)
    CHECK(box.velocity.x == doctest::Approx(expected_dvx).epsilon(1e-5f));
}

TEST_CASE_FIXTURE(World_Fixture,
    "world.cc: World::step updates position by velocity * dt")
{
    // Give the box a known horizontal velocity; check position shifts
    box.position = { 0.0f, 100.0f }; // far from floor - no contacts
    box.velocity = { 6.0f, 0.0f };
    float x_before = box.position.x;
    world.step(1.0f / 60.0f);
    // position.x += velocity.x * dt = 6 / 60 = 0.1
    CHECK(box.position.x ==
          doctest::Approx(x_before + 6.0f / 60.0f).epsilon(1e-5f));
}

TEST_CASE_FIXTURE(World_Fixture,
    "world.cc: World::step does not move static body")
{
    Vec2 pos_before = floor.position;
    world.step(1.0f / 60.0f);
    CHECK(floor.position.x == pos_before.x);
    CHECK(floor.position.y == pos_before.y);
    CHECK(floor.velocity.x == 0.0f);
    CHECK(floor.velocity.y == 0.0f);
}

TEST_CASE("world.cc: World::broad_phase produces arbiter for "
          "overlapping bodies")
{
    World world({ 0.0f, -10.0f }, 10);

    // place two dynamic boxes on top of each other
    Body a, b;
    a.set({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.set({ 0.5f, 0.5f }, 1.0f);
    b.position = { 0.0f, 0.0f };

    world.add(&a);
    world.add(&b);
    world.broad_phase();

    CHECK(!world.arbiters.empty());
}

TEST_CASE("world.cc: World::broad_phase skips two static bodies")
{
    World world({ 0.0f, -10.0f }, 10);

    // both bodies are default-constructed (static - inv_mass = 0)
    Body wall, floor;
    wall.width = { 0.5f, 5.0f };
    wall.position = { 0.0f, 0.0f };
    floor.width = { 5.0f, 0.5f };
    floor.position = { 0.0f, 0.0f }; // overlapping wall

    world.add(&wall);
    world.add(&floor);
    world.broad_phase();

    CHECK(world.arbiters.empty());
}

TEST_CASE("world.cc: World::broad_phase removes arbiter when bodies separate")
{
    World world({ 0.0f, 0.0f }, 10); // no gravity

    Body a, b;
    a.set({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.set({ 0.5f, 0.5f }, 1.0f);
    b.position = { 0.5f, 0.0f }; // overlapping

    world.add(&a);
    world.add(&b);
    world.broad_phase();
    REQUIRE(!world.arbiters.empty());

    // move b far away
    b.position = { 100.0f, 0.0f };
    world.broad_phase();

    CHECK(world.arbiters.empty());
}

TEST_CASE("world.cc: World::step produces arbiters when bodies collide")
{
    // drop a dynamic box directly onto a static floor that it is already
    // touching so contact is detected on the first step
    World world({ 0.0f, -10.0f }, 10);

    Body floor;
    floor.width = { 10.0f, 0.5f };
    floor.position = { 0.0f,
        -0.4f }; // h_y=0.25 → top at -0.15; box bottom at -0.25 → 0.1 overlap

    Body box;
    box.set({ 0.5f, 0.5f }, 1.0f);
    box.position = { 0.0f, 0.0f }; // resting on the floor

    world.add(&floor);
    world.add(&box);
    world.step(1.0f / 60.0f);

    CHECK(!world.arbiters.empty());
}

TEST_CASE("world.cc: World::step - dynamic box does not sink through "
          "static floor")
{
    // Run the simulation long enough for a falling box to land and settle.
    // The box's final y position should be above the floor surface.
    World world({ 0.0f, -10.0f }, 10);

    Body floor;
    floor.width = { 10.0f, 0.5f };
    floor.position = { 0.0f, -5.0f };

    Body box;
    box.set({ 0.5f, 0.5f }, 1.0f);
    box.position = { 0.0f, 0.0f };

    world.add(&floor);
    world.add(&box);

    for (int i = 0; i < 120; ++i)
        world.step(1.0f / 60.0f);

    // floor top = -5.0 + 0.5*0.5 = -4.75   (width is full dim, h = width/2)
    // box  bottom = box.position.y - 0.5*0.5
    float box_bottom = box.position.y - 0.25f;
    float floor_top = floor.position.y + 0.25f;
    CHECK(box_bottom >=
          floor_top - 0.1f); // 0.1 tolerance for allowed penetration
}
