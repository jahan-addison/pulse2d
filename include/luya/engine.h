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

#include <luya/audio.h>           // for Audio
#include <luya/display/display.h> // for Display
#include <luya/renderer.h>        // for Renderer
#include <luya/storage.h>         // for Storage
#include <memory>                 // for unique_ptr

namespace luya::physics {
class World;
} // namespace luya::physics

namespace luya {

/**
 * @brief
 *   Luya 2D Engine
 *
 *   Controls the display, audio, and storage components. Construct once,
 *   call init() from Teensy setup(), and tick() on every loop() iteration.
 *
 *   The display driver is selected at compile time via display::factory()
 */
class Engine
{
  public:
    Engine()
        : display_(display::factory())
        , renderer_(*display_)
    {
    }
    Engine(Engine const&) = delete;
    Engine& operator=(Engine const&) = delete;

  public:
    /**
     * @brief Hardware and component initialization
     */
    inline void init()
    {
        storage_.init();
        audio_.init();
        display_->init();
    }
    /**
     * @brief Per-frame update, drive rendering and logic each loop() tick
     */
    inline void tick(physics::World& world)
    {
        renderer_.clear();
        renderer_.draw(world);
        renderer_.render();
    }

    Renderer& renderer() { return renderer_; }
    Storage& storage() { return storage_; }

  private:
    std::unique_ptr<display::Display> display_;
    Renderer renderer_;
    Audio audio_;
    Storage storage_;
};

} // namespace luya
