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
#include <luya/physics/body.h>  // for Body
#include <luya/physics/math.h>  // for Vec2
#include <luya/physics/world.h> // for World

static luya::Engine engine;
static luya::physics::World world({ 0.0f, 0.0f }, 10);

static luya::physics::Body planet;
static luya::physics::Body spell;

static luya::Sprite planet_sprite;
static luya::Sprite spell_sprite;
static luya::Sprite explode_sprite;

static bool exploded = false;

constexpr float k_dt = 1.0f / 60.0f;

void setup()
{
    engine.init();

    planet.position = { 3.5f, 0.0f };
    planet.width = { 1.0f, 1.0f };

    spell.set_mass({ 1.0f, 0.5f }, 1.0f);
    spell.position = { -5.0f, -0.1111f };
    spell.velocity = { 0.5f, 0.0f };

    world.add(&planet);
    world.add(&spell);

    planet_sprite =
        engine.storage().load_sprite("Water_Ball_Frame_07.bin", 96, 96);
    spell_sprite =
        engine.storage().load_sprite("Fire_Spell_Frame_07.bin", 64, 36);
    explode_sprite =
        engine.storage().load_sprite("Fire_Explosion_Frame_07.bin", 96, 96);
}

void loop()
{
    world.step(k_dt);

    if (!exploded and not world.arbiters.empty()) {
        exploded = true;
    }

    auto& renderer = engine.renderer();

    {
        auto [sx, sy] =
            renderer.project_coordinates(planet.position.x, planet.position.y);
        const luya::Sprite& active = exploded ? explode_sprite : planet_sprite;
        renderer.add_sprite(&active,
            static_cast<int16_t>(sx - active.width / 2),
            static_cast<int16_t>(sy - active.height / 2));
    }

    if (!exploded) {
        auto [sx, sy] =
            renderer.project_coordinates(spell.position.x, spell.position.y);
        renderer.add_sprite(&spell_sprite,
            static_cast<int16_t>(sx - spell_sprite.width / 2),
            static_cast<int16_t>(sy - spell_sprite.height / 2),
            3.111f);
    }

    engine.tick(world);
}
