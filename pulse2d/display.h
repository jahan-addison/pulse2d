/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cstdint>        // for uint16_t
#include <etl/array.h>    // for array
#include <pulse2d/util.h> // for PULSE2D_PRIVATE, PULSE2D_TEENSY
#if defined(PULSE2D_TEENSY)
#include <ILI9341_t3.h>
#else
#include <SDL2/SDL.h>   // for SDL_Quit
#include <SDL_render.h> // for SDL_CreateRenderer, SDL_DestroyRenderer
#include <SDL_video.h>  // for SDL_CreateWindow, SDL_DestroyWindow
#endif

/****************************************************************************
 * Display
 *
 * On Teensy the target hardware is the PJRC ILI9341 320x240, and on host the
 * driver opens an SDL2 window at the same logical resolution scaled up by
 * config::scale.
 *
 * Example:
 *
 *   pulse2d::Display display;
 *   display.init();
 *   display.blit(framebuffer, config::width * config::height);
 *
 ****************************************************************************/

namespace pulse2d {

/**
 * @brief ILI9341 native display resolution and SDL2 desktop scale factor
 */
namespace config {
inline constexpr int width = 320;
inline constexpr int height = 240;
inline constexpr int scale = 3; // SDL2 window scale (960x720)
} // namespace config

template<int x = config::width, int y = config::height>
using Frame_Buffer = etl::array<uint16_t, x * y>;

using frame_buffer_t = Frame_Buffer<>;

#if defined(PULSE2D_TEENSY)
namespace pins {
inline constexpr uint8_t tft_cs = 10;
inline constexpr uint8_t tft_dc = 9;
inline constexpr uint8_t tft_rst = 6;
inline constexpr uint8_t tft_mosi = 11;
inline constexpr uint8_t tft_sck = 13;
inline constexpr uint8_t tft_miso = 12;
inline constexpr uint8_t touch_cs =
    8; // XPT2046 CS — must be HIGH before SPI init
} // namespace pins

#endif

/**
 * @brief Display driver
 *
 *  Teensy: ILI9341_t3 over SPI0.
 *  Host: SDL2 window at config::width × config::height × config::scale.
 */
class Display
{
  public:
#if defined(PULSE2D_TEENSY)
    Display()
        : tft_(pins::tft_cs,
              pins::tft_dc,
              pins::tft_rst,
              pins::tft_mosi,
              pins::tft_sck,
              pins::tft_miso)
    {
    }
#else
    Display() = default;
    ~Display()
    {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_) {
            SDL_DestroyWindow(window_);
        }
        SDL_Quit();
    }
#endif

    Display(Display const&) = delete;
    Display& operator=(Display const&) = delete;

  public:
    void init();
    void clear(uint16_t color = 0x0000);
    void blit(frame_buffer_t const* framebuffer, int len);

  private:
#if defined(PULSE2D_TEENSY)
    ILI9341_t3 tft_;
#else
    SDL_Window* window_{ nullptr };
    SDL_Renderer* renderer_{ nullptr };
    SDL_Texture* texture_{ nullptr };
#endif
};

} // namespace pulse2d
