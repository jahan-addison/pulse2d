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

#pragma once

#include "math.h"

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
 * A rigid box body — the fundamental unit of simulation. Every collidable
 * object in the scene is a Body.
 *
 * A Body carries two kinds of state. Kinematic state (position, rotation,
 * velocity, angular_velocity) describes where the body is and how fast it
 * is moving. Dynamic state (force, torque) is the total push accumulated
 * during a frame. Each call to World::step() integrates dynamic into
 * kinematic via Newton's second law:
 *
 * clang-format off
 *
 *   velocity         += dt * (gravity + inv_mass * force)
 *   angular_velocity += dt * inv_i    * torque
 *   position         += dt * velocity
 *   rotation         += dt * angular_velocity
 *
 * clang-format on
 *
 * The solver never divides by mass directly — it multiplies by inv_mass.
 * This means a static body (inv_mass = 0) requires no special case: any
 * force multiplied by zero produces no velocity change. A default-constructed
 * Body is already static; call set() to make it dynamic.
 *
 * width holds full dimensions, not half-extents. A body with
 * width = { 1.0, 0.5 } is a 1x0.5 unit box. Internally collide.cc
 * computes half-extents as h = 0.5 * width for the SAT overlap test.
 */
class Body
{
  public:
    Body();
    void set(const Vec2& w, float m);

    void add_force(const Vec2& f) { force += f; }

    Vec2 position; // where the center of the box is in world space
    float
        rotation; // angle in radians; 0 = upright, positive = counter-clockwise

    Vec2 velocity;          // how fast the body is moving (units per second)
    float angular_velocity; // how fast it is spinning (radians per second)

    Vec2 force;   // total force to apply this step; cleared each frame
    float torque; // rotational force to apply this step; cleared each frame

    Vec2 width; // full width and height of the box

  public:
    float friction; // how much the surface resists sliding (0 = ice, 1 = rough)
    float mass;     // total mass in kg; FLT_MAX means the body never moves
    float inv_mass; // 1/mass; 0 when the body is static (avoids dividing by
                    // FLT_MAX)
    float I;        // resistance to spinning, derived from mass and size
    float inv_i;    // 1/I; 0 when the body is static
};

} // namespace physics