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

#include <cstddef>       // for size_t
#include <etl/array.h>   // for array
#include <luya/sprite.h> // for Sprite
#include <luya/util.h>   // for LUYA_TEENSY

#ifndef MAX_LOADED_SPRITES
#define MAX_LOADED_SPRITES 8
#endif

#if defined(LUYA_TEENSY)
#include <SdFat.h> // for SdFs, SdioConfig, FIFO_SDIO, BUILTIN_SDCARD
#endif

/****************************************************************************
 * Storage
 *
 * A fixed-pool sprite loader. On host, uses stb_image to decode any supported
 * format and converts RGBA8 to RGB565 with nearest-neighbour scaling.
 *
 * On Teensy, reads raw binary (uint16_t width, height, then pixels) from
 * the built-in SDIO SD card via SdFat.
 *
 ****************************************************************************/

namespace luya {

/**
 * @brief
 *   Storage component - sprite loader
 *
 *   Manages a fixed pool of RGB565 pixel buffers. Sprites are loaded from
 *   disk on host or from the built-in SDIO SD card slot via SdFat on
 *   Teensy 4.1.
 *
 *   Sprite file format (raw binary):
 *     uint16_t  width
 *     uint16_t  height
 *     uint16_t  pixels[width * height]
 *
 *   Call reset() to reclaim all pool slots between scenes.
 */
class Storage
{
  public:
    // maximum number of sprites that can be resident at once
    static constexpr size_t k_max_loaded_sprites = MAX_LOADED_SPRITES;
    // maximum pixels per sprite - sized for the host display; Teensy games
    // should use smaller sprites and tune this for available SRAM.
#if defined(LUYA_TEENSY)
    static constexpr size_t k_max_sprite_pixels = 96 * 96;
#else
    static constexpr size_t k_max_sprite_pixels = 320 * 240;
#endif

  public:
    Storage() = default;
    Storage(Storage const&) = delete;
    Storage& operator=(Storage const&) = delete;

  public:
    bool init();
    Sprite load_sprite(const char* path,
        uint16_t target_w = 0,
        uint16_t target_h = 0);
    void reset();

  private:
    using Pixel_Buffer = etl::array<uint16_t, k_max_sprite_pixels>;
    etl::array<Pixel_Buffer, k_max_loaded_sprites> pool_{};
    size_t next_slot_{ 0 };

#if defined(LUYA_TEENSY)
    SdFs sd_;
#endif
};

} // namespace luya
