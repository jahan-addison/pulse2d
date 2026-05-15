/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <pulse2d/audio.h>    // for Audio
#include <pulse2d/display.h>  // for Display
#include <pulse2d/renderer.h> // for Renderer
#include <pulse2d/storage.h>  // for Storage
#include <pulse2d/util.h>     // for utilities

namespace pulse2d::graphics {
class World;
} // namespace pulse2d::graphics

namespace pulse2d {

/********************************************************************************
 * @brief Pulse2d
 *
 * Primary control of the display, audio, and storage components. Construct
 * once, call init() from Teensy setup(), and tick() on every loop() iteration.
 *
 ********************************************************************************/
class Pulse2d
{
  public:
    Pulse2d() = default;
    Pulse2d(Pulse2d const&) = delete;
    Pulse2d& operator=(Pulse2d const&) = delete;

  public:
    /**
     * @brief Hardware and component initialization
     */
    inline void init()
    {
        storage_.init();
        PULSE2D_DEBUG_SERIAL("Pulse2d: storage OK");
        audio_.init();
        PULSE2D_DEBUG_SERIAL("Pulse2d: audio OK");
        display_.init();
        PULSE2D_DEBUG_SERIAL("Pulse2d: display OK");
        renderer_.init();
        PULSE2D_DEBUG_SERIAL("Pulse2d: renderer OK");
    }
    /**
     * @brief Per-frame update, rendering and logic each loop() tick
     */
    inline void tick(graphics::World const& world)
    {
        renderer_.clear();
        renderer_.draw(world);
        renderer_.render();
    }

    Renderer& renderer() { return renderer_; }
    Storage& storage() { return storage_; }

  private:
    Display display_;
    Renderer renderer_;
    Audio audio_;
    Storage storage_;
};

} // namespace pulse2d
