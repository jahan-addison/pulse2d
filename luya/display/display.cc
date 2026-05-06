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

#include <luya/display/display.h> // for Display, make

#include <memory> // for make_unique, unique_ptr

#if defined(__IMXRT1062__)
#include <luya/display/adafruit.h> // for Adafruit_Display
#else
#include <luya/display/sdl.h> // for SDL_Display
#endif

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
 * @brief Return the platform-appropriate display driver
 */
std::unique_ptr<Display> factory()
{
#if defined(__IMXRT1062__)
    return std::make_unique<Adafruit_Display>();
#else
    return std::make_unique<SDL_Display>();
#endif
}

} // namespace luya::display
