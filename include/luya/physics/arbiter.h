/*********************************************************
 * box2d-lite - Heavily modified for ETL and Teensy 4.1
 *********************************************************/

/*
 * Copyright (c) 2006-2009 Erin Catto http://www.gphysics.com
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erin Catto makes no representations about the suitability
 * of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#pragma once

#include "math.h"
#include <etl/array.h> // for array

/****************************************************************************
 * Arbiter
 *
 * When two bodies overlap, the engine creates an Arbiter to track the
 * contact between them. It stores up to two contact points — each with
 * a position, a direction (normal) pointing from one body to the other,
 * and the impulses needed to push them apart.
 *
 * Arbiters are created and updated automatically by World::broad_phase()
 * each step. They are stored in World::arbiters, which is non-empty
 * whenever at least one pair of bodies is touching:
 *
 *   if (!world.arbiters.empty()) {
 *       // at least one collision is active this frame
 *   }
 *
 * You do not need to create or manage Arbiters yourself.
 *
 *****************************************************************************/

namespace luya::physics {

class Body;

/**
 * @brief
 * Packed label identifying which pair of box edges produced a contact point.
 *
 * Four edge indices (in/out on each body) are packed into a single int for
 * fast equality comparison. When the same feature value appears on a contact
 * in two consecutive frames, Arbiter::update() treats it as the same physical
 * contact and carries the accumulated impulses forward for warm-starting.
 * A changed feature means the contact geometry shifted and impulses reset.
 */
union Feature_Pair
{
    struct Edges
    {
        char in_edge1;
        char out_edge1;
        char in_edge2;
        char out_edge2;
    } e;
    int value;
};

/**
 * @brief
 * One contact point between two overlapping bodies.
 *
 * Carries both geometric data — position, normal, r1, r2, separation —
 * and solver data — mass_normal, mass_tangent, bias, pn, pt, pnb.
 *
 * pn, pt, and pnb are the accumulated impulses across solver iterations
 * within a frame. Arbiter::update() preserves them across frames so that
 * pre_step() can warm-start the solver by re-applying them before the first
 * iteration. The larger these values, the closer the solver's starting
 * estimate is to the converged answer, and the fewer iterations it needs
 * to resolve the contact cleanly.
 */
struct Contact
{
    Contact()
        : pn(0.0f)
        , pt(0.0f)
        , pnb(0.0f)
        , separation(0.0f)
        , mass_normal(0.0f)
        , mass_tangent(0.0f)
        , bias(0.0f)
        , feature{ .value = 0 }
    {
    }

    float
        pn; // total push-apart impulse applied along the contact normal so far
    float pt;  // total sideways (friction) impulse applied so far
    float pnb; // extra normal impulse used to correct position overlap

    float separation; // how far the two bodies are overlapping (negative =
                      // inside)
    float
        mass_normal; // how hard it is to push the bodies apart along the normal
    float mass_tangent; // how hard it is to slide the bodies past each other
    float bias;         // target velocity to resolve the overlap this step

    Feature_Pair feature;

    Vec2 position{}; // world-space position of the contact point
    Vec2 normal{};   // direction from body1 toward body2 at the contact
    Vec2 r1{}, r2{}; // offset from each body's center to the contact point
};

/**
 * @brief
 * Canonical lookup key for the arbiter map.
 *
 * The two body pointers are sorted on construction (smaller address first)
 * so the pair (A, B) and the pair (B, A) always hash to the same key.
 * This guarantees that broad_phase() looks up and updates the same Arbiter
 * regardless of the order in which the two bodies appear in the body list.
 * operator< enables the key to be used inside etl::map.
 */
struct Arbiter_Key
{
    Arbiter_Key(Body* b1, Body* b2)
    {
        if (b1 < b2) {
            body1 = b1;
            body2 = b2;
        } else {
            body1 = b2;
            body2 = b1;
        }
    }

    Body* body1;
    Body* body2;
};

/**
 * @brief
 * Contact manifold between two overlapping bodies. Holds up to two contact
 * points and is cached in World::arbiters across frames.
 *
 * Each frame broad_phase() calls collide() to find fresh contact geometry,
 * then calls update() to match new points to old ones by feature label. Old
 * impulses (pn, pt, pnb) are copied onto matched points so the solver can
 * warm-start from last frame's answer rather than solving from zero.
 *
 * The iteration loop in step() calls pre_step() once to compute effective
 * masses and biases, then calls apply_impulse() for each iteration. Two
 * constraints are enforced per contact point:
 *
 * clang-format off
 *
 *   Normal:   pn >= 0           bodies can only push, never pull
 *   Friction: |pt| <= friction * pn   Coulomb friction cone
 *
 * clang-format on
 *
 * You never construct an Arbiter directly. World::broad_phase() manages the
 * full lifetime — creation, update, and removal when bodies separate.
 */
struct Arbiter
{
    constexpr static std::size_t MAX_POINTS = 2;

    using Contacts = etl::array<Contact, MAX_POINTS>;

    Arbiter(Body* b1, Body* b2);

    void update(const Contacts& new_contacts, int num_new_contacts);

    void pre_step(float inv_dt);
    void apply_impulse();

    Contacts contacts; // up to two contact points this step
    int num_contacts;  // how many contact points are active (0, 1, or 2)

    Body* body1;
    Body* body2;

    float friction; // geometric mean of both bodies' friction values
};

// This is used by std::set
inline bool operator<(const Arbiter_Key& a1, const Arbiter_Key& a2)
{
    if (a1.body1 < a2.body1)
        return true;

    if (a1.body1 == a2.body1 && a1.body2 < a2.body2)
        return true;

    return false;
}

int collide(Arbiter::Contacts& contacts, Body const* bodyA, Body const* bodyB);
} // namespace physics