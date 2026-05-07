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

#include <luya/storage.h>

#if !defined(__IMXRT1062__)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // for stbi_load, stbi_image_free
#endif

/****************************************************************************
 * Storage
 *
 * Fixed-pool sprite loader. On host, uses stb_image to decode any supported
 * format and converts RGBA8 to RGB565 with nearest-neighbour scaling.
 *
 * On Teensy, reads raw binary (uint16_t width, height, then pixels) from
 * the built-in SDIO SD card via SdFat.
 *
 ****************************************************************************/

namespace luya {

/**
 * @brief Mount and initialise the SD card (no-op on host, returns true)
 */
bool Storage::init()
{
#if defined(__IMXRT1062__)
    return sd_.begin(SdioConfig(FIFO_SDIO));
#else
    return true;
#endif
}

/**
 * @brief Load a sprite from disk (host) or SD card (Teensy) into the pool
 *
 * Reads a raw binary file: uint16_t width, uint16_t height, then
 * width * height uint16_t RGB565 pixels. Returns a Sprite with a nullptr
 * data pointer on any failure (pool exhausted, file not found, too large).
 */
Sprite Storage::load_sprite(const char* path,
    uint16_t target_w,
    uint16_t target_h)
{
    if (next_slot_ >= k_max_loaded_sprites) {
        return { nullptr, 0, 0 };
    }

    uint16_t w = 0;
    uint16_t h = 0;

#if defined(__IMXRT1062__)
    auto file = sd_.open(path, O_RDONLY);
    if (!file) {
        return { nullptr, 0, 0 };
    }
    if (file.read(&w, sizeof(w)) != sizeof(w) or
        file.read(&h, sizeof(h)) != sizeof(h)) {
        file.close();
        return { nullptr, 0, 0 };
    }
    const size_t pixel_count = static_cast<size_t>(w) * h;
    if (pixel_count == 0 or pixel_count > k_max_sprite_pixels) {
        file.close();
        return { nullptr, 0, 0 };
    }
    auto& buf = pool_[next_slot_];
    if (file.read(buf.data(), pixel_count * sizeof(uint16_t)) !=
        static_cast<int>(pixel_count * sizeof(uint16_t))) {
        file.close();
        return { nullptr, 0, 0 };
    }
    file.close();
    (void)target_w;
    (void)target_h;
#else
    int src_w = 0, src_h = 0, channels = 0;
    uint8_t* img = stbi_load(path, &src_w, &src_h, &channels, 4);
    if (!img) {
        return { nullptr, 0, 0 };
    }

    // Resolve final dimensions; fall back to source size if no target given
    w = target_w ? target_w : static_cast<uint16_t>(src_w);
    h = target_h ? target_h : static_cast<uint16_t>(src_h);

    const size_t pixel_count = static_cast<size_t>(w) * h;
    if (pixel_count == 0 or pixel_count > k_max_sprite_pixels) {
        stbi_image_free(img);
        return { nullptr, 0, 0 };
    }

    auto& buf = pool_[next_slot_];

    // nearest-neighbour scale + RGBA8 → RGB565 conversion
    // pixels with alpha < 128 are written as 0xF81F (magenta chroma-key)
    for (uint16_t row = 0; row < h; ++row) {
        int const sy = row * src_h / h;
        for (uint16_t col = 0; col < w; ++col) {
            int const sx = col * src_w / w;
            const uint8_t* px = img + (sy * src_w + sx) * 4;
            if (px[3] < 16) {
                buf[row * w + col] = 0xF81F; // transparent sentinel
            } else {
                buf[row * w + col] = static_cast<uint16_t>(
                    ((px[0] >> 3) << 11) | ((px[1] >> 2) << 5) | (px[2] >> 3));
            }
        }
    }

    stbi_image_free(img);
#endif

    return { pool_[next_slot_++].data(), w, h };
}

/**
 * @brief Reclaim all pool slots - invalidates all previously returned Sprites
 */
void Storage::reset()
{
    next_slot_ = 0;
}

} // namespace luya
