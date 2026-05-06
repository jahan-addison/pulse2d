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
#include <memory>      // for unique_ptr

/****************************************************************************
 * Display
 *
 * Abstract display driver interface and compile-time dimensions.
 * config:: holds the ILI9341 native resolution (320x240) and the SDL2
 * desktop scale factor. display::factory() returns the platform-appropriate
 * driver.
 *
 *  Example:
 *
 *   auto display = luya::display::factory();
 *   display->init();
 *   display->blit(framebuffer, config::width * config::height);
 *
 ****************************************************************************/

namespace luya::display {

/**
 * @brief
 *   ILI9341 native display resolution and SDL2 desktop scale factor
 */
namespace config {
inline constexpr int width = 320;
inline constexpr int height = 240;
inline constexpr int scale = 3; // SDL2 window scale (960x720)
} // namespace config

template<int x = config::width, int y = config::height>
using Frame_Buffer = etl::array<uint16_t, x * y>;

using frame_buffer_t = Frame_Buffer<>;

/**
 * @brief
 * Abstract display driver
 */
class Display
{
  protected:
    Display() = default;

  public:
    virtual ~Display() = default;
    Display(Display const&) = delete;
    Display& operator=(Display const&) = delete;

  public:
    virtual void init() = 0;
    virtual void clear(uint16_t color = 0x0000) = 0;
    virtual void blit(frame_buffer_t const* framebuffer, int len) = 0;
};

[[nodiscard]] std::unique_ptr<Display> factory();

} // namespace luya::display
