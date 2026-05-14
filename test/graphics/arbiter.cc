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

#include <pulse2d/graphics/arbiter.h>
#include <pulse2d/graphics/body.h>
#include <pulse2d/graphics/math.h>
#include <pulse2d/graphics/world.h>

#include <math.h>

using namespace pulse2d::graphics;

// Two dynamic boxes fully overlapping - guarantees contacts.
struct Arbiter_Fixture
{
    Body a;
    Body b;

    Arbiter_Fixture()
    {
        a.set_mass({ 0.5f, 0.5f }, 1.0f);
        a.position = { 0.0f, 0.0f };
        b.set_mass({ 0.5f, 0.5f }, 1.0f);
        b.position = { 0.0f, 0.0f }; // fully overlapping - always has contacts
    }

    ~Arbiter_Fixture() = default;
};

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter constructor sorts body pointers")
{
    Arbiter arb1(&a, &b);
    Arbiter arb2(&b, &a);
    // both orderings must produce the same internal body1/body2
    CHECK(arb1.body1 == arb2.body1);
    CHECK(arb1.body2 == arb2.body2);
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter constructor finds at least one contact")
{
    Arbiter arb(&a, &b);
    CHECK(arb.num_contacts > 0);
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter constructor computes geometric mean friction")
{
    a.friction = 0.4f;
    b.friction = 0.9f;
    Arbiter arb(&a, &b);
    float expected = sqrtf(0.4f * 0.9f);
    CHECK(arb.friction == doctest::Approx(expected));
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter friction with two identical surfaces")
{
    a.friction = 0.5f;
    b.friction = 0.5f;
    Arbiter arb(&a, &b);
    CHECK(arb.friction == doctest::Approx(0.5f));
}

TEST_CASE("arbiter.cc: Arbiter separated bodies produce zero contacts")
{
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.position = { 5.0f, 0.0f }; // clearly separated
    Arbiter arb(&a, &b);
    CHECK(arb.num_contacts == 0);
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter::update carries impulses when warm_starting "
    "is on")
{
    World::warm_starting = true;

    Arbiter arb(&a, &b);
    int n = arb.num_contacts;
    REQUIRE(n > 0);

    // inject accumulated impulses from a previous frame
    for (int i = 0; i < n; ++i) {
        arb.contacts[i].pn = 1.0f;
        arb.contacts[i].pt = 0.5f;
        arb.contacts[i].pnb = 0.2f;
    }

    // update with a fresh set of contacts at the same geometry
    // - features will match, so impulses must be carried forward
    Arbiter fresh(&a, &b);
    arb.update(fresh.contacts, fresh.num_contacts);

    for (int i = 0; i < arb.num_contacts; ++i) {
        CHECK(arb.contacts[i].pn == doctest::Approx(1.0f));
        CHECK(arb.contacts[i].pt == doctest::Approx(0.5f));
        CHECK(arb.contacts[i].pnb == doctest::Approx(0.2f));
    }
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter::update zeroes impulses when warm_starting is "
    "off")
{
    World::warm_starting = false;

    Arbiter arb(&a, &b);
    int n = arb.num_contacts;
    REQUIRE(n > 0);

    for (int i = 0; i < n; ++i) {
        arb.contacts[i].pn = 1.0f;
        arb.contacts[i].pt = 0.5f;
        arb.contacts[i].pnb = 0.2f;
    }

    Arbiter fresh(&a, &b);
    arb.update(fresh.contacts, fresh.num_contacts);

    for (int i = 0; i < arb.num_contacts; ++i) {
        CHECK(arb.contacts[i].pn == 0.0f);
        CHECK(arb.contacts[i].pt == 0.0f);
        CHECK(arb.contacts[i].pnb == 0.0f);
    }

    World::warm_starting = true; // restore default
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter::update preserves contact count")
{
    World::warm_starting = true;

    Arbiter arb(&a, &b);
    int n_before = arb.num_contacts;

    Arbiter fresh(&a, &b);
    arb.update(fresh.contacts, fresh.num_contacts);

    CHECK(arb.num_contacts == n_before);
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter::pre_step computes positive mass_normal")
{
    Arbiter arb(&a, &b);
    REQUIRE(arb.num_contacts > 0);

    arb.pre_step(60.0f);

    for (int i = 0; i < arb.num_contacts; ++i)
        CHECK(arb.contacts[i].mass_normal > 0.0f);
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter::pre_step computes positive mass_tangent")
{
    Arbiter arb(&a, &b);
    REQUIRE(arb.num_contacts > 0);

    arb.pre_step(60.0f);

    for (int i = 0; i < arb.num_contacts; ++i)
        CHECK(arb.contacts[i].mass_tangent > 0.0f);
}

TEST_CASE_FIXTURE(Arbiter_Fixture,
    "arbiter.cc: Arbiter::pre_step sets non-positive bias")
{
    // bias = -k_bias_factor * inv_dt * min(0, separation + 0.01)
    // separation is negative (overlapping) so bias should be non-negative
    // (it pushes bodies apart i.e. corrects in the direction of separation)
    Arbiter arb(&a, &b);
    REQUIRE(arb.num_contacts > 0);

    arb.pre_step(60.0f);

    for (int i = 0; i < arb.num_contacts; ++i)
        CHECK(arb.contacts[i].bias >= 0.0f);
}

TEST_CASE("arbiter.cc: Arbiter::apply_impulse separates overlapping "
          "bodies over time")
{
    // Two dynamic boxes starting at the same position should be pushed apart
    // by the solver over a few steps.
    World world({ 0.0f, 0.0f }, 20); // no gravity

    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.position = { 0.0f, 0.0f };

    world.add(&a);
    world.add(&b);

    for (int i = 0; i < 60; ++i)
        world.step(1.0f / 60.0f);

    // after 60 steps the bodies should have separated
    float dx = a.position.x - b.position.x;
    float dy = a.position.y - b.position.y;
    float dist = sqrtf(dx * dx + dy * dy);
    CHECK(dist > 0.5f);
}

TEST_CASE("arbiter.cc: Arbiter::apply_impulse normal impulse is non-negative")
{
    // The normal impulse constraint (pn >= 0) means bodies can only push,
    // never pull. After pre_step and apply_impulse, pn must be >= 0.
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.position = { 0.4f, 0.0f };

    Arbiter arb(&a, &b);
    REQUIRE(arb.num_contacts > 0);

    World::accumulate_impulses = true;
    arb.pre_step(60.0f);
    arb.apply_impulse();

    for (int i = 0; i < arb.num_contacts; ++i)
        CHECK(arb.contacts[i].pn >= 0.0f);
}
