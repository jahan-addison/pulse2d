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

#include <luya/display/adafruit.h>
#include <luya/display/display.h>

#if defined(__IMXRT1062__)

#include <cstdint> // for uint16_t

/****************************************************************************
 * Adafruit_Display
 *
 * ILI9341 TFT display driver for Teensy 4.1. Uses ILI9341_t3 by default.
 * Define LUYA_USE_ILI9341_T3N to switch to the DMA-capable variant. Pin
 * assignments for the Teensy 4.1 SPI0 bus are in `pins::'.
 *
 ****************************************************************************/

namespace luya::display {

/**
 * @brief Initialize the ILI9341, set landscape rotation, and clear to black
 */
void Adafruit_Display::init()
{
    tft_.begin();
    tft_.setRotation(1);
    clear();
}

/**
 * @brief Fill the screen with a solid RGB565 color
 */
void Adafruit_Display::clear(uint16_t color)
{
    tft_.fillScreen(color);
}

/**
 * @brief DMA-blit a full-screen RGB565 framebuffer to the ILI9341
 */
void Adafruit_Display::blit(frame_buffer_t const* framebuffer, int len)
{
    (void)len;
    tft_.writeRect(0, 0, config::width, config::height, framebuffer->data());
}

} // namespace luya::display

#endif // defined(__IMXRT1062__)
