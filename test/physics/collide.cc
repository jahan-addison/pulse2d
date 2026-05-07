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

#include <luya/physics/arbiter.h>
#include <luya/physics/body.h>
#include <luya/physics/math.h>

using namespace luya::physics;

TEST_CASE("collide.cc: collide returns 0 for clearly separated boxes")
{
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 5.0f, 0.0f }; // 5 units apart; extents are 0.5 each

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    CHECK(n == 0);
}

TEST_CASE("collide.cc: collide returns 0 for boxes separated in y")
{
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.0f, 10.0f };

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    CHECK(n == 0);
}

TEST_CASE("collide.cc: collide returns contacts for overlapping boxes")
{
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.3f, 0.0f }; // centers 0.3 apart; h=0.25 each → 0.2 overlap

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    CHECK(n > 0);
}

TEST_CASE("collide.cc: collide returns contacts for vertically "
          "overlapping boxes")
{
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.0f, 0.3f }; // centers 0.3 apart; h=0.25 each → 0.2 overlap

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    CHECK(n > 0);
}

TEST_CASE("collide.cc: collide contact count is at most 2")
{
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.5f, 0.0f };

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    CHECK(n <= 2);
}

TEST_CASE("collide.cc: collide separation is negative when boxes overlap")
{
    // separation < 0 means the boxes are penetrating each other
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.3f, 0.0f }; // h=0.25 each → 0.2 overlap

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    REQUIRE(n > 0);

    for (int i = 0; i < n; ++i)
        CHECK(contacts[i].separation < 0.0f);
}

TEST_CASE("collide.cc: collide separation is proportional to overlap depth")
{
    // more overlap should produce a more negative separation value
    Body a, b1, b2;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b1.set_mass({ 0.5f, 0.5f }, 1.0f);
    b2.set_mass({ 0.5f, 0.5f }, 1.0f);

    a.position = { 0.0f, 0.0f };
    b1.position = { 0.4f, 0.0f }; // h=0.25 each → 0.1 overlap
    b2.position = { 0.1f, 0.0f }; // h=0.25 each → 0.4 overlap (deeper)

    Arbiter::Contacts c1, c2;
    int n1 = collide(c1, &a, &b1);
    int n2 = collide(c2, &a, &b2);

    REQUIRE(n1 > 0);
    REQUIRE(n2 > 0);

    // b2 overlaps more deeply so its separation should be more negative
    CHECK(c2[0].separation < c1[0].separation);
}

TEST_CASE("collide.cc: collide normal points from A toward B (x axis)")
{
    // B is to the right of A - normal should point in +x
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.4f, 0.0f }; // h=0.25 each → 0.1 overlap; B is to the right

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    REQUIRE(n > 0);

    CHECK(contacts[0].normal.x > 0.0f);
    CHECK(contacts[0].normal.y == doctest::Approx(0.0f).epsilon(1e-5f));
}

TEST_CASE("collide.cc: collide normal points from A toward B (y axis)")
{
    // B is above A - normal should point in +y
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.0f, 0.4f }; // h=0.25 each → 0.1 overlap; B is above

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    REQUIRE(n > 0);

    CHECK(contacts[0].normal.x == doctest::Approx(0.0f).epsilon(1e-5f));
    CHECK(contacts[0].normal.y > 0.0f);
}

TEST_CASE("collide.cc: collide contact normal is a unit vector")
{
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.3f, 0.0f }; // h=0.25 each → 0.2 overlap

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    REQUIRE(n > 0);

    for (int i = 0; i < n; ++i)
        CHECK(contacts[i].normal.length() ==
              doctest::Approx(1.0f).epsilon(1e-5f));
}

TEST_CASE("collide.cc: collide produces two contacts for wide "
          "face-on-face overlap")
{
    // Two wide boxes overlapping along the y axis - both corners of the
    // incident edge are within the reference face, so both contact points
    // should survive the clip.
    Body a, b;
    a.set_mass({ 2.0f, 0.5f }, 1.0f);
    b.set_mass({ 2.0f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.0f,
        0.4f }; // h_y=0.25 each → 0.1 overlap; wide face → 2 contacts

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    CHECK(n == 2);
}

TEST_CASE("collide.cc: collide works when one box is rotated")
{
    // rotate b by 45 degrees; it should still overlap if close enough
    Body a, b;
    a.set_mass({ 0.5f, 0.5f }, 1.0f);
    b.set_mass({ 0.5f, 0.5f }, 1.0f);
    a.position = { 0.0f, 0.0f };
    b.position = { 0.5f, 0.0f };
    b.rotation = k_pi / 4.0f;

    Arbiter::Contacts contacts;
    int n = collide(contacts, &a, &b);
    // may or may not overlap at this distance - just must not crash
    CHECK(n >= 0);
    CHECK(n <= 2);
}
