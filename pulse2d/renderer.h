/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cstddef>                 // for size_t
#include <cstdint>                 // for int16_t, uint16_t
#include <etl/vector.h>            // for vector
#include <pulse2d/display.h>       // for frame_buffer_t, Display, height, width
#include <pulse2d/graphics/math.h> // for Vec2
#include <pulse2d/util.h>          // for PULSE2D_EXTMEM, PULSE2D_PRIVATE
#include <string_view>             // for string_view (TextEntry)

namespace pulse2d {
struct Sprite;
namespace graphics {
class Body;
class World;
}
}

namespace pulse2d {

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
    PULSE2D_INLINE void init() { framebuffer_ = &s_framebuffer; }

    /**
     * @brief Project a world-space position to screen pixel coordinates
     */
    PULSE2D_INLINE static Screen units_to_pixels(float wx, float wy)
    {
        return { static_cast<int16_t>(
                     static_cast<int>(wx * k_pixels_per_unit) + k_center_x),
            static_cast<int16_t>(
                k_center_y - static_cast<int>(wy * k_pixels_per_unit)) };
    }
    /**
     * @brief Blit the framebuffer to the display driver
     */
    PULSE2D_INLINE void render()
    {
        display_.blit(framebuffer_, k_width * k_height);
    }

  public:
    /**
     * @brief
     * Descriptor for a custom bitmap font produced by font2bytes or
     * a compatible tool. The data table must be column-major, 1 bit per
     * pixel, with cell_w bytes per glyph and cell_h rows per column.
     * Keep the table in flash with PULSE2D_FLASHMEM.
     */
    struct Font_Def
    {
        const uint8_t* data;
        uint8_t cell_w;
        uint8_t cell_h;
        uint8_t first_char;
    };

    // clang-format off

    /**
     * @brief Named RGB565 color constants. Cast to uint16_t to pass to any
     * color parameter, or use text_color() for arbitrary values.
     */
    enum class Color : uint16_t
    {
        Black       = 0x0000,
        Navy        = 0x000F,
        DarkGreen   = 0x03E0,
        DarkCyan    = 0x03EF,
        Maroon      = 0x7800,
        Purple      = 0x780F,
        Olive       = 0x7BE0,
        LightGrey   = 0xC618,
        DarkGrey    = 0x7BEF,
        Blue        = 0x001F,
        Green       = 0x07E0,
        Cyan        = 0x07FF,
        Red         = 0xF800,
        Yellow      = 0xFFE0,
        White       = 0xFFFF,
        Orange      = 0xFD20,
        GreenYellow = 0xAFE5,
        Magenta     = 0xF81F,  // also the sprite transparency key
        Pink        = 0xF81F,
    };
    // clang-format on

    /**
     * @brief Pack RGB components into an RGB565 color value for draw_text.
     */
    PULSE2D_INLINE static uint16_t text_color(uint8_t r, uint8_t g, uint8_t b)
    {
        return static_cast<uint16_t>(
            ((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
    }

    static constexpr int k_screen_w = config::width;

    /**
     * @brief
     * Draw text into the framebuffer using the built-in 5x7 font.
     * Supports '\n' for line breaks and integer size scaling.
     */
    void draw_text(std::string_view sv,
        int x,
        int y,
        uint16_t color,
        float size = 1.0f);

    /**
     * @brief Draw text into the framebuffer using a custom Font_Def.
     */
    void draw_text(std::string_view sv,
        int x,
        int y,
        uint16_t color,
        const Font_Def& font,
        float size = 1.0f);

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
    void draw_glyph(uint8_t glyph_idx,
        const uint8_t* data,
        uint8_t cell_w,
        uint8_t cell_h,
        int x,
        int y,
        uint16_t color,
        float scale);

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

    static constexpr float k_pixels_per_unit = config::pixels_per_unit;

    static constexpr int k_center_x = k_width / 2;
    static constexpr int k_center_y = k_height / 2;

    static constexpr std::size_t k_max_sprites = 64;
    static constexpr std::size_t k_max_text_entries = 8;

    // clang-format off
    struct Entry
    {
        Sprite const* sprite;
        int16_t x;
        int16_t y;
        float angle_rad;
    };

    struct TextEntry
    {
        std::string_view sv;
        int x;
        int y;
        uint16_t color;
        float size;
        const Font_Def* font;  // nullptr = built-in 5x7
    };
  PULSE2D_PRIVATE:
    Display& display_;
    frame_buffer_t* framebuffer_;
    etl::vector<Entry, k_max_sprites> sprite_queue_;
    etl::vector<TextEntry, k_max_text_entries> text_queue_;
};

// clang-format on

} // namespace pulse2d
