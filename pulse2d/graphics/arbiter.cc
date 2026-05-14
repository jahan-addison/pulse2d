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

#include <pulse2d/graphics/arbiter.h>

#include <math.h>                  // for sqrtf
#include <pulse2d/graphics/body.h> // for Body
#include <pulse2d/graphics/math.h> // for Vec2, cross, dot, operator*, operator-
#include <pulse2d/graphics/world.h> // for World

/****************************************************************************
 * Arbiter
 *
 * When two bodies overlap, the engine creates an Arbiter to track the
 * contact between them. It stores up to two contact points - each with
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

namespace pulse2d::graphics {

/**
 * @brief
 * Find the initial contact points between b1 and b2 and compute the combined
 * surface friction for this pair.
 *
 * The bodies are sorted by pointer address so that the pair (b1, b2) and the
 * pair (b2, b1) always produce an Arbiter with the same internal body order.
 * This guarantees that the Arbiter_Key used to look up the arbiter in
 * world.arbiters is consistent regardless of which order broad_phase()
 * encounters them on any given frame.
 *
 * collide() is called to fill the contacts[] array with up to two contact
 * points. Each point carries a world-space position, a normal pointing from
 * body1 toward body2, and a separation depth (negative means overlapping).
 *
 * Friction is computed as the geometric mean of both bodies' coefficients:
 *
 *   friction = sqrt(body1->friction * body2->friction)
 *
 * The geometric mean keeps the result between the two individual values.
 * For example, ice (0.05) against rubber (0.9) gives sqrt(0.045) ~= 0.21,
 * which feels more physically correct than the arithmetic mean 0.475.
 */
Arbiter::Arbiter(Body* b1, Body* b2)
{
    if (b1 < b2) {
        body1 = b1;
        body2 = b2;
    } else {
        body1 = b2;
        body2 = b1;
    }

    num_contacts = collide(contacts, body1, body2);

    friction = sqrtf(body1->friction * body2->friction);
}

/**
 * @brief
 * Merge a fresh set of contact points with the previous frame's, carrying
 * over accumulated impulses so the solver can warm-start from a good guess.
 *
 * Each contact point carries a "feature" - a compact label that describes
 * which pair of edges or vertices produced it. If a new contact has the same
 * feature as an old one, the engine treats them as the same contact and
 * copies the old impulses (pn, pt, pnb) onto the new point. This is
 * warm-starting: instead of solving from zero every frame, the solver begins
 * with last frame's answer and refines it. Stacked objects stay stable and
 * the solver converges much faster.
 *
 * If a contact has no matching old feature it is brand new, and its impulses
 * start at zero.
 *
 * clang-format off
 *
 *   Frame N   contacts: [ C0 (feature=E1) ][ C1 (feature=E3) ]
 *   Frame N+1 contacts: [ C0 (feature=E1) ][ C1 (feature=E2) ]
 *
 *   Result:
 *     C0 -> matched, carry pn/pt/pnb from old C0
 *     C1 -> new feature, start from pn = pt = pnb = 0
 *
 * clang-format on
 *
 * If World::warm_starting is false all impulses are zeroed and the solver
 * restarts cold every frame - useful for debugging but noticeably less
 * stable on resting contacts.
 */
void Arbiter::update(const Contacts& new_contacts, int num_new_contacts)
{
    Contacts merged_contacts;

    for (int i = 0; i < num_new_contacts; ++i) {
        const Contact& c_new = new_contacts[i];
        int k = -1;
        for (int j = 0; j < num_contacts; ++j) {
            const Contact& c_old = contacts[j];
            if (c_new.feature.value == c_old.feature.value) {
                k = j;
                break;
            }
        }

        if (k > -1) {
            Contact& c = merged_contacts[i];
            const Contact& c_old = contacts[k];
            c = c_new;
            if (World::warm_starting) {
                c.pn = c_old.pn;
                c.pt = c_old.pt;
                c.pnb = c_old.pnb;
            } else {
                c.pn = 0.0f;
                c.pt = 0.0f;
                c.pnb = 0.0f;
            }
        } else {
            merged_contacts[i] = new_contacts[i];
        }
    }

    for (int i = 0; i < num_new_contacts; ++i)
        contacts[i] = merged_contacts[i];

    num_contacts = num_new_contacts;
}

/**
 * @brief
 * Compute the effective mass and position-correction bias for each contact
 * point, then optionally warm-start by re-applying the previous frame's
 * accumulated impulse.
 *
 * For each contact, two effective masses are precomputed so apply_impulse()
 * only needs a multiply rather than a full solve on every iteration:
 *
 *   mass_normal  - resistance to being pushed apart along the contact normal.
 *                  It is 1 / (1/m1 + 1/m2 + rotational contributions).
 *
 *   mass_tangent - same idea but along the tangent (friction) direction.
 *
 * The rotational contribution captures the lever-arm effect: a force applied
 * far from a body's center creates more angular acceleration than one applied
 * near it. For the normal direction:
 *
 *   k_normal += inv_i * (|r|^2 - (r . n)^2)
 *
 * The bias is the target separation velocity that gently pushes overlapping
 * bodies apart each step. A small allowed penetration (0.01 units) is
 * subtracted first to prevent jitter on resting contacts - without it,
 * objects sitting on a surface would vibrate as the solver alternately
 * pushes and releases them:
 *
 *   bias = -k_bias_factor * inv_dt * min(0, separation + 0.01)
 *
 * Warm-starting: if World::accumulate_impulses is true, the accumulated
 * normal and friction impulses from the previous step are applied to both
 * bodies before the iteration loop begins.
 */
void Arbiter::pre_step(float inv_dt)
{
    const float k_allowed_penetration = 0.01f;
    float k_bias_factor = World::position_correction ? 0.2f : 0.0f;

    for (int i = 0; i < num_contacts; ++i) {
        Contact& c = contacts[i];

        Vec2 r1 = c.position - body1->position;
        Vec2 r2 = c.position - body2->position;

        // Precompute normal mass, tangent mass, and bias.
        float rn1 = dot(r1, c.normal);
        float rn2 = dot(r2, c.normal);
        float k_normal = body1->inv_mass + body2->inv_mass;
        k_normal += body1->inv_i * (dot(r1, r1) - rn1 * rn1) +
                    body2->inv_i * (dot(r2, r2) - rn2 * rn2);
        c.mass_normal = 1.0f / k_normal;

        Vec2 tangent = cross(c.normal, 1.0f);
        float rt1 = dot(r1, tangent);
        float rt2 = dot(r2, tangent);
        float k_tangent = body1->inv_mass + body2->inv_mass;
        k_tangent += body1->inv_i * (dot(r1, r1) - rt1 * rt1) +
                     body2->inv_i * (dot(r2, r2) - rt2 * rt2);
        c.mass_tangent = 1.0f / k_tangent;

        c.bias = -k_bias_factor * inv_dt *
                 min_val(0.0f, c.separation + k_allowed_penetration);

        if (World::accumulate_impulses) {
            // Apply normal + friction impulse
            Vec2 P = c.pn * c.normal + c.pt * tangent;

            body1->velocity -= body1->inv_mass * P;
            body1->angular_velocity -= body1->inv_i * cross(r1, P);

            body2->velocity += body2->inv_mass * P;
            body2->angular_velocity += body2->inv_i * cross(r2, P);
        }
    }
}

/**
 * @brief
 * Compute and apply normal and friction impulses to both bodies for every
 * active contact point, preventing them from sinking into each other.
 *
 * Each call is one iteration of the sequential impulse solver. For each
 * contact point the method runs two passes - first the normal direction,
 * then the tangent (friction) direction.
 *
 * Normal impulse pass:
 *
 *   1. Compute the relative velocity at the contact point:
 *        dv = v2 + w2 x r2 - v1 - w1 x r1
 *
 *   2. Project it onto the contact normal to get the closing speed vn.
 *
 *   3. Compute the impulse magnitude that zeroes out vn, adding the bias
 *      velocity to push overlapping bodies apart:
 *        d_pn = mass_normal * (-vn + bias)
 *
 *   4. If accumulate_impulses is on, clamp the total accumulated normal
 *      impulse pn to >= 0. Otherwise clamp just d_pn. The clamp is what
 *      prevents the solver from pulling bodies toward each other on
 *      separating contacts - bodies can only push, never pull.
 *
 * Friction impulse pass:
 *
 *   1. Re-compute dv after the normal impulse has been applied.
 *
 *   2. Project onto the tangent direction to get the sliding speed vt.
 *
 *   3. Compute d_pt = mass_tangent * (-vt).
 *
 *   4. Clamp the accumulated tangent impulse pt to the friction cone:
 *        |pt| <= friction * pn
 *      This is Coulomb's law: the sideways force cannot exceed a fixed
 *      multiple of the normal force. Surfaces with higher friction values
 *      allow a larger tangential impulse before the bodies slide.
 *
 * Both impulses are applied symmetrically: body1 in the negative direction,
 * body2 in the positive direction, scaled by inverse mass and inverse
 * inertia so heavier or harder-to-spin bodies change speed less.
 */
void Arbiter::apply_impulse()
{
    Body* b1 = body1;
    Body* b2 = body2;

    for (int i = 0; i < num_contacts; ++i) {
        Contact& c = contacts[i];
        c.r1 = c.position - b1->position;
        c.r2 = c.position - b2->position;

        // Relative velocity at contact
        Vec2 dv = b2->velocity + cross(b2->angular_velocity, c.r2) -
                  b1->velocity - cross(b1->angular_velocity, c.r1);

        // Compute normal impulse
        float vn = dot(dv, c.normal);

        float d_pn = c.mass_normal * (-vn + c.bias);

        if (World::accumulate_impulses) {
            // clamp the accumulated impulse
            float pn0 = c.pn;
            c.pn = max_val(pn0 + d_pn, 0.0f);
            d_pn = c.pn - pn0;
        } else {
            d_pn = max_val(d_pn, 0.0f);
        }

        // Apply contact impulse
        Vec2 pn = d_pn * c.normal;

        b1->velocity -= b1->inv_mass * pn;
        b1->angular_velocity -= b1->inv_i * cross(c.r1, pn);

        b2->velocity += b2->inv_mass * pn;
        b2->angular_velocity += b2->inv_i * cross(c.r2, pn);

        // Relative velocity at contact
        dv = b2->velocity + cross(b2->angular_velocity, c.r2) - b1->velocity -
             cross(b1->angular_velocity, c.r1);

        Vec2 tangent = cross(c.normal, 1.0f);
        float vt = dot(dv, tangent);
        float d_pt = c.mass_tangent * (-vt);

        if (World::accumulate_impulses) {
            // Compute friction impulse
            float max_pt = friction * c.pn;

            // clamp friction
            float old_tangent_impulse = c.pt;
            c.pt = clamp(old_tangent_impulse + d_pt, -max_pt, max_pt);
            d_pt = c.pt - old_tangent_impulse;
        } else {
            float max_pt = friction * d_pn;
            d_pt = clamp(d_pt, -max_pt, max_pt);
        }

        // Apply contact impulse
        Vec2 pt = d_pt * tangent;

        b1->velocity -= b1->inv_mass * pt;
        b1->angular_velocity -= b1->inv_i * cross(c.r1, pt);

        b2->velocity += b2->inv_mass * pt;
        b2->angular_velocity += b2->inv_i * cross(c.r2, pt);
    }
}

} // namespace pulse2d