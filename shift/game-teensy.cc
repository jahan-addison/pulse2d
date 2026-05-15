/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <pulse2d/graphics/body.h>  // for Body
#include <pulse2d/graphics/math.h>  // for Vec2
#include <pulse2d/graphics/world.h> // for World
#include <pulse2d/pulse2d.h>        // for Pulse2d
#include <pulse2d/util.h>           // for HARDWARE_Deferred_Init

PULSE2D_DEFINE_ENGINE();

// @todo PULSE2D_BODY
PULSE2D_DEFINE pulse2d::graphics::Body planet;
PULSE2D_DEFINE pulse2d::graphics::Body spell;

// @todo PULSE2D_SPRITE
PULSE2D_DEFINE pulse2d::Sprite planet_sprite;
PULSE2D_DEFINE pulse2d::Sprite spell_sprite;
PULSE2D_DEFINE pulse2d::Sprite explode_sprite;

PULSE2D_DEFINE bool exploded = false;

PULSE2D_DEFINE constexpr float k_dt = 1.0f / 60.0f;

void setup()
{
    Serial.begin(115200);

    PULSE2D_POLL_SERIAL_CONNECTION();

    PULSE2D_INIT(0.0f, 0.0f, 10);

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
    world->step(k_dt);

    if (!exploded && !world->arbiters.empty())
        exploded = true;

#ifdef DEBUG
    static uint32_t frame = 0;
    if (++frame % 300 == 0)
        Serial.printf("stack used: %lu bytes\n", pulse2d::teensy::stack_used());
#endif

    auto& renderer = engine->renderer();

    {
        auto [sx, sy] =
            renderer.project_coordinates(planet.position.x, planet.position.y);
        const pulse2d::Sprite& active =
            exploded ? explode_sprite : planet_sprite;
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