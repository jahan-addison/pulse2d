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

#include "arbiter.h"    // for Arbiter_Key, operator<
#include "math.h"       // for Vec2
#include <cstddef>      // for size_t
#include <etl/map.h>    // for map
#include <etl/vector.h> // for vector

#ifndef MAX_PHYSICS_BODIES
#define MAX_PHYSICS_BODIES 256
#endif

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

class Body;
class Joint;

/**
 * @brief
 * The simulation context. Stores all bodies, joints, and active contact
 * arbiters, and advances the entire scene by one timestep each frame.
 *
 * step() runs five ordered stages:
 *
 * clang-format off
 *
 *   1. broad_phase()       O(n^2) overlap test; builds the arbiter map
 *   2. integrate forces    gravity + user forces applied to each velocity
 *   3. pre_step()          effective masses and biases computed; warm-start
 *   4. apply_impulse()     sequential impulse solve, repeated `iterations`
 * times
 *   5. integrate velocities positions and rotations updated; forces cleared
 *
 * clang-format on
 *
 * Stage 4 is the heart of the solver. Each pass applies a small velocity
 * correction to every contact and joint to reduce constraint error. More
 * passes make stacked objects more stable at higher CPU cost. 10 iterations
 * is a practical default on the Teensy 4.1 and the host alike.
 *
 * After broad_phase() the arbiters map holds exactly one Arbiter per
 * touching body pair. Checking arbiters.empty() is the correct way to
 * detect whether any collision is active this frame. The three static flags
 * control solver optimizations - disable them individually to isolate bugs.
 */
class World
{
  public:
    World(Vec2 gravity, int iterations)
        : gravity(gravity)
        , iterations(iterations)
    {
    }

  public:
    void add(Body* body);
    void add(Joint* joint);
    void clear();
    void step(float dt);
    void broad_phase();

  public:
    /**
     * @brief
     * All bodies registered with add(Body*).
     *
     * step() iterates this vector every frame. Order does not matter.
     * Do not remove entries manually - use clear() to reset the world.
     */
    etl::vector<Body*, MAX_PHYSICS_BODIES> bodies;

    /**
     * @brief
     * All joints registered with add(Joint*).
     *
     * step() runs pre_step() and apply_impulse() on every joint each frame,
     * after contacts. Do not remove entries manually - use clear().
     */
    etl::vector<Joint*, MAX_PHYSICS_BODIES> joints;

    /**
     * @brief
     * Active contact arbiters for this step, keyed by body pair.
     *
     * broad_phase() (called from step()) populates this map: one Arbiter
     * per touching pair, removed when the pair separates. Check emptiness
     * to detect any collision this frame:
     *
     *   if (!world.arbiters.empty()) { ... }
     *
     * Iterate to inspect individual contacts:
     *
     *   for (auto& [key, arb] : world.arbiters)
     *       // key.body1, key.body2 - the two bodies
     *       // arb.num_contacts     - 1 or 2
     *       // arb.contacts[i].normal - push direction
     */
    etl::map<Arbiter_Key, Arbiter, MAX_PHYSICS_BODIES> arbiters;

    /**
     * @brief
     * Gravitational acceleration applied to every dynamic body per
     * step.
     *
     * Integrated into velocity once per step before the constraint solver:
     *
     *   velocity += dt * gravity
     *
     * Standard Earth gravity pointing down: { 0.0f, -10.0f }.
     * Set to { 0.0f, 0.0f } for top-down or zero-gravity scenes.
     */
    Vec2 gravity;

    /**
     * @brief
     * Number of constraint solver passes per step.
     *
     * Each pass visits every contact and joint once, applying a small
     * corrective velocity. More passes reduce residual penetration and
     * make stacked objects more stable, at proportionally higher CPU cost.
     * 10 is a practical default on both the Teensy 4.1 and the host.
     */
    int iterations;

    /**
     * @brief
     * Accumulate impulses across solver passes within a step.
     *
     * When true (default) each pass adds to the impulse accumulated so far
     * rather than recomputing from scratch. This significantly improves
     * stability for stacked objects. Disable to isolate solver bugs.
     */
    static bool accumulate_impulses;

    /**
     * @brief
     * Seed the solver with impulses carried over from the last step.
     *
     * When true (default) the solver starts each step with the contact
     * impulses computed last frame. This reduces the number of passes needed
     * to reach a stable solution (warm-starting). Disable to observe cold
     * convergence behaviour.
     */
    static bool warm_starting;

    /**
     * @brief
     * Apply a position correction to push overlapping bodies apart.
     *
     * When true (default) a small fraction of penetration depth is removed
     * directly from position each step (Baumgarte stabilization). Disable
     * if you want to observe purely velocity-based resolution.
     */
    static bool position_correction;
};

} // namespace physics