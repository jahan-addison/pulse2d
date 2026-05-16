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

#include "arbiter.h" // for Feature_Pair, Contact, Arbiter, collide
#include "body.h"    // for Body
#include "math.h"    // for Vec2, Mat22, operator*, dot, opera...

/****************************************************************************
 * Collide
 *
 * Box-vs-box contact generation using the separating axis theorem. Selects
 * the least-penetrating axis, identifies the reference and incident edges,
 * clips the incident edge to the reference face planes, and writes up to
 * two Contact points with separation, position, and feature data.
 *
 ****************************************************************************/

namespace pulse2d::graphics {

// Box vertex and edge numbering:
//
//        ^ y
//        |
//        e1
//   v2 ------ v1
//    |        |
// e2 |        | e4  --> x
//    |        |
//   v3 ------ v4
//        e3

enum Axis
{
    FACE_A_X,
    FACE_A_Y,
    FACE_B_X,
    FACE_B_Y
};

enum EdgeNumbers
{
    NO_EDGE = 0,
    EDGE1,
    EDGE2,
    EDGE3,
    EDGE4
};

struct ClipVertex
{
    ClipVertex() { fp.value = 0; }
    Vec2 v;
    Feature_Pair fp;
};

/**
 * @brief
 * Swap the in/out edge labels between body A's and body B's perspective.
 *
 * Contact feature labels record which pair of edges produced a contact
 * point. When body B is chosen as the reference face instead of A, the
 * labels need to be swapped so they are still consistent with the
 * A-perspective convention used everywhere else in the arbiter.
 */
void flip(Feature_Pair& fp)
{
    swap_val(fp.e.in_edge1, fp.e.in_edge2);
    swap_val(fp.e.out_edge1, fp.e.out_edge2);
}

/**
 * @brief
 * Clip the line segment defined by v_in[0] and v_in[1] against a half-plane
 * defined by (normal, offset), writing surviving vertices into v_out[].
 *
 * A vertex is "inside" the half-plane when:
 *   dot(normal, vertex) - offset <= 0
 *
 * Vertices outside are discarded. If the two input vertices are on opposite
 * sides of the boundary, an intersection point is computed by linear
 * interpolation and added to v_out. Its feature label is set to clip_edge
 * so the arbiter can track this new vertex across frames for warm-starting.
 *
 * Returns the number of vertices written to v_out (0, 1, or 2).
 */
int clip_segment_to_line(ClipVertex v_out[2],
    ClipVertex v_in[2],
    const Vec2& normal,
    float offset,
    char clip_edge)
{
    // Start with no output points
    int num_out = 0;

    // Calculate the distance of end points to the line
    float distance0 = dot(normal, v_in[0].v) - offset;
    float distance1 = dot(normal, v_in[1].v) - offset;

    // If the points are behind the plane
    if (distance0 <= 0.0f)
        v_out[num_out++] = v_in[0];
    if (distance1 <= 0.0f)
        v_out[num_out++] = v_in[1];

    // If the points are on different sides of the plane
    if (distance0 * distance1 < 0.0f) {
        // Find intersection point of edge and plane
        float interp = distance0 / (distance0 - distance1);
        v_out[num_out].v = v_in[0].v + interp * (v_in[1].v - v_in[0].v);
        if (distance0 > 0.0f) {
            v_out[num_out].fp = v_in[0].fp;
            v_out[num_out].fp.e.in_edge1 = clip_edge;
            v_out[num_out].fp.e.in_edge2 = NO_EDGE;
        } else {
            v_out[num_out].fp = v_in[1].fp;
            v_out[num_out].fp.e.out_edge1 = clip_edge;
            v_out[num_out].fp.e.out_edge2 = NO_EDGE;
        }
        ++num_out;
    }

    return num_out;
}

/**
 * @brief
 * Find the edge of the incident box that faces most directly toward the
 * reference face's inward normal, and write its two endpoints as world-space
 * ClipVertex values into c[0] and c[1].
 *
 * The normal arrives in the reference box's frame. It is rotated into the
 * incident box's local frame (and negated) to find which of the incident
 * box's four edges is most anti-parallel to it. That edge is the one most
 * "seen" by the reference face and therefore the one most likely to produce
 * valid contact points after clipping.
 */
static void compute_incident_edge(ClipVertex c[2],
    const Vec2& h,
    const Vec2& pos,
    const Mat22& rot,
    const Vec2& normal)
{
    // The normal is from the reference box. Convert it
    // to the incident boxe's frame and flip sign.
    Mat22 rot_t = rot.transpose();
    Vec2 n = -(rot_t * normal);
    Vec2 n_abs = abs_val(n);

    if (n_abs.x > n_abs.y) {
        if (sign(n.x) > 0.0f) {
            c[0].v.set(h.x, -h.y);
            c[0].fp.e.in_edge2 = EDGE3;
            c[0].fp.e.out_edge2 = EDGE4;

            c[1].v.set(h.x, h.y);
            c[1].fp.e.in_edge2 = EDGE4;
            c[1].fp.e.out_edge2 = EDGE1;
        } else {
            c[0].v.set(-h.x, h.y);
            c[0].fp.e.in_edge2 = EDGE1;
            c[0].fp.e.out_edge2 = EDGE2;

            c[1].v.set(-h.x, -h.y);
            c[1].fp.e.in_edge2 = EDGE2;
            c[1].fp.e.out_edge2 = EDGE3;
        }
    } else {
        if (sign(n.y) > 0.0f) {
            c[0].v.set(h.x, h.y);
            c[0].fp.e.in_edge2 = EDGE4;
            c[0].fp.e.out_edge2 = EDGE1;

            c[1].v.set(-h.x, h.y);
            c[1].fp.e.in_edge2 = EDGE1;
            c[1].fp.e.out_edge2 = EDGE2;
        } else {
            c[0].v.set(-h.x, -h.y);
            c[0].fp.e.in_edge2 = EDGE2;
            c[0].fp.e.out_edge2 = EDGE3;

            c[1].v.set(h.x, -h.y);
            c[1].fp.e.in_edge2 = EDGE3;
            c[1].fp.e.out_edge2 = EDGE4;
        }
    }

    c[0].v = pos + rot * c[0].v;
    c[1].v = pos + rot * c[1].v;
}

/**
 * @brief
 * Test two oriented boxes for overlap using the separating axis theorem (SAT)
 * and write up to two contact points into contacts[].
 *
 * SAT says two convex shapes overlap if and only if no axis separates their
 * projections. For two boxes there are four candidate axes: the two face
 * normals of box A and the two face normals of box B. The separation along
 * each axis is computed in the respective body's local frame:
 *
 * clang-format off
 *
 *   face_a = |d_a| - h_a - |C|   * h_b   (separation along A's own axes)
 *   face_b = |d_b| - |C^T| * h_a - h_b   (separation along B's own axes)
 *
 *   where d_a = rot_a^T * (pos_b - pos_a)  (B's center in A's frame)
 *         d_b = rot_b^T * (pos_b - pos_a)  (B's center in B's frame)
 *         C   = rot_a^T * rot_b            (relative rotation matrix)
 *
 * clang-format on
 *
 * If any separation is positive the boxes do not overlap and 0 is returned.
 * Otherwise the axis with the least penetration is chosen as the reference
 * axis. A small tolerance (relative_tol = 0.95, absolute_tol = 0.01) biases
 * the selection toward the same axis as the previous frame to reduce
 * flickering when two axes are nearly equal.
 *
 * Once the reference face is chosen, contact points are produced in
 * three steps:
 *
 *   1. compute_incident_edge() finds the edge of the other (incident) box
 *      that faces most directly toward the reference face.
 *
 *   2. clip_segment_to_line() clips the incident edge against each of the
 *      two side planes of the reference face, trimming it to the portion
 *      that lies within the reference face's column.
 *
 *   3. Any clipped vertex that lies behind the reference face plane
 *      (separation <= 0) becomes a contact point. Each contact carries a
 *      world-space position, a shared normal (pointing from A toward B),
 *      the penetration depth, and a feature identifier used to match
 *      contacts across frames for warm-starting.
 *
 * Returns the number of contact points found (0, 1, or 2).
 */
// The normal points from A to B
int collide(Arbiter::Contacts& contacts, Body const* bodyA, Body const* bodyB)
{
    // Setup
    Vec2 h_a = 0.5f * bodyA->width;
    Vec2 h_b = 0.5f * bodyB->width;

    Vec2 pos_a = bodyA->position;
    Vec2 pos_b = bodyB->position;

    Mat22 rot_a(bodyA->rotation), rot_b(bodyB->rotation);

    Mat22 rot_at = rot_a.transpose();
    Mat22 rot_bt = rot_b.transpose();

    Vec2 dp = pos_b - pos_a;
    Vec2 d_a = rot_at * dp;
    Vec2 d_b = rot_bt * dp;

    Mat22 C = rot_at * rot_b;
    Mat22 abs_c = abs_val(C);
    Mat22 abs_ct = abs_c.transpose();

    // Box A faces
    Vec2 face_a = abs_val(d_a) - h_a - abs_c * h_b;
    if (face_a.x > 0.0f or face_a.y > 0.0f)
        return 0;

    // Box B faces
    Vec2 face_b = abs_val(d_b) - abs_ct * h_a - h_b;
    if (face_b.x > 0.0f or face_b.y > 0.0f)
        return 0;

    // Find best axis
    Axis axis;
    float separation;
    Vec2 normal;

    // Box A faces
    axis = FACE_A_X;
    separation = face_a.x;
    normal = d_a.x > 0.0f ? rot_a.col1 : -rot_a.col1;

    const float relative_tol = 0.95f;
    const float absolute_tol = 0.01f;

    if (face_a.y > relative_tol * separation + absolute_tol * h_a.y) {
        axis = FACE_A_Y;
        separation = face_a.y;
        normal = d_a.y > 0.0f ? rot_a.col2 : -rot_a.col2;
    }

    // Box B faces
    if (face_b.x > relative_tol * separation + absolute_tol * h_b.x) {
        axis = FACE_B_X;
        separation = face_b.x;
        normal = d_b.x > 0.0f ? rot_b.col1 : -rot_b.col1;
    }

    if (face_b.y > relative_tol * separation + absolute_tol * h_b.y) {
        axis = FACE_B_Y;
        separation = face_b.y;
        normal = d_b.y > 0.0f ? rot_b.col2 : -rot_b.col2;
    }

    // Setup clipping plane data based on the separating axis
    Vec2 front_normal, side_normal;
    ClipVertex incident_edge[2];
    float front, neg_side, pos_side;
    char neg_edge, pos_edge;

    // Compute the clipping lines and the line segment to be clipped.
    switch (axis) {
        case FACE_A_X: {
            front_normal = normal;
            front = dot(pos_a, front_normal) + h_a.x;
            side_normal = rot_a.col2;
            float side = dot(pos_a, side_normal);
            neg_side = -side + h_a.y;
            pos_side = side + h_a.y;
            neg_edge = EDGE3;
            pos_edge = EDGE1;
            compute_incident_edge(
                incident_edge, h_b, pos_b, rot_b, front_normal);
        } break;

        case FACE_A_Y: {
            front_normal = normal;
            front = dot(pos_a, front_normal) + h_a.y;
            side_normal = rot_a.col1;
            float side = dot(pos_a, side_normal);
            neg_side = -side + h_a.x;
            pos_side = side + h_a.x;
            neg_edge = EDGE2;
            pos_edge = EDGE4;
            compute_incident_edge(
                incident_edge, h_b, pos_b, rot_b, front_normal);
        } break;

        case FACE_B_X: {
            front_normal = -normal;
            front = dot(pos_b, front_normal) + h_b.x;
            side_normal = rot_b.col2;
            float side = dot(pos_b, side_normal);
            neg_side = -side + h_b.y;
            pos_side = side + h_b.y;
            neg_edge = EDGE3;
            pos_edge = EDGE1;
            compute_incident_edge(
                incident_edge, h_a, pos_a, rot_a, front_normal);
        } break;

        case FACE_B_Y: {
            front_normal = -normal;
            front = dot(pos_b, front_normal) + h_b.y;
            side_normal = rot_b.col1;
            float side = dot(pos_b, side_normal);
            neg_side = -side + h_b.x;
            pos_side = side + h_b.x;
            neg_edge = EDGE2;
            pos_edge = EDGE4;
            compute_incident_edge(
                incident_edge, h_a, pos_a, rot_a, front_normal);
        } break;
    }

    // clip other face with 5 box planes (1 face plane, 4 edge planes)

    ClipVertex clip_points1[2];
    ClipVertex clip_points2[2];
    int np;

    // Clip to box side 1
    np = clip_segment_to_line(
        clip_points1, incident_edge, -side_normal, neg_side, neg_edge);

    if (np < 2)
        return 0;

    // Clip to negative box side 1
    np = clip_segment_to_line(
        clip_points2, clip_points1, side_normal, pos_side, pos_edge);

    if (np < 2)
        return 0;

    // Now clip_points2 contains the clipping points.
    // Due to roundoff, it is possible that clipping removes all points.

    int num_contacts = 0;
    for (int i = 0; i < 2; ++i) {
        float c_separation = dot(front_normal, clip_points2[i].v) - front;

        if (c_separation <= 0) {
            contacts[num_contacts].separation = c_separation;
            contacts[num_contacts].normal = normal;
            // slide contact point onto reference face (easy to cull)
            contacts[num_contacts].position =
                clip_points2[i].v - c_separation * front_normal;
            contacts[num_contacts].feature = clip_points2[i].fp;
            if (axis == FACE_B_X or axis == FACE_B_Y)
                flip(contacts[num_contacts].feature);
            ++num_contacts;
        }
    }

    return num_contacts;
}

} // namespace pulse2d