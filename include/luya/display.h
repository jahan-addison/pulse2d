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

#include <cstdint>     // for uint16_t
#include <etl/array.h> // for array
#include <luya/util.h> // for LUYA_PRIVATE, LUYA_TEENSY

/****************************************************************************
 * Display
 *
 * On Teensy 4.1 (i.MX RT1062) the target hardware is the PJRC ILI9341 320x240
 * TFT, driven by ILI9341_t3. On host the driver opens an SDL2 window at the
 * same logical resolution scaled up by config::scale.
 *
 * Example:
 *
 *   luya::Display display;
 *   display.init();
 *   display.blit(framebuffer, config::width * config::height);
 *
 ****************************************************************************/

#if defined(LUYA_TEENSY)
#include <ILI9341_t3.h>
#else
#include <SDL2/SDL.h>   // for SDL_Quit
#include <SDL_render.h> // for SDL_CreateRenderer, SDL_DestroyRenderer
#include <SDL_video.h>  // for SDL_CreateWindow, SDL_DestroyWindow
#endif

namespace luya {

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

#if defined(LUYA_TEENSY)
/**
 * @brief ILI9341 pin assignments for the Teensy 4.1 SPI0 bus
 *
 *  | Display pin | Teensy 4.1 pin |
 *  |-------------|---------------|
 *  | CS          | 10            |
 *  | D/C         | 9             |
 *  | RESET       | 3.3V (hardwired, not a GPIO — pass 255 to ILI9341_t3) |
 *  | MOSI/SDI    | 11            |
 *  | SCK         | 13            |
 *  | MISO/SDO    | 12            |
 *  | T_CS        | 8  (touch CS — driven HIGH to keep XPT2046 off the bus) |
 */
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

// Compile-time pin validation

static_assert(pins::tft_mosi == 11,
    "luya: tft_mosi must be 11 (hardware SPI0 MOSI on Teensy 4.1)");
static_assert(pins::tft_sck == 13,
    "luya: tft_sck must be 13 (hardware SPI0 SCK on Teensy 4.1)");
static_assert(pins::tft_miso == 12,
    "luya: tft_miso must be 12 (hardware SPI0 MISO on Teensy 4.1)");

static_assert(pins::tft_cs != pins::tft_dc,
    "luya: tft_cs and tft_dc must be different pins");
static_assert(pins::tft_cs != pins::tft_rst,
    "luya: tft_cs and tft_rst must be different pins");
static_assert(pins::tft_dc != pins::tft_rst,
    "luya: tft_dc and tft_rst must be different pins");
static_assert(pins::tft_cs != pins::touch_cs,
    "luya: tft_cs and touch_cs must be different pins");
static_assert(pins::tft_dc != pins::touch_cs,
    "luya: tft_dc and touch_cs must be different pins");
static_assert(pins::tft_rst != pins::touch_cs,
    "luya: tft_rst and touch_cs must be different pins");

static_assert(pins::tft_cs < 42,
    "luya: tft_cs is not a valid Teensy 4.1 pin (0-41)");
static_assert(pins::tft_dc < 42,
    "luya: tft_dc is not a valid Teensy 4.1 pin (0-41)");
static_assert(pins::tft_rst < 42,
    "luya: tft_rst is not a valid Teensy 4.1 pin (0-41)");
static_assert(pins::touch_cs < 42,
    "luya: touch_cs is not a valid Teensy 4.1 pin (0-41)");

#endif

/**
 * @brief Display driver
 *
 *   Teensy: ILI9341_t3 over SPI0.
 *   Host:   SDL2 window at config::width × config::height × config::scale.
 */
class Display
{
  public:
#if defined(LUYA_TEENSY)
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
#if defined(LUYA_TEENSY)
    ILI9341_t3 tft_;
#else
    SDL_Window* window_{ nullptr };
    SDL_Renderer* renderer_{ nullptr };
    SDL_Texture* texture_{ nullptr };
#endif
};

} // namespace luya
