/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cstddef>           // for size_t
#include <cstdint>           // for uint16_t, int16_t
#include <etl/array.h>       // for array
#include <etl/vector.h>      // for vector
#include <pulse2d/display.h> // for Display, config::width, config::height
#include <pulse2d/sprite.h>  // for Sprite
#include <pulse2d/util.h>    // for PULSE2D_PRIVATE

#include <pulse2d/graphics/world.h> // for World

namespace pulse2d {

namespace pulse2d {
class Body;
} // namespace pulse2d

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

// The framebuffer is placed in OCRAM (secondary 512 KB bank) on Teensy
static PULSE2D_EXTMEM frame_buffer_t s_framebuffer;

/**
 * @brief
 * RGB565 software renderer. Contains the framebuffer, physics body
 * rasterization, sprite blitting, and display output each frame.
 */
class Renderer
{
  public:
    explicit Renderer()
        : framebuffer_(nullptr)
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
    void draw(graphics::World const& world);

  private:
    void draw_body(graphics::Body const* body);
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
  PULSE2D_PRIVATE:
    Display display_;
    frame_buffer_t* framebuffer_;
    etl::vector<Entry, k_max_sprites> sprite_queue_;
};

// clang-format on

} // namespace pulse2d
