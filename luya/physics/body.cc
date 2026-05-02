/*********************************************************
 * box2d-lite - Heavily modified for ETL and Teensy 4.1
 *********************************************************/

/*
 * Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erin Catto makes no representations about the suitability
 * of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include <luya/physics/body.h>

#include <float.h>             // for FLT_MAX
#include <luya/physics/math.h> // for Vec2

/****************************************************************************
 * Body
 *
 * A body is a box that the physics engine can push around. It has a
 * position in world space, a speed (velocity), and a spin (angular
 * velocity). Each frame the engine applies gravity and any forces you
 * have added, then moves the body accordingly.
 *
 * Call set() to give the body a size and mass. The two values in set()
 * are the full dimensions of the box: a value of { 1.0, 1.0 } makes a 1x1
 * unit box, { 2.0, 0.5 } makes a 2x0.5 unit platform, and so on.
 * After set(), assign position to place it in the world, then pass a
 * pointer to World::add().
 *
 * A body constructed without calling set() has infinite mass and will
 * never move — use this for static walls and floors.
 *
 *  Example:
 *
 *   physics::Body box;
 *   box.set({ 1.0f, 1.0f }, 10.0f); // 1x1 unit box, 10 kg
 *   box.position = { 0.0f, 3.0f };  // start 3 units above origin
 *   world.add(&box);
 *
 *   // nudge it to the right before the first step
 *   box.add_force({ 5.0f, 0.0f });
 *
 ****************************************************************************/

namespace luya::physics {

/**
 * @brief
 * Zero-initialize every field and start the body in a static state
 * (infinite mass, inv_mass = 0, inv_i = 0).
 *
 * A body that has only been constructed — without calling set() — has
 * inv_mass = 0, which means the constraint solver treats it as immovable.
 * This is useful: you can create a static floor or wall simply by
 * constructing a Body, setting its position, and adding it to the world:
 *
 *   physics::Body floor;
 *   floor.position = { 0.0f, -5.0f }; // placed at y = -5, never moves
 *   world.add(&floor);
 *
 * Friction is initialized to 0.2 — a light surface grip, close to
 * wood-on-wood. Width defaults to { 1.0, 1.0 } as a placeholder; it
 * is overwritten the moment you call set().
 */
Body::Body()
{
    position.set(0.0f, 0.0f);
    rotation = 0.0f;
    velocity.set(0.0f, 0.0f);
    angular_velocity = 0.0f;
    force.set(0.0f, 0.0f);
    torque = 0.0f;
    friction = 0.2f;

    width.set(1.0f, 1.0f);
    mass = FLT_MAX;
    inv_mass = 0.0f;
    I = FLT_MAX;
    inv_i = 0.0f;
}

/**
 * @brief
 * Configure this body with a box size and mass, then derive all the
 * internal values the solver needs. Also resets all motion state —
 * position, velocity, forces — back to zero, so it is safe to call
 * set() more than once to reinitialize the body between levels.
 *
 * The parameter `w` is the full dimensions of the box. A value of
 * { 1.0, 1.0 } makes a 1x1 unit square; { 2.0, 0.5 } makes a 2x0.5
 * platform:
 *
 * clang-format off
 *
 *   w = { 1.0, 1.0 }       w = { 2.0, 0.5 }
 *   +----------+            +--------------------+
 *   |          |            |                    |
 *   |  1 x 1   |            |      2 x 0.5       |
 *   |          |            +--------------------+
 *   +----------+
 *    ^-- full width = w.x
 *
 * clang-format on
 *
 * The solver never divides by mass or I during each step — it multiplies
 * by their inverses instead (inv_mass, inv_i). This avoids a division
 * every frame and, more importantly, lets static bodies be expressed as
 * inv_mass = 0. A body with zero inverse mass ignores every force:
 *
 *   velocity += inv_mass * force * dt   // 0 * anything = 0
 *
 * So we compute:
 *
 *   inv_mass = 1.0 / mass   (or 0 if mass == FLT_MAX, i.e. static)
 *
 * The moment of inertia I measures how hard the body is to spin. A
 * wide, heavy box resists rotation more than a narrow, light one. It
 * is computed from mass and the box half-extents using the formula
 * from box2d-lite:
 *
 *   I = mass * (w.x^2 + w.y^2) / 12
 *
 * Then inv_i = 1.0 / I (or 0 if static), used the same way as
 * inv_mass — multiplied against torque to get angular acceleration.
 *
 * Pass FLT_MAX for mass to make the body static. It will not move,
 * rotate, or respond to forces — use this for walls, floors, and
 * any fixed platform.
 */
void Body::set(const Vec2& w, float m)
{
    position.set(0.0f, 0.0f);
    rotation = 0.0f;
    velocity.set(0.0f, 0.0f);
    angular_velocity = 0.0f;
    force.set(0.0f, 0.0f);
    torque = 0.0f;
    friction = 0.2f;

    width = w;
    mass = m;

    if (mass < FLT_MAX) {
        inv_mass = 1.0f / mass;
        I = mass * (width.x * width.x + width.y * width.y) / 12.0f;
        inv_i = 1.0f / I;
    } else {
        inv_mass = 0.0f;
        I = FLT_MAX;
        inv_i = 0.0f;
    }
}

} // namespace physics