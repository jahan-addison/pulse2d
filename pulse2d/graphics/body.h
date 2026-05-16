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
#include "types.h"
#include <concepts>
#include <type_traits>
#include <utility>

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

#define SET_BODY_DESCRIPTOR(name) detail::assign(&name, &desc.name)

namespace pulse2d::graphics {

namespace detail {

/**
 * @brief
 *  The descriptor pattern
 */
struct Body_Descriptor
{
    Vec2 position = { 0.0f, 0.0f };
    float rotation = 0.0f;
    Vec2 velocity = { 0.0f, 0.0f };
    float angular_velocity = 0.0f;
    Vec2 force = { 0.0f, 0.0f };
    float torque = 0.0f;
    Vec2 width = { 1.0f, 1.0f };
    float friction = 0.2f;
    float mass = FLT_MAX;
    float inv_mass = 0.0f;
    float I = FLT_MAX;
    float inv_i = 0.0f;
};

} // namespace detail

/**
 * @brief
 * A rigid box body - the fundamental unit of simulation. Every collidable
 * object in the scene is a Body.
 *
 * A Body carries two kinds of state: Kinematic state (position, rotation,
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
 * The solver never divides by mass directly - it multiplies by inv_mass.
 * This means a static body (inv_mass = 0) requires no special case: any
 * force multiplied by zero produces no velocity change.
 *
 * Width holds full dimensions, not half-extents. A body with
 * width = { 1.0, 0.5 } is a 1x0.5 unit box. Internally collide.cc
 * computes half-extents as h = 0.5 * width for the SAT overlap test.
 *
 * Note:
 *
 * Unlike in box2d-lite, this is an aggregate type that takes designated
 * initializers. A body with no defined mass, inv_mass, or inv_i, i.e.:
 *
 * (mass = FLT_MAX, inv_mass = 0, inv_i = 0).
 *
 * is an immovable static body.
 *
 */
class Body
{
  public:
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
     */
    void set_mass(Vec2 const& w, float m);

    void set_motion();

  public:
    /**
     * @brief
     * Set the world object using the descriptor pattern:
     *
     * Example:
     *
     *   my_body.set({
     *    .velocity = {0.1f, 0.0f},
     *    .rotation = 0.5f,
     *    .mass = 5.0f
     *    .width = {1.0f, 0.0f}
     *    // ...
     *   });
     *
     */
    inline void set(detail::Body_Descriptor const& desc)
    {
        SET_BODY_DESCRIPTOR(position);
        SET_BODY_DESCRIPTOR(rotation);
        SET_BODY_DESCRIPTOR(velocity);
        SET_BODY_DESCRIPTOR(angular_velocity);
        SET_BODY_DESCRIPTOR(force);
        SET_BODY_DESCRIPTOR(torque);
        SET_BODY_DESCRIPTOR(width);
        SET_BODY_DESCRIPTOR(friction);
        SET_BODY_DESCRIPTOR(mass);
        SET_BODY_DESCRIPTOR(inv_mass);
        SET_BODY_DESCRIPTOR(I);
        SET_BODY_DESCRIPTOR(inv_i);
    }

    inline void add_force(Vec2 const& f) { force += f; }

    /**
     * @brief
     * Center of the box in world space, in world units.
     *
     * The origin is wherever you decide - a common convention is to put
     * the floor at a large negative y so positive y is "up". Position is
     * updated by step() each frame:
     *
     *   position += dt * velocity
     *
     * Assign directly to teleport the body:
     *
     *   body.position = { 0.0f, 5.0f }; // place it 5 units above origin
     */
    Vec2 position = { 0.0f, 0.0f };

    /**
     * @brief
     * Orientation in radians, counter-clockwise from the x-axis.
     *
     * 0.0 means the box is axis-aligned (upright). Positive values rotate
     * counter-clockwise; negative values rotate clockwise. Updated by
     * step() each frame:
     *
     *   rotation += dt * angular_velocity
     *
     * IMPORTANT: rotation only affects the physics collision shape. The
     * renderer does not read this field when drawing sprites - sprites are
     * always blitted axis-aligned unless you pass an explicit angle to
     * Renderer::add_sprite(). To keep a sprite visually in sync with its
     * body, pass body.rotation as the angle_rad argument:
     *
     *   renderer.add_sprite(&sprite, sx, sy, body.rotation);
     */
    float rotation = 0.0f;

    /**
     * @brief
     * How fast the body is translating, in world units per second.
     *
     * velocity is a 2D vector: x is horizontal speed, y is vertical speed.
     * Positive y moves up; positive x moves right (standard math axes).
     * Each frame step() integrates forces into velocity, then integrates
     * velocity into position:
     *
     *   velocity  += dt * (gravity + inv_mass * force)
     *   position  += dt * velocity
     *
     * You can read velocity to check how fast a body is moving, or write
     * it to give a body an instant speed without going through forces:
     *
     *   body.velocity = { 3.0f, 0.0f }; // 3 units/second to the right
     *   body.velocity.y = 0.0f;         // kill vertical speed (e.g. on floor)
     *
     * For a push that builds up over many frames, prefer add_force() so
     * the physics integrator handles the acceleration naturally. Writing
     * velocity directly is best for one-shot effects like jumps.
     */
    Vec2 velocity = { 0.0f, 0.0f };

    /**
     * @brief
     * How fast the body is spinning, in radians per second.
     *
     * Positive values spin counter-clockwise. Updated each frame:
     *
     *   angular_velocity += dt * inv_i * torque
     *   rotation         += dt * angular_velocity
     *
     * A static body (inv_i = 0) ignores all torque, so angular_velocity
     * stays zero regardless of collisions. Assign directly to pre-spin:
     *
     *   body.angular_velocity = k_pi; // half a turn per second
     */
    float angular_velocity = 0.0f;

    /**
     * @brief
     * Accumulated force to apply this step, in world-unit kg·m/s².
     *
     * Never write to force directly - use add_force() so multiple callers
     * can contribute without overwriting each other. step() consumes force
     * via the integrator and then zeroes it, so it is automatically cleared
     * every frame:
     *
     *   velocity += dt * (gravity + inv_mass * force)
     *   force     = { 0, 0 }   // cleared by step()
     */
    Vec2 force = { 0.0f, 0.0f };

    /**
     * @brief Accumulated rotational force to apply this step, in N·m.
     *
     * Torque spins the body the same way force translates it. step() adds
     * it to angular_velocity and then zeroes it:
     *
     *   angular_velocity += dt * inv_i * torque
     *   torque            = 0   // cleared by step()
     *
     * Assign directly when you want a one-frame spin kick:
     *
     *   body.torque = 2.0f; // spin counter-clockwise for one step
     */
    float torque = 0.0f;

    /**
     * @brief Full width and height of the box, in world units.
     *
     * A value of { 2.0, 0.5 } is a box that is 2 units wide and 0.5 units
     * tall. Internally collide.cc computes half-extents as h = 0.5 * width
     * for the SAT overlap test - you never need to halve this yourself.
     *
     * For static bodies you can assign width directly. For dynamic bodies
     * always go through set(), which also recomputes mass and inertia:
     *
     *   body.set({ 2.0f, 0.5f }, 3.0f); // 2×0.5 box, 3 kg
     */
    Vec2 width = { 1.0f, 1.0f };

    /**
     * @brief
     * Surface friction coefficient. Range [0, 1].
     *
     * 0.0 is frictionless (ice); 1.0 is maximally rough. When two bodies
     * collide the engine uses the geometric mean of their friction values
     * so neither body alone controls the result:
     *
     *   contact_friction = sqrt(a.friction * b.friction)
     *
     * Defaults to 0.2 (light wood-on-wood).
     */
    float friction = 0.2f;

    /**
     * @brief
     * Total mass in kg.
     *
     * FLT_MAX means the body is static - it never moves
     * regardless of forces applied. Read-only after set().
     */
    float mass = FLT_MAX;

    /**
     * @brief
     * Reciprocal of mass (1/mass). 0 for static bodies.
     *
     * The solver always multiplies by inv_mass rather than dividing by mass.
     * This avoids a division every frame and naturally handles static bodies:
     * inv_mass = 0 means any force scaled by it produces zero acceleration.
     * Read-only after set().
     */
    float inv_mass = 0.0f;

    /**
     * @brief
     * Moment of inertia - resistance to spinning, in kg·m².
     *
     * Derived from mass and full box dimensions by set():
     *
     *   I = mass * (width.x^2 + width.y^2) / 12
     *
     * A wide, heavy box has a larger I and is harder to spin than a small,
     * light one. FLT_MAX for static bodies. Read-only after set().
     */
    float I = FLT_MAX;

    /**
     * @brief
     * Reciprocal of moment of inertia (1/I). 0 for static bodies.
     *
     * Used the same way as inv_mass - multiplied against torque to get
     * angular acceleration. 0 means the body never rotates.
     * Read-only after set().
     */
    float inv_i = 0.0f;
};

} // namespace pulse2d