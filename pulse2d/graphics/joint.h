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
 * Joint
 *
 * A joint links two bodies at a fixed point in the world so they cannot
 * drift apart. Think of it as a pin driven through both bodies at a shared
 * anchor position. The engine applies a small corrective push each step to
 * keep them at the original distance.
 *
 * Call set() with both bodies and the anchor point in world space, then
 * pass a pointer to World::add(). The joint does not need any per-frame
 * calls - the world handles it automatically inside step().
 *
 * bias_factor controls how hard the engine pushes them back together each
 * step (default 0.2, range 0–1). softness adds a small give to the
 * constraint so it feels spring-like rather than rigid (default 0.0).
 *
 *  Example:
 *
 *   graphics::Joint pin;
 *   pin.set(&body_a, &body_b, { 0.0f, 1.0f }); // anchor at (0, 1)
 *   world.add(&pin);
 *
 *   pin.bias_factor = 0.3f; // slightly stiffer correction
 *
 *****************************************************************************/

namespace pulse2d::graphics {

class Body;

/**
 * @brief
 * An impulse-based pin constraint that holds two bodies at a shared anchor
 * point regardless of any forces applied to either body.
 *
 * Each step the world calls pre_step() once, then apply_impulse() once per
 * iteration. pre_step() builds the effective mass matrix M = K^{-1}, where
 * K captures both linear and rotational inertia at the anchor:
 *
 * clang-format off
 *
 *   K = (1/m1 + 1/m2) * I  +  inv_i1 * skew(r1)^T * skew(r1)
 *                          +  inv_i2 * skew(r2)^T * skew(r2)
 *
 * clang-format on
 *
 * apply_impulse() then uses M to compute a velocity correction each iteration
 * that zeroes the relative motion at the pin point. Over `iterations` passes
 * both bodies converge on the constrained motion.
 *
 * The anchor is stored in each body's local frame (local_anchor1,
 * local_anchor2) so it rides with the body as it rotates. pre_step() rotates
 * these back to world space (r1, r2) at the start of each step.
 *
 * bias_factor and softness tune the feel of the constraint. bias_factor
 * (default 0.2) controls how aggressively drift is corrected each step.
 * softness (default 0.0) adds a spring-like give; increase it slightly if
 * the joint feels too rigid under heavy load.
 */
class Joint
{
  public:
    Joint()
        : P(0.0f, 0.0f)
        , body1(0)
        , body2(0)
        , bias_factor(0.2f)
        , softness(0.0f)
    {
    }

  public:
    void set(Body* body1, Body* body2, const Vec2& anchor);

    void pre_step(float inv_dt);
    void apply_impulse();

  public:
    Mat22 M; // effective mass matrix; computed by pre_step()
    Vec2 local_anchor1,
        local_anchor2; // anchor offsets in each body's local space
    Vec2 r1, r2;       // anchor offsets rotated to world space this step
    Vec2 bias;         // position correction velocity target
    Vec2 P;            // accumulated impulse across solver iterations
    Body* body1;
    Body* body2;
    float bias_factor; // position correction strength per step (default 0.2,
                       // range 0-1)
    float
        softness; // reduces constraint stiffness; 0 = rigid, > 0 = spring-like
};

} // namespace pulse2d