/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 *
 * This file is part of pulse2d.
 * This software is released under the MIT License. You may use,
 * distribute, and modify this code under the terms of the license.
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <pulse2d/display.h>
#include <pulse2d/util.h> //  PULSE2D_TEENSY

#include <cstdint> // for uint16_t, uint8_t

#if !defined(PULSE2D_TEENSY)
#include <SDL2/SDL.h> // for SDL_Init, SDL_Quit, SDL_INIT_VIDEO
#endif

/****************************************************************************
 * Display
 *
 * On Teensy 4.1 (i.MX RT1062) the target hardware is the PJRC ILI9341 320x240
 * TFT, driven by ILI9341_t3. On host the driver opens an SDL2 window at the
 * same logical resolution scaled up by config::scale.
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
 * @brief Create window, renderer, and streaming RGB565 texture; clear to black
 */
void Display::init()
{
#if defined(PULSE2D_TEENSY)
    // Deselect the XPT2046 touchscreen before SPI init. T_CS (pin 8) shares
    // MOSI/SCK/MISO with the ILI9341 — if left floating it corrupts all SPI.
    pinMode(pins::touch_cs, OUTPUT);
    digitalWrite(pins::touch_cs, HIGH);
    Serial.printf(
        "display: CS=%u DC=%u RST=%u MOSI=%u SCK=%u MISO=%u T_CS=%u\n",
        pins::tft_cs,
        pins::tft_dc,
        pins::tft_rst,
        pins::tft_mosi,
        pins::tft_sck,
        pins::tft_miso,
        pins::touch_cs);
    Serial.flush();
    tft_.begin();
    tft_.setRotation(1);
    clear();
#else
    SDL_Init(SDL_INIT_VIDEO);
    window_ = SDL_CreateWindow("pulse2d \xe2\x80\x94 SDL2 Display",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config::width * config::scale,
        config::height * config::scale,
        SDL_WINDOW_SHOWN);
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderer_, config::width, config::height);
    texture_ = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        config::width,
        config::height);
    clear();
#endif
}

/**
 * @brief Upload framebuffer to the SDL texture and present the frame
 */
void Display::blit(frame_buffer_t const* framebuffer, [[maybe_unused]] int len)
{
#if defined(PULSE2D_TEENSY)
    tft_.writeRect(0, 0, config::width, config::height, framebuffer->data());
#else
    SDL_UpdateTexture(texture_,
        nullptr,
        framebuffer,
        config::width * static_cast<int>(sizeof(uint16_t)));
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
#endif
}

/**
 * @brief Expand RGB565 color to RGB888 and clear the SDL renderer
 */
void Display::clear(uint16_t color)
{
#if defined(PULSE2D_TEENSY)
    tft_.fillScreen(color);
#else
    uint8_t const r = static_cast<uint8_t>(((color >> 11) & 0x1F) << 3);
    uint8_t const g = static_cast<uint8_t>(((color >> 5) & 0x3F) << 2);
    uint8_t const b = static_cast<uint8_t>((color & 0x1F) << 3);
    SDL_SetRenderDrawColor(renderer_, r, g, b, 0xFF);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
#endif
}

} // namespace pulse2d
