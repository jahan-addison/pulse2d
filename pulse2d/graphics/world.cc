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
 * Copyright (c) 2006-2009 Erin Catto http://www.gphysics.com
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erin Catto makes no representations about the suitability
 * of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include "world.h"

#include "arbiter.h"     // for Arbiter_Key, operator<, Arbiter
#include "body.h"        // for Body
#include "joint.h"       // for Joint
#include "math.h"        // for Vec2, operator*, operator+
#include <algorithm>     // for range::contains
#include <cstddef>       // for size_t
#include <etl/array.h>   // for array
#include <etl/map.h>     // for map, operator!=, operator==
#include <etl/utility.h> // for pair
#include <etl/vector.h>  // for vector

/****************************************************************************
 * World
 *
 * The World holds every body and joint in the scene and runs the physics
 * each frame. You give it a gravity vector and a solver iteration count,
 * then register bodies with add(). Call step() once per frame with the
 * frame duration (e.g. 1/60 for 60 Hz) and the engine moves everything.
 *
 * Higher iteration counts make collisions feel more solid but cost more
 * CPU time. 10 is a reasonable starting value.
 *
 * world.arbiters is a map of every active contact pair this step. It is
 * non-empty whenever two bodies are touching. Use it to detect collisions:
 *
 *   if (!world.arbiters.empty()) { ... }
 *
 * Call clear() to remove all bodies and joints when changing scenes.
 *
 *  Example:
 *
 *   graphics::World world({ 0.0f, -10.0f }, 10); // gravity down, 10 iters
 *
 *   graphics::Body floor;
 *   floor.position = { 0.0f, -4.0f };   // static; inv_mass = 0
 *
 *   graphics::Body box;
 *   box.set_motion({ 0.5f, 0.5f }, 1.0f);
 *   box.position = { 0.0f, 2.0f };
 *
 *   world.add(&floor);
 *   world.add(&box);
 *   world.step(1.0f / 60.0f);
 *
 *****************************************************************************/

namespace pulse2d::graphics {

typedef etl::map<Arbiter_Key, Arbiter, MAX_ARBITER_PAIRS>::iterator ArbIter;
typedef etl::pair<Arbiter_Key, Arbiter> ArbPair;

bool World::accumulate_impulses = true;
bool World::warm_starting = true;
bool World::position_correction = true;

/**
 * @brief
 * Register a body with the world so it is included in every future step().
 *
 * Bodies are stored by pointer; the world does not own or copy them. You are
 * responsible for keeping the body alive for as long as it is registered.
 * The body will receive gravity, forces, and collision responses starting on
 * the very next call to step().
 *
 * A body must be added before step() is called for it to be seen by the
 * broad-phase. Adding a body mid-simulation is safe - it simply begins
 * participating from that frame onward.
 */
void World::add(Body* body)
{
    bodies.push_back(body);
}

/**
 * @brief
 * Register a joint constraint so it is enforced during every future step().
 *
 * Like bodies, joints are stored by pointer and not owned by the world.
 * Joints must have already had set() called on them before being added,
 * because set() converts the world-space anchor into per-body local offsets
 * that pre_step() reads every frame.
 *
 * Joints are solved in the same iteration loop as contact arbiters - their
 * impulses interleave with collision impulses, so a joint attached to a
 * body that is also in a contact will behave correctly.
 */
void World::add(Joint* joint)
{
    joints.push_back(joint);
}

/**
 * @brief
 * Remove all bodies, joints, and contact records from the world.
 *
 * This is the correct way to transition between scenes or levels. Simply
 * calling clear() and then re-adding new bodies and joints is safe and
 * cheap - ETL containers keep their backing storage, so no allocations
 * occur. The static solver flags (accumulate_impulses, warm_starting,
 * position_correction) are left unchanged.
 *
 * Note: clear() does not call any destructor on the stored pointers. If
 * your bodies or joints are heap-allocated, free them separately before
 * or after calling clear().
 */
void World::clear()
{
    bodies.clear();
    joints.clear();
    arbiters.clear();
}

/**
 * @brief
 * Remove a body by pointer in the physics world
 *
 * Useful for temporary bodies like projectiles, lasers, etc.
 */

void World::remove(Body* body)
{
    auto it = std::ranges::find(bodies, body);
    if (it != bodies.end()) {
        std::iter_swap(it, bodies.end() - 1);
        bodies.pop_back();
    }

    for (auto arb = arbiters.begin(); arb != arbiters.end();) {
        if (arb->first.body1 == body or arb->first.body2 == body)
            arb = arbiters.erase(arb);
        else
            ++arb;
    }
}

/**
 * @brief
 * Two-stage collision detection: AABB prefilter then narrow-phase SAT.
 * Called automatically at the start of step() - you should not call this
 * manually.
 *
 * Stage 1 - static skip + AABB prefilter (cheap):
 *
 *   Pure-static pairs (inv_mass == 0, not a sensor, on both bodies) are
 *   skipped at the top of the inner loop - walls never usefully collide with
 *   other walls. Per-body AABB half-extents are pre-computed once for all N
 *   bodies (N scalar ops; trig only for rotating bodies, static walls hit the
 *   rotation==0 fast path). Pairs that fail the AABB overlap test are
 *   rejected before reaching the SAT, but their arbiter is still erased so
 *   separated bodies stop receiving impulses from the previous contact.
 *
 * Stage 2 - narrow phase (only for AABB-passing pairs):
 *
 *   collide() runs the full SAT and writes up to two Contact points. If
 *   contacts are found, the pair's Arbiter is inserted or updated with
 *   warm-start data from the previous frame. If no contacts are found and
 *   an Arbiter existed, it is erased.
 *
 * clang-format off
 *
 *   bodies: [ S1(static) ][ S2(static) ][ D1(dynamic) ][ D2(dynamic) ]
 *
 *   active×active:  (D1,D2)
 *   active×static:  (D1,S1) (D1,S2) (D2,S1) (D2,S2)
 *   skipped:        (S1,S2)  ← pure-static × pure-static never tested
 *
 * clang-format on
 *
 * After broad_phase() returns, world.arbiters holds exactly one entry per
 * currently touching pair. This is the contact data step() hands to the
 * constraint solver.
 */
void World::broad_phase()
{
    using namespace math;

    // Pre-compute AABB half-extents for every body.
    // rotation == 0 (all static bodies, most dynamic ones) uses the fast
    // path - no trig. Rotated bodies expand their OBB into a conservative
    // AABB: half_x = hx|cosθ| + hy|sinθ|, half_y = hx|sinθ| + hy|cosθ|.
    // This is never a false negative; false positives fall through to SAT.
    etl::array<Vec2, MAX_PHYSICS_BODIES> extents;
    for (std::size_t k = 0; k < bodies.size(); ++k) {
        Body const* b = bodies[k];
        Vec2 h = 0.5f * b->width;
        if (b->rotation == 0.0f) {
            extents[k] = h;
        } else {
            float c = fabsf(cosf(b->rotation));
            float s = fabsf(sinf(b->rotation));
            extents[k] = { h.x * c + h.y * s, h.x * s + h.y * c };
        }
    }

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        Body* bi = bodies[i];
        bool i_pure_static =
            bi->inv_mass == 0.0f and not bi->is_sensor and not bi->is_kinematic;

        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            Body* bj = bodies[j];

            // Skip pure-static × pure-static pairs entirely.
            if (i_pure_static and bj->inv_mass == 0.0f and not bj->is_sensor)
                continue;

            Arbiter_Key key(bi, bj);

            // AABB prefilter: reject non-adjacent pairs before the SAT.
            // Must still erase any stale arbiter so separated bodies stop
            // receiving phantom impulses from the solver.
            float dx = fabsf(bj->position.x - bi->position.x);
            float dy = fabsf(bj->position.y - bi->position.y);
            if (dx > extents[i].x + extents[j].x or
                dy > extents[i].y + extents[j].y) {
                arbiters.erase(key);
                continue;
            }

            Arbiter new_arb(bi, bj);

            if (new_arb.num_contacts > 0) {
                ArbIter iter = arbiters.find(key);
                if (iter == arbiters.end()) {
                    arbiters.insert(ArbPair(key, new_arb));
                } else {
                    iter->second.update(new_arb.contacts, new_arb.num_contacts);
                }
            } else {
                arbiters.erase(key);
            }
        }
    }
}

/**
 * @brief
 * Advance the simulation by dt seconds through five ordered stages.
 *
 * Call this once per frame with a fixed timestep (e.g. 1.0f/60.0f for 60 Hz).
 * A fixed dt makes the simulation deterministic and stable. A variable dt
 * can cause objects to tunnel through thin walls at low frame rates.
 *
 * The five stages, in order:
 *
 * clang-format off
 *
 *  1. broad_phase()
 *       Detect which body pairs are overlapping and build or update the
 *       arbiter map. New contacts are inserted; separated pairs are removed.
 *
 *  2. Integrate forces
 *       For every dynamic body (inv_mass != 0), add gravity and any
 *       user-applied forces to velocity:
 *
 *         velocity         += dt * (gravity + inv_mass * force)
 *         angular_velocity += dt * inv_i   * torque
 *
 *  3. pre_step()  (arbiters + joints)
 *       Compute effective masses and position-correction biases. If
 *       warm_starting is true, seed each constraint with the impulse
 *       accumulated in the previous frame - this makes the solver
 *       converge faster and keeps stacked objects stable.
 *
 *  4. apply_impulse()  x iterations
 *       Run the constraint solver for `iterations` passes. Each pass
 *       applies a small velocity correction to every contact and joint.
 *       More passes = stiffer, more accurate collisions at higher CPU cost.
 *
 *  5. Integrate velocities into positions
 *       Move every body by its (now corrected) velocity:
 *
 *         position += dt * velocity
 *         rotation += dt * angular_velocity
 *
 *       Then clear force and torque so they do not accumulate across frames.
 *
 * clang-format on
 */
void World::step(float dt)
{
    float inv_dt = dt > 0.0f ? 1.0f / dt : 0.0f;

    // Determine overlapping bodies and update contact points.
    broad_phase();

    // Integrate forces.
    for (int i = 0; i < (int)bodies.size(); ++i) {
        Body* b = bodies[i];

        if (b->inv_mass == 0.0f)
            continue;

        b->velocity += dt * (gravity + b->inv_mass * b->force);
        b->angular_velocity += dt * b->inv_i * b->torque;
    }

    // Perform pre-steps (skip sensor pairs - apply_impulse skips them too,
    // and static+static sensor contacts would divide by zero in mass_normal).
    for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb) {
        if (arb->first.body1->is_sensor or arb->first.body2->is_sensor)
            continue;
        arb->second.pre_step(inv_dt);
    }

    for (int i = 0; i < (int)joints.size(); ++i) {
        joints[i]->pre_step(inv_dt);
    }

    // Perform iterations
    for (int i = 0; i < iterations; ++i) {
        for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb) {
            // Bypass collision solver on sensor bodies
            if (arb->first.body1->is_sensor or arb->first.body2->is_sensor)
                continue;
            arb->second.apply_impulse();
        }

        for (int j = 0; j < (int)joints.size(); ++j) {
            joints[j]->apply_impulse();
        }
    }

    // Integrate Velocities
    for (int i = 0; i < (int)bodies.size(); ++i) {
        Body* b = bodies[i];

        // Skip truly static bodies - their velocity is always zero so
        // integration would be a no-op, and skipping avoids touching their
        // position unnecessarily. Kinematic bodies (inv_mass==0 but with an
        // externally set velocity) must still integrate.
        if (b->inv_mass == 0.0f and not b->is_kinematic)
            continue;

        b->position += dt * b->velocity;
        b->rotation += dt * b->angular_velocity;

        b->force.set(0.0f, 0.0f);
        b->torque = 0.0f;
    }
}

} // namespace pulse2d