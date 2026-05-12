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

#include <luya/common.h>        // for Deferred_Init
#include <luya/engine.h>        // for Engine
#include <luya/physics/body.h>  // for Body
#include <luya/physics/math.h>  // for Vec2
#include <luya/physics/world.h> // for World

// NOLINTBEGIN
static luya::Deferred_Init<luya::Engine> engine;
static luya::Deferred_Init<luya::physics::World> world;

static luya::physics::Body planet;
static luya::physics::Body spell;

static luya::Sprite planet_sprite;
static luya::Sprite spell_sprite;
static luya::Sprite explode_sprite;

static bool exploded = false;

constexpr float k_dt = 1.0f / 60.0f;

extern "C" uint32_t _ebss;
extern "C" uint32_t _estack;

uint32_t stack_used()
{
    const uint32_t* p = (const uint32_t*)&_ebss;
    uint32_t count = 0;
    while (*p++ == 0xA5A5A5A5)
        count += 4;
    return (uint32_t)&_estack - (uint32_t)&_ebss - count; // bytes consumed
}

void setup()
{
    Serial.begin(115200);
#ifdef DEBUG
    while (!Serial)
        ; // DEBUG: wait indefinitely so all setup() output is visible
#else
    while (!Serial && millis() < 3000)
        ; // release builds: give up after 3 s (no monitor attached)
#endif

    Serial.println("setup: serial ok");

    // Construct engine and world now that the Arduino runtime is ready.
    Serial.println("setup: constructing Engine");
    engine.emplace();
    Serial.println("setup: constructing World");
    world.emplace(luya::physics::Vec2{ 0.0f, 0.0f }, 10);
    Serial.println("setup: objects constructed");

    Serial.println("setup: calling engine->init()");
    Serial.flush();

    engine->init();
    Serial.println("setup: engine->init() done");

    planet.position = { 3.5f, 0.0f };
    planet.width = { 1.0f, 1.0f };

    spell.set_mass({ 1.0f, 0.5f }, 1.0f);
    spell.position = { -5.0f, -0.1111f };
    spell.velocity = { 0.5f, 0.0f };

    world->add(&planet);
    world->add(&spell);

    planet_sprite =
        engine->storage().load_sprite("Water_Ball_Frame_07.bin", 96, 96);
    Serial.printf("setup: planet_sprite  data=%p w=%u h=%u\n",
        planet_sprite.data,
        planet_sprite.width,
        planet_sprite.height);

    spell_sprite =
        engine->storage().load_sprite("Fire_Spell_Frame_07.bin", 64, 36);
    Serial.printf("setup: spell_sprite   data=%p w=%u h=%u\n",
        spell_sprite.data,
        spell_sprite.width,
        spell_sprite.height);

    explode_sprite =
        engine->storage().load_sprite("Fire_Explosion_Frame_07.bin", 96, 96);
    Serial.printf("setup: explode_sprite data=%p w=%u h=%u\n",
        explode_sprite.data,
        explode_sprite.width,
        explode_sprite.height);

    Serial.println("setup: done");
}

void loop()
{
#ifdef DEBUG
    static bool first_loop = true;
    if (first_loop) {
        Serial.println("loop: running");
        Serial.flush();
        first_loop = false;
    }
#endif

    world->step(k_dt);

    if (!exploded and not world->arbiters.empty()) {
        exploded = true;
    }

#ifdef DEBUG
    static uint32_t frame = 0;
    if (++frame % 300 == 0) { // every 5 seconds at 60 Hz
        Serial.printf("stack used: %lu bytes\n", stack_used());
    }
#endif

    auto& renderer = engine->renderer();

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

    engine->tick(*world);
}
// NOLINTEND