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

#include <luya/renderer.h>

#include <cmath>                // for cosf, sinf
#include <cstring>              // for memset
#include <luya/physics/body.h>  // for Body
#include <luya/physics/math.h>  // for Vec2
#include <luya/physics/world.h> // for World

/****************************************************************************
 * Renderer
 *
 * Contains the full-screen RGB565 framebuffer. Each frame:
 *   1. clear()  - fill the framebuffer with a background color
 *   2. draw()   - rasterize active physics bodies and queued sprites
 *   3. render() - blit the finished framebuffer to the display driver
 *
 *  Example:
 *
 *   luya::Engine engine{};
 *   engine.init();
 *
 *   auto& renderer = engine.renderer();
 *   renderer.clear();
 *   renderer.draw(world);
 *   renderer.render();
 *
 ****************************************************************************/

namespace luya {

/**
 * @brief Fill the framebuffer with a solid RGB565 color
 */
void Renderer::clear(uint16_t color)
{
    if (color == 0x0000) {
        framebuffer_->fill(0);
        return;
    }
    framebuffer_->fill(color);
}

/**
 * @brief Queue a sprite to be drawn at (x, y) on the next render pass.
 * angle_rad rotates the sprite counter-clockwise around its center,
 * 0.0 is the fast axis-aligned path. Pass body.rotation directly to keep
 * a sprite visually in sync with its physics body.
 */
void Renderer::add_sprite(Sprite const* sprite,
    int16_t x,
    int16_t y,
    float angle_rad)
{
    if (sprite and not sprite_queue_.full()) {
        sprite_queue_.emplace_back(Entry{ sprite, x, y, angle_rad });
    }
}

/**
 * @brief Rasterize all active bodies in the world and queued sprites
 * into the framebuffer, then drain the sprite queue
 */
void Renderer::draw(physics::World const& world)
{
    for (auto const* body : world.bodies) {
        draw_body(body);
    }
    for (auto const& entry : sprite_queue_) {
        draw_sprite(entry.sprite, entry.x, entry.y, entry.angle_rad);
    }
    sprite_queue_.clear();
}

/**
 * @brief Rasterize a body's axis-aligned bounding box
 */
void Renderer::draw_body(physics::Body const* body)
{
    if (!show_debug_rects)
        return;

    // Y is negated so that positive physics-Y maps upward on screen
    const int cx =
        static_cast<int>(body->position.x * k_pixels_per_unit) + k_center_x;
    const int cy =
        k_center_y - static_cast<int>(body->position.y * k_pixels_per_unit);

    // body->width holds full dimensions; use half-extents for centering
    const int hw = static_cast<int>(body->width.x * k_pixels_per_unit * 0.5f);
    const int hh = static_cast<int>(body->width.y * k_pixels_per_unit * 0.5f);

    fill_rect(cx - hw, cy - hh, 2 * hw, 2 * hh, 0xFFFF);
}

/**
 * @brief Blit a sprite into the framebuffer at (sx, sy), clipping to screen
 * bounds. If angle_rad is non-zero the sprite is rotated counter-clockwise
 * around its center using an inverse-transform per-pixel lookup.
 */
void Renderer::draw_sprite(Sprite const* sprite,
    int16_t sx,
    int16_t sy,
    float angle_rad)
{
    if (angle_rad == 0.0f) {
        // fast path - axis-aligned blit
        for (uint16_t row = 0; row < sprite->height; ++row) {
            const int dy = sy + static_cast<int>(row);
            if (dy < 0 or dy >= k_height)
                continue;

            for (uint16_t col = 0; col < sprite->width; ++col) {
                const int dx = sx + static_cast<int>(col);
                if (dx < 0 or dx >= k_width)
                    continue;

                uint16_t const px = sprite->data[row * sprite->width + col];
                if (px != 0xF81F)
                    (*framebuffer_)[dy * k_width + dx] = px;
            }
        }
        return;
    }

    // Rotated blit: for each output pixel, inverse-rotate back to the source
    // scanning area is a square circumscribed around the sprite diagonal
    // so no corner is missed after rotation.
    const float cos_a = cosf(angle_rad);
    const float sin_a = sinf(angle_rad);
    const float half_w = sprite->width * 0.5f;
    const float half_h = sprite->height * 0.5f;
    // centre of the sprite in screen space
    const float cx = sx + half_w;
    const float cy = sy + half_h;
    // scan radius: half the diagonal so all rotated corners are covered
    const int radius =
        static_cast<int>(sqrtf(half_w * half_w + half_h * half_h)) + 1;

    for (int oy = -radius; oy <= radius; ++oy) {
        const int screen_y = static_cast<int>(cy) + oy;
        if (screen_y < 0 or screen_y >= k_height)
            continue;

        for (int ox = -radius; ox <= radius; ++ox) {
            const int screen_x = static_cast<int>(cx) + ox;
            if (screen_x < 0 or screen_x >= k_width)
                continue;

            // inverse rotation: map output pixel back to sprite-local coords
            const float src_u = ox * cos_a + oy * sin_a + half_w;
            const float src_v = -ox * sin_a + oy * cos_a + half_h;

            const int src_x = static_cast<int>(src_u);
            const int src_y = static_cast<int>(src_v);

            if (src_x < 0 or src_x >= sprite->width)
                continue;
            if (src_y < 0 or src_y >= sprite->height)
                continue;

            uint16_t const px = sprite->data[src_y * sprite->width + src_x];
            if (px != 0xF81F)
                (*framebuffer_)[screen_y * k_width + screen_x] = px;
        }
    }
}

/**
 * @brief Fill an axis-aligned rectangle in the framebuffer, clipping to screen
 * bounds
 */
void Renderer::fill_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int row = y; row < y + h; ++row) {
        if (row < 0 or row >= k_height)
            continue;
        for (int col = x; col < x + w; ++col) {
            if (col < 0 or col >= k_width)
                continue;
            (*framebuffer_)[row * k_width + col] = color;
        }
    }
}

} // namespace luya
