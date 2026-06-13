/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cstddef>          // for size_t
#include <etl/array.h>      // for array
#include <pulse2d/config.h> // for MAX_LOADED_SPRITES
#include <pulse2d/sprite.h> // for Sprite
#include <pulse2d/util.h>   // for PULSE2D_TEENSY

#if defined(PULSE2D_TEENSY)
#include <SdFat.h> // for SdFs, SdioConfig, FIFO_SDIO, BUILTIN_SDCARD
#endif

/****************************************************************************
 * Storage
 *
 * On Teensy, reads .bin (uint16_t width, height, then pixels) from
 * the built-in SDIO SD card via SdFat.
 *
 * Otherwise, Uses stb_image to decode any supported format and converts
 * RGBA8 to RGB565 with nearest-neighbour scaling.
 *
 ****************************************************************************/

namespace pulse2d {

/**
 * @brief
 *   Storage component - sprite loader
 *
 *   A fixed pool of RGB565 pixel buffers. Sprites are loaded from
 *   disk on host or from the built-in SDIO SD card slot via SdFat on
 *   Teensy 4.1.
 *
 *   Sprite file format:
 *     uint16_t  width
 *     uint16_t  height
 *     uint16_t  pixels[width * height]
 *
 *   Call reset() to reclaim all pool slots between scenes.
 */
class Storage
{
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
    using Pixel_Buffer = etl::array<uint16_t, config::max_sprite_pixels>;
    etl::array<Pixel_Buffer, config::max_loaded_sprites> pool_{};
    size_t next_slot_{ 0 };

#if defined(PULSE2D_TEENSY)
    SdFs sd_;
#endif
};

} // namespace pulse2d
