/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

/****************************************************************************
 * Core
 *
 * This is what PULSE2D_HEADER expands to in Teensy builds.
 *
 * Game files should never include this header - use PULSE2D_HEADER
 * so the same source compiles on both Teensy and host:
 *
 *   #include PULSE2D_HEADER    // expands to "pulse2d/core.h"
 *   #include PULSE2D_GRAPHICS  // expands to "pulse2d/graphics/all.h"
 *
 * Header inclusion order matters here: pulse2d.h defines the engine
 * types and Static_Inplace_T, dsl.h includes api.h directly and
 * adds DSL macros on top.
 *
 ****************************************************************************/

#include <pulse2d/pulse2d.h>

#include <pulse2d/dsl.h>

#include <pulse2d/api.h>

// External
#include <boost/sml.hpp>
