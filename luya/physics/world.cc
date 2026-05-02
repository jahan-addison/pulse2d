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

#include <luya/physics/world.h>

#include <etl/map.h>              // for map, operator!=, operator==
#include <etl/utility.h>          // for pair
#include <etl/vector.h>           // for vector
#include <luya/physics/arbiter.h> // for Arbiter_Key, operator<, Arbiter
#include <luya/physics/body.h>    // for Body
#include <luya/physics/joint.h>   // for Joint
#include <luya/physics/math.h>    // for Vec2, operator*, operator+

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
 *   physics::World world({ 0.0f, -10.0f }, 10); // gravity down, 10 iters
 *
 *   physics::Body floor;
 *   floor.position = { 0.0f, -4.0f };   // static; inv_mass = 0
 *
 *   physics::Body box;
 *   box.set({ 0.5f, 0.5f }, 1.0f);
 *   box.position = { 0.0f, 2.0f };
 *
 *   world.add(&floor);
 *   world.add(&box);
 *   world.step(1.0f / 60.0f);
 *
 *****************************************************************************/

namespace luya::physics {

typedef etl::map<Arbiter_Key, Arbiter, MAX_PHYSICS_BODIES>::iterator ArbIter;
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
 * Check every pair of bodies for overlap and rebuild the contact list.
 * Called automatically at the start of step() - you should not call this
 * manually.
 *
 * This is an O(n^2) broad-phase: every body is tested against every other
 * body. For small body counts (roughly < 50) this is fast enough on both
 * the Teensy and the host. The algorithm for each pair (bi, bj):
 *
 *   1. Skip the pair if both bodies are static (inv_mass == 0 for both).
 *      Two immovable objects can never usefully collide.
 *
 *   2. Run collide() to find contact points between the two boxes.
 *
 *   3. If contacts were found, either insert a new Arbiter into
 *      world.arbiters, or call update() on the existing one to carry over
 *      accumulated impulses from the previous frame (warm-starting).
 *
 *   4. If no contacts were found this frame but an Arbiter existed from
 *      the previous frame, erase it - the bodies have separated.
 *
 * clang-format off
 *
 *   bodies: [ A ][ B ][ C ][ D ]
 *
 *   pairs:  (A,B) (A,C) (A,D)
 *                 (B,C) (B,D)
 *                       (C,D)
 *
 * clang-format on
 *
 * After broad_phase() returns, world.arbiters holds exactly one entry per
 * currently touching pair. This is the contact data step() hands to the
 * constraint solver.
 */
void World::broad_phase()
{
    // O(n^2) broad-phase
    for (int i = 0; i < (int)bodies.size(); ++i) {
        Body* bi = bodies[i];

        for (int j = i + 1; j < (int)bodies.size(); ++j) {
            Body* bj = bodies[j];

            if (bi->inv_mass == 0.0f && bj->inv_mass == 0.0f)
                continue;

            Arbiter new_arb(bi, bj);
            Arbiter_Key key(bi, bj);

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

    // Perform pre-steps.
    for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb) {
        arb->second.pre_step(inv_dt);
    }

    for (int i = 0; i < (int)joints.size(); ++i) {
        joints[i]->pre_step(inv_dt);
    }

    // Perform iterations
    for (int i = 0; i < iterations; ++i) {
        for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb) {
            arb->second.apply_impulse();
        }

        for (int j = 0; j < (int)joints.size(); ++j) {
            joints[j]->apply_impulse();
        }
    }

    // Integrate Velocities
    for (int i = 0; i < (int)bodies.size(); ++i) {
        Body* b = bodies[i];

        b->position += dt * b->velocity;
        b->rotation += dt * b->angular_velocity;

        b->force.set(0.0f, 0.0f);
        b->torque = 0.0f;
    }
}

} // namespace physics