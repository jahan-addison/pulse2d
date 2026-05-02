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

/****************************************************************************
 * Sample — fire spell collision
 *
 * A fire spell body falls under gravity and strikes a static ground body.
 * On contact, i.e. when World::arbiters becomes non-empty, the spell sprite
 * is replaced by an explosion sprite at the same world position.
 *
 * Sprites are loaded from sample/images/ via Storage::load_sprite() and
 * scaled to fit the 320x240 logical display. world_to_screen() maps each
 * body's world-space position to a pixel coordinate for add_sprite().
 *
 ****************************************************************************/

#include <luya/engine.h>        // for Engine
#include <luya/physics/body.h>  // for Body
#include <luya/physics/math.h>  // for Vec2
#include <luya/physics/world.h> // for World

#include <SDL2/SDL.h> // for SDL_PollEvent, SDL_QUIT

int main()
{
    luya::Engine engine{};
    engine.init();

    luya::physics::World world({ 0.0f, -10.0f }, 10);

    // static ground platform — default Body has inv_mass = 0
    luya::physics::Body ground;
    ground.position = { 3.5f, -0.5f };
    ground.width = { 0.5f, 0.5f };

    // dynamic spell body — set() assigns mass and inertia
    luya::physics::Body spell;
    spell.set({ 0.3f, 0.3f }, 1.0f);
    spell.position = { -4.2f, -1.0f };
    spell.velocity = { -0.0f, 10.0f };

    world.add(&ground);
    world.add(&spell);

    luya::Sprite world_sprite = engine.storage().load_sprite(
        "sample/images/Fire_World_Frame.png", 320, 80);
    luya::Sprite spell_sprite = engine.storage().load_sprite(
        "sample/images/Fire_Spell_Frame_07.png", 64, 36);
    luya::Sprite explode_sprite = engine.storage().load_sprite(
        "sample/images/Fire_Explosion_Frame_07.png", 96, 96);

    bool exploded = false;

    constexpr float k_dt = 1.0f / 60.0f;

    SDL_Event event;
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        world.step(k_dt);

        if (!exploded && !world.arbiters.empty()) {
            exploded = true;
        }

        auto& renderer = engine.renderer();

        {
            auto [sx, sy] =
                renderer.world_to_screen(ground.position.x, ground.position.y);
            renderer.add_sprite(&world_sprite,
                static_cast<int16_t>(sx - world_sprite.width / 2),
                static_cast<int16_t>(sx - world_sprite.height / 2));
        }

        {
            auto [sx, sy] =
                renderer.world_to_screen(spell.position.x, spell.position.y);
            const luya::Sprite& active =
                exploded ? explode_sprite : spell_sprite;
            renderer.add_sprite(&active,
                static_cast<int16_t>(sx - active.width / 2),
                static_cast<int16_t>(sy - active.height / 2));
        }

        engine.tick(world);
    }

    return 0;
}
