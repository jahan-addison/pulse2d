/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <pulse2d/util.h> // for PULSE2D_TEENSY

/****************************************************************************
 * Config
 *
 * Compile-time pool sizes and display constants for pulse2d. Every capacity
 * limit is guarded by #ifndef so a game can override any of them by defining
 * the symbol before including this header or via a -D flag in the build
 * system.
 *
 * The defaults are tuned for the Teensy 4.1 (512 KB OCRAM, 8 MB flash):
 *
 *   MAX_PHYSICS_BODIES             64   bodies per world
 *   MAX_PHYSICS_JOINTS             32   joints per world
 *   MAX_LOADED_SPRITES             12   sprites per scene
 *   MAX_BACKGROUND_LAYERS           8   parallax layers per scene
 *   MAX_ACTIVE_KINEMATIC_POOL      32   live pool slots across all pools
 *   MAX_ACTIVE_KINEMATIC_INSTANCE   8   named pools per scene
 *   MAX_ANIMATION_DEFINITION       16   registered VFX definitions per scene
 *   MAX_ACTIVE_ANIMATION           32   simultaneously playing VFX instances
 *
 * To raise a limit for a large scene, add a -D before including core.h:
 *
 *   // my_game/Makefile (or CMakeLists.txt)
 *   -DMAX_LOADED_SPRITES=20
 *   -DMAX_ACTIVE_KINEMATIC_POOL=64
 *
 * config::width / config::height are the ILI9341 native resolution (320x240).
 * config::scale is the SDL2 window multiplier for host development (960x720).
 * config::pixels_per_unit is the physics-to-screen conversion factor (30.0).
 *
 ****************************************************************************/

#ifndef MAX_PHYSICS_BODIES
#define MAX_PHYSICS_BODIES 64
#endif

#ifndef MAX_PHYSICS_JOINTS
#define MAX_PHYSICS_JOINTS 32
#endif

#ifndef MAX_LOADED_SPRITES
#define MAX_LOADED_SPRITES 12
#endif

#ifndef MAX_BACKGROUND_LAYERS
#define MAX_BACKGROUND_LAYERS 8
#endif

#ifndef MAX_ACTIVE_KINEMATIC_POOL
#define MAX_ACTIVE_KINEMATIC_POOL 32
#endif

#ifndef MAX_ACTIVE_KINEMATIC_INSTANCE
#define MAX_ACTIVE_KINEMATIC_INSTANCE 8
#endif

#ifndef MAX_ANIMATION_DEFINITION
#define MAX_ANIMATION_DEFINITION 16
#endif

#ifndef MAX_CONTACT_POINTS
#define MAX_CONTACT_POINTS 2
#endif

// Maximum simultaneous contact pairs in the arbiter map.
// For n active bodies the theoretical worst case is n*(n-1)/2 pairs; in
// practice most pairs are not in contact at the same time. 64 handles the
// default MAX_PHYSICS_BODIES=64 comfortably for typical sparse contact patterns
// (space shooters, platformers). Raise it if your scene has many bodies in
// dense simultaneous contact.
#ifndef MAX_ARBITER_PAIRS
#define MAX_ARBITER_PAIRS 64
#endif

#ifndef MAX_ACTIVE_ANIMATION
#define MAX_ACTIVE_ANIMATION 32
#endif

namespace pulse2d::config {
/**
 * @brief ILI9341 native display resolution and SDL2 desktop scale factor
 */
inline constexpr int width = 320;
inline constexpr int height = 240;
inline constexpr int scale = 3; // SDL2 window scale (960x720)

inline constexpr size_t max_loaded_sprites = MAX_LOADED_SPRITES;

inline constexpr std::size_t max_pooled_objects = MAX_ACTIVE_KINEMATIC_POOL;

inline constexpr float pixels_per_unit = 30.0f;

#if defined(PULSE2D_TEENSY)
inline constexpr size_t max_sprite_pixels = 96 * 96;
#else
inline constexpr size_t max_sprite_pixels = width * height;
#endif

} // namespace config