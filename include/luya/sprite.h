/*****************************************************************************
 * Copyright (c) Jahan Addison
 *
 * This software is dual-licensed under the Apache License, Version 2.0 or
 * the GNU General Public License, Version 3.0 or later.
 *
 * You may use this work, in part or in whole, under the terms of either
 * license.
 *
 * See the LICENSE.Apache-v2 and LICENSE.GPL-v3 files in the project root
 * for the full text of these licenses.
 ****************************************************************************/

#pragma once

#include <cstdint> // for uint16_t

namespace luya {

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

} // namespace luya
