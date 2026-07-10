/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <pulse2d/renderer.h>

#include <algorithm>                 // for for_each
#include <cmath>                     // for cosf, sinf, sqrtf
#include <etl/utility.h>             // for forward
#include <pulse2d/detail/glcdfont.h> // for k_glcd_font, k_glcd_cell_w/h
#include <pulse2d/graphics/body.h>   // for Body
#include <pulse2d/graphics/math.h>   // for Vec2
#include <pulse2d/graphics/world.h>  // for World
#include <pulse2d/sprite.h>          // for Sprite
#include <string_view>               // for string_view

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
 *   pulse2d::Pulse2d engine{};
 *   engine.init();
 *
 *   auto& renderer = engine.renderer();
 *   renderer.clear();
 *   renderer.draw(world);
 *   renderer.render();
 *
 ****************************************************************************/

namespace pulse2d {

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
    if (sprite and sprite->data and not sprite_queue_.full()) {
        sprite_queue_.emplace_back(Entry{ sprite, x, y, angle_rad });
    }
}

/**
 * @brief Rasterize all active bodies in the world and queued sprites
 * into the framebuffer, then drain the sprite queue
 *
 * Note: Runs on the reverse iterator
 */
void Renderer::draw(graphics::World const& world)
{
    std::for_each(world.bodies.begin(),
        world.bodies.end(),
        [&, this](auto const* body) { this->draw_body(body); });

    std::for_each(sprite_queue_.begin(),
        sprite_queue_.end(),
        [&, this](auto const& entry) {
            this->draw_sprite(entry.sprite, entry.x, entry.y, entry.angle_rad);
        });

    sprite_queue_.clear();

    // flush text entries after sprites so glyphs composite on top
    for (auto const& e : text_queue_) {
        using namespace detail;
        const uint8_t* fdata = e.font ? e.font->data : k_glcd_font;
        uint8_t fw = e.font ? e.font->cell_w : k_glcd_cell_w;
        uint8_t fh = e.font ? e.font->cell_h : k_glcd_cell_h;
        uint8_t first = e.font ? e.font->first_char : 0;
        int cx = e.x;
        int cy = e.y;
        for (char c : e.sv) {
            const auto uc = static_cast<uint8_t>(c);
            if (c == '\n') {
                cy += static_cast<int>((fh + 1) * e.size);
                cx = e.x;
            } else if (!e.font || uc >= first) {
                draw_glyph(e.font ? (uc - first) : uc,
                    fdata,
                    fw,
                    fh,
                    cx,
                    cy,
                    e.color,
                    e.size);
                cx += static_cast<int>((fw + 1) * e.size);
            }
        }
    }
    text_queue_.clear();
}

/**
 * @brief Rasterize a body's axis-aligned bounding box
 */
void Renderer::draw_body(graphics::Body const* body)
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

    fill_rect(cx - hw, cy - hh, 2 * hw, 2 * hh, 0xF800);
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
    const float radius = sqrtf(half_w * half_w + half_h * half_h) + 1.0f;

    for (float oy = -radius; oy <= radius; ++oy) {
        const int screen_y = static_cast<int>(cy + oy);
        if (screen_y < 0 or screen_y >= k_height)
            continue;

        for (float ox = -radius; ox <= radius; ++ox) {
            const int screen_x = static_cast<int>(cx + ox);
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

/**
 * @brief Blit one glyph into the framebuffer.
 * glyph_idx is the pre-offset index into data (i.e. c - first_char for
 * custom fonts, or the raw ASCII value for the built-in font).
 */
void Renderer::draw_glyph(uint8_t glyph_idx,
    const uint8_t* data,
    uint8_t cell_w,
    uint8_t cell_h,
    int x,
    int y,
    uint16_t color,
    float scale)
{
    const uint8_t* glyph = data + (glyph_idx * cell_w);
    const int out_w = static_cast<int>(cell_w * scale + 0.5f);
    const int out_h = static_cast<int>(cell_h * scale + 0.5f);

    for (int oy = 0; oy < out_h; ++oy) {
        const auto src_row = static_cast<uint8_t>(oy / scale);
        if (src_row >= cell_h)
            continue;
        for (int ox = 0; ox < out_w; ++ox) {
            const auto src_col = static_cast<uint8_t>(ox / scale);
            if (src_col >= cell_w)
                continue;
            if (!(glyph[src_col] & (1u << src_row)))
                continue;
            const int px = x + ox;
            const int py = y + oy;
            if (px >= 0 && px < k_width && py >= 0 && py < k_height)
                (*framebuffer_)[py * k_width + px] = color;
        }
    }
}

void Renderer::draw_text(std::string_view sv,
    int x,
    int y,
    uint16_t color,
    float size)
{
    if (!text_queue_.full())
        text_queue_.emplace_back(TextEntry{ sv, x, y, color, size, nullptr });
}

void Renderer::draw_text(std::string_view sv,
    int x,
    int y,
    uint16_t color,
    const Font_Def& font,
    float size)
{
    if (!text_queue_.full())
        text_queue_.emplace_back(TextEntry{ sv, x, y, color, size, &font });
}

} // namespace pulse2d
