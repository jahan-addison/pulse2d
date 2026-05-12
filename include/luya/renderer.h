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

#include <cstddef>              // for size_t
#include <cstdint>              // for uint16_t, int16_t
#include <etl/array.h>          // for array
#include <etl/vector.h>         // for vector
#include <luya/common.h>        // for LUYA_PRIVATE
#include <luya/display.h>       // for Display, config::width, config::height
#include <luya/physics/world.h> // for World
#include <luya/sprite.h>        // for Sprite

namespace luya {

namespace physics {
class Body;
} // namespace physics

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

// The framebuffer is placed in OCRAM (secondary 512 KB bank) on Teensy
static LUYA_EXTMEM frame_buffer_t s_framebuffer;

/**
 * @brief
 * RGB565 software renderer. Contains the framebuffer, physics body
 * rasterization, sprite blitting, and display output each frame.
 */
class Renderer
{
  public:
    explicit Renderer(Display& display)
        : display_(display)
        , framebuffer_(nullptr)
        , sprite_queue_()
    {
    }
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;

  public:
    struct Screen
    {
        int16_t x;
        int16_t y;
    };

  public:
    /**
     * @brief Set the OCRAM framebuffer to this renderer instance
     */
    inline void init() { framebuffer_ = &s_framebuffer; }

    /**
     * @brief Project a world-space position to screen pixel coordinates
     */
    inline static Screen project_coordinates(float wx, float wy)
    {
        return { static_cast<int16_t>(
                     static_cast<int>(wx * k_pixels_per_unit) + k_center_x),
            static_cast<int16_t>(
                k_center_y - static_cast<int>(wy * k_pixels_per_unit)) };
    }
    /**
     * @brief Blit the framebuffer to the display driver
     */
    inline void render() { display_.blit(framebuffer_, k_width * k_height); }

  public:
    void clear(uint16_t color = 0x0000);
    void add_sprite(Sprite const* sprite,
        int16_t x,
        int16_t y,
        float angle_rad = 0.0f);
    void draw(physics::World const& world);

  private:
    void draw_body(physics::Body const* body);
    void draw_sprite(Sprite const* sprite,
        int16_t x,
        int16_t y,
        float angle_rad);
    void fill_rect(int x, int y, int w, int h, uint16_t color);

  public:
    /**
     * @brief
     * When true, draw() overlays a white bounding-box rectangle for
     * every body in the world. Enable during to visualize
     * collision shapes.
     */
    bool show_debug_rects{ false };

  private:
    static constexpr int k_width = config::width;
    static constexpr int k_height = config::height;
    static constexpr float k_pixels_per_unit = 30.0f;
    static constexpr int k_center_x = k_width / 2;
    static constexpr int k_center_y = k_height / 2;

    static constexpr std::size_t k_max_sprites = 64;

    // clang-format off
    struct Entry
    {
        Sprite const* sprite;
        int16_t x;
        int16_t y;
        float angle_rad;
    };
  LUYA_PRIVATE:
    Display& display_;
    frame_buffer_t* framebuffer_;
    etl::vector<Entry, k_max_sprites> sprite_queue_;
};

// clang-format on

} // namespace luya
