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

#include <luya/engine.h>        // for Engine
#include <luya/physics/math.h>  // for Vec2
#include <luya/physics/world.h> // for World

/*****************************************************
 *   _   _   _ __   __ _
 *  | | | | | |\ \ / // \
 *  | | | | | | \ V // _ \
 *  | |_| |_| |  | |/ ___ \
 *  |___|\___/   |_/_/   \_\
 *
 *        2D Gaming Engine
 *          on Teensy 4.1
 *****************************************************/

// Teensy entry point - game logic goes here.
// For the SDL2 host sample see sample/main.cc.

static luya::Engine engine;
static luya::physics::World world({ 0.0f, -10.0f }, 10);

void setup()
{
    engine.init();
}

void loop()
{
    world.step(1.0f / 60.0f);
    engine.tick(world);
}
