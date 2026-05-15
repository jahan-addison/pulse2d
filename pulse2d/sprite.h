/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cstdint> // for uint16_t

namespace pulse2d {

/**
 * @brief
 *   RGB565 sprite descriptor
 *
 *   A non-owning view into pixel data managed by the Storage pool. The
 *   pixel data is row-major RGB565. On Teensy the pool lives in DMAMEM;
 *   on host it is a plain static array.
 *
 *   Sprite file format (raw binary):
 *     uint16_t  width
 *     uint16_t  height
 *     uint16_t  pixels[width * height]   (row-major, RGB565)
 */
struct Sprite
{
    uint16_t const* data; // row-major RGB565 pixels; nullptr on load failure
    uint16_t width;
    uint16_t height;
};

} // namespace pulse2d
