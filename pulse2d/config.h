/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <pulse2d/util.h> // for PULSE2D_TEENSY

#ifndef MAX_PHYSICS_BODIES
#define MAX_PHYSICS_BODIES 256
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

#if defined(PULSE2D_TEENSY)
inline constexpr size_t max_sprite_pixels = 96 * 96;
#else
inline constexpr size_t max_sprite_pixels = width * height;
#endif

} // namespace config