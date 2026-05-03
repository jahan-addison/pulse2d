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

#include <cstddef>                // for size_t
#include <cstdint>                // for uint16_t, int16_t
#include <etl/array.h>            // for array
#include <etl/vector.h>           // for vector
#include <luya/display/display.h> // for Display, config::width, config::height
#include <luya/physics/world.h>   // for World
#include <luya/sprite.h>          // for Sprite

namespace luya {

namespace physics {
class Body;
} // namespace physics

/****************************************************************************
 * Renderer
 *
 * Maintains a full-screen RGB565 framebuffer. Each frame:
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

/**
 * @brief
 * RGB565 software renderer. Contains the framebuffer, physics body
 * rasterization, sprite blitting, and display output each frame.
 */
class Renderer
{
  public:
    explicit Renderer(display::Display& display)
        : display_(display)
        , framebuffer_{}
        , sprite_queue_()
    {
    }
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;

  public:
    void clear(uint16_t color = 0x0000);
    void add_sprite(Sprite const* sprite,
        int16_t x,
        int16_t y,
        float angle_rad = 0.0f);
    void draw(physics::World const& world);
    void render();

    /**
     * @brief
     * When true, draw() overlays a white bounding-box rectangle for
     * every body in the world. Enable during to visualize
     * collision shapes:
     *
     *   engine.renderer().show_debug_rects = true;
     */
    bool show_debug_rects{ false };

    struct Screen
    {
        int16_t x;
        int16_t y;
    };

    Screen world_to_screen(float wx, float wy) const;

  private:
    void draw_body(physics::Body const* body);
    void draw_sprite(Sprite const* sprite,
        int16_t x,
        int16_t y,
        float angle_rad);
    void fill_rect(int x, int y, int w, int h, uint16_t color);

  private:
    static constexpr int k_width = display::config::width;
    static constexpr int k_height = display::config::height;
    static constexpr float k_pixels_per_unit = 30.0f;
    static constexpr int k_center_x = k_width / 2;
    static constexpr int k_center_y = k_height / 2;

    static constexpr std::size_t k_max_sprites = 64;

    struct Entry
    {
        const Sprite* sprite;
        int16_t x;
        int16_t y;
        float angle_rad;
    };

  private:
    display::Display& display_;
    display::frame_buffer_t framebuffer_;
    etl::vector<Entry, k_max_sprites> sprite_queue_;
};

} // namespace luya
