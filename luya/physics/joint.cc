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

#include <luya/physics/joint.h>

#include <luya/physics/body.h>  // for Body
#include <luya/physics/math.h>  // for Vec2, Mat22, operator*, operator-
#include <luya/physics/world.h> // for World

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
 *   physics::Joint pin;
 *   pin.set(&body_a, &body_b, { 0.0f, 1.0f }); // anchor at (0, 1)
 *   world.add(&pin);
 *
 *   pin.bias_factor = 0.3f; // slightly stiffer correction
 *
 *****************************************************************************/

namespace luya::physics {

/**
 * @brief
 * Store both bodies and convert the world-space anchor into per-body local
 * offsets that stay valid regardless of how the bodies rotate later.
 *
 * The anchor is a point in world space where the two bodies should be pinned
 * together. At the moment set() is called, the engine rotates that anchor
 * into each body's own coordinate frame:
 *
 *   local_anchor1 = transpose(rot1) * (anchor - body1->position)
 *   local_anchor2 = transpose(rot2) * (anchor - body2->position)
 *
 * local_anchor1 is a fixed offset from body1's center, expressed in body1's
 * local space. When body1 rotates in a future step, pre_step() rotates
 * local_anchor1 back to world space (stored as r1) to find where the anchor
 * actually is that frame. Storing it locally means the pin point "rides"
 * with the body correctly even as the body spins.
 *
 * Also resets P to zero (no accumulated impulse yet), softness to 0.0
 * (rigid constraint), and bias_factor to 0.2 (default correction strength).
 * Adjust these after calling set() if needed.
 */
void Joint::set(Body* b1, Body* b2, const Vec2& anchor)
{
    body1 = b1;
    body2 = b2;

    Mat22 rot_1(body1->rotation);
    Mat22 rot_2(body2->rotation);
    Mat22 rot_1t = rot_1.transpose();
    Mat22 rot_2t = rot_2.transpose();

    local_anchor1 = rot_1t * (anchor - body1->position);
    local_anchor2 = rot_2t * (anchor - body2->position);

    P.set(0.0f, 0.0f);

    softness = 0.0f;
    bias_factor = 0.2f;
}

/**
 * @brief
 * Prepare the joint for the iteration loop: rotate anchors to world space,
 * build the effective mass matrix M, compute the correction bias, and
 * optionally warm-start by re-applying the previous frame's impulse.
 *
 * The effective mass matrix M = K^{-1} is the key precomputation. K is the
 * "generalized mass" of the constraint - how hard it is to move both bodies
 * at the anchor. It accounts for both linear and rotational inertia:
 *
 * clang-format off
 *
 *   K = (1/m1 + 1/m2) * I  +  inv_i1 * skew(r1)^T * skew(r1)
 *                          +  inv_i2 * skew(r2)^T * skew(r2)
 *
 *   where skew(r) = [[ r.y, -r.x ],   converts a cross product into
 *                    [-r.y,  r.x ]]   a matrix multiply
 *
 * clang-format on
 *
 * M is inverted once here so apply_impulse() only needs to multiply by M
 * on every iteration rather than solving a 2x2 system from scratch.
 *
 * The bias velocity gently pushes the two anchor points back together if
 * they have drifted apart (only when World::position_correction is true):
 *
 *   bias = -bias_factor * inv_dt * (p2 - p1)
 *
 * where p1 = body1->position + r1 and p2 = body2->position + r2. A larger
 * bias_factor corrects drift faster but can overshoot and make the joint
 * feel bouncy. The default of 0.2 corrects 20% of the gap each step.
 *
 * Warm-starting: if World::warm_starting is true, the accumulated impulse P
 * from the previous step is applied to both bodies immediately. This seeds
 * the solver with a good starting estimate, which usually means the joint
 * converges in far fewer iterations than starting from zero.
 */
void Joint::pre_step(float inv_dt)
{
    // Pre-compute anchors, mass matrix, and bias.
    Mat22 rot_1(body1->rotation);
    Mat22 rot_2(body2->rotation);

    r1 = rot_1 * local_anchor1;
    r2 = rot_2 * local_anchor2;

    // deltaV = deltaV0 + K * impulse
    // invM = [(1/m1 + 1/m2) * eye(2) - skew(r1) * inv_i1 * skew(r1) -
    // skew(r2) * inv_i2 * skew(r2)]
    //      = [1/m1+1/m2     0    ] + inv_i1 * [r1.y*r1.y -r1.x*r1.y] + inv_i2
    //      * [r1.y*r1.y -r1.x*r1.y]
    //        [    0     1/m1+1/m2]           [-r1.x*r1.y r1.x*r1.x]
    //        [-r1.x*r1.y r1.x*r1.x]
    Mat22 K1;
    K1.col1.x = body1->inv_mass + body2->inv_mass;
    K1.col2.x = 0.0f;
    K1.col1.y = 0.0f;
    K1.col2.y = body1->inv_mass + body2->inv_mass;

    Mat22 K2;
    K2.col1.x = body1->inv_i * r1.y * r1.y;
    K2.col2.x = -body1->inv_i * r1.x * r1.y;
    K2.col1.y = -body1->inv_i * r1.x * r1.y;
    K2.col2.y = body1->inv_i * r1.x * r1.x;

    Mat22 K3;
    K3.col1.x = body2->inv_i * r2.y * r2.y;
    K3.col2.x = -body2->inv_i * r2.x * r2.y;
    K3.col1.y = -body2->inv_i * r2.x * r2.y;
    K3.col2.y = body2->inv_i * r2.x * r2.x;

    Mat22 K = K1 + K2 + K3;
    K.col1.x += softness;
    K.col2.y += softness;

    M = K.invert();

    Vec2 p1 = body1->position + r1;
    Vec2 p2 = body2->position + r2;
    Vec2 dp = p2 - p1;

    if (World::position_correction) {
        bias = -bias_factor * inv_dt * dp;
    } else {
        bias.set(0.0f, 0.0f);
    }

    if (World::warm_starting) {
        // Apply accumulated impulse.
        body1->velocity -= body1->inv_mass * P;
        body1->angular_velocity -= body1->inv_i * cross(r1, P);

        body2->velocity += body2->inv_mass * P;
        body2->angular_velocity += body2->inv_i * cross(r2, P);
    } else {
        P.set(0.0f, 0.0f);
    }
}

/**
 * @brief
 * Compute and apply a velocity correction impulse that pulls the two anchor
 * points toward each other.
 *
 * Each call is one pass of the sequential impulse solver. The goal is to
 * zero out the relative velocity at the pin point - the difference in speed
 * between the two anchors. The impulse that achieves this is:
 *
 *   impulse = M * (bias - relative_velocity - softness * P)
 *
 * where:
 *   M                  is the effective mass matrix from pre_step()
 *   bias               is the position-correction target velocity
 *   relative_velocity  is dv = v2 + w2×r2 - v1 - w1×r1
 *   softness * P       adds a small restoring force proportional to the
 *                      total accumulated impulse; makes the joint feel
 *                      slightly elastic when softness > 0
 *
 * The impulse is then split between both bodies: body1 is pushed in the
 * negative direction and body2 in the positive direction, each scaled by
 * their inverse mass and inverse inertia so heavier or harder-to-spin
 * bodies change speed less.
 *
 * P accumulates across all iterations within a frame so it can be reused
 * as the warm-start seed on the next frame.
 */
void Joint::apply_impulse()
{
    Vec2 dv = body2->velocity + cross(body2->angular_velocity, r2) -
              body1->velocity - cross(body1->angular_velocity, r1);

    Vec2 impulse;

    impulse = M * (bias - dv - softness * P);

    body1->velocity -= body1->inv_mass * impulse;
    body1->angular_velocity -= body1->inv_i * cross(r1, impulse);

    body2->velocity += body2->inv_mass * impulse;
    body2->angular_velocity += body2->inv_i * cross(r2, impulse);

    P += impulse;
}

} // namespace physics