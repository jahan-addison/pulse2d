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

#include <pulse2d/graphics/body.h>

#include <float.h>                 // for FLT_MAX
#include <pulse2d/graphics/math.h> // for Vec2

/****************************************************************************
 * Body
 *
 * A Body is a rectangular rigid object. It holds two kinds of state:
 *
 *   Kinematic - where it is and how fast it is moving:
 *     position, rotation, velocity, angular_velocity
 *
 *   Dynamic - accumulated pushes waiting to be applied this frame:
 *     force, torque
 *
 * Every frame World::step() integrates the dynamic state into the kinematic
 * state in this order:
 *
 *   1. gravity and force are added to velocity:
 *        velocity += dt * (gravity + inv_mass * force)
 *
 *   2. torque is added to angular_velocity:
 *        angular_velocity += dt * inv_i * torque
 *
 *   3. velocity is integrated into position:
 *        position += dt * velocity
 *
 *   4. angular_velocity is integrated into rotation:
 *        rotation += dt * angular_velocity
 *
 *   5. force and torque are cleared to zero.
 *
 * This means gravity affects velocity first, then velocity moves the body.
 * Setting velocity directly (e.g. for a jump) bypasses step 1 - gravity
 * will still accumulate from the next frame onward. Setting force via
 * add_force() lets gravity and the push combine naturally each frame.
 *
 * A body created without calling set() has inv_mass = 0 and never moves
 * regardless of gravity or forces. Use this for static floors and walls.
 *
 *  Example:
 *
 *   graphics::Body box;
 *   box.set({ 1.0f, 1.0f }, 10.0f); // 1x1 unit box, 10 kg
 *   box.position = { 0.0f, 3.0f };  // start 3 units above origin
 *   world.add(&box);
 *
 *   // give it a horizontal speed; gravity will arc it downward
 *   box.velocity = { 3.0f, 0.0f };
 *
 *   // or push it for one frame; add_force() can be called repeatedly
 *   box.add_force({ 5.0f, 0.0f });
 *
 ****************************************************************************/

namespace pulse2d::graphics {

/**
 * @brief
 * Configure this body with a box size and mass, then derive all the
 * internal values the solver needs. Also resets all motion state -
 * position, velocity, forces - back to zero, so it is safe to call
 * set_mass() more than once to reinitialize the body between levels.
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
 * The solver never divides by mass or I during each step - it multiplies
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
 * inv_mass - multiplied against torque to get angular acceleration.
 *
 * Pass FLT_MAX for mass to make the body static. It will not move,
 * rotate, or respond to forces - use this for walls, floors, and
 * any fixed platform.
 */
void Body::set_mass(const Vec2& w, float m)
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

} // namespace pulse2d