/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

PULSE2D_START_PULSE();

PULSE2D_ENABLE_SEESAW_GAMEPAD();

/*
 * Level:
 * 2 Entities
 * 3 Sprites
 */
PULSE2D_DEFINE_SCENE(Sample_Level, 2, 3);

PULSE2D_GAME_SCENES(Sample_Level);

PULSE2D_DEFINE bool exploded = false;
PULSE2D_DEFINE bool fired = false;

PULSE2D_ON_GAMESCENE_START(Sample_Level)
{
    PULSE2D_SPRITE(planet_sprite, "planet.bin", 96, 96);
    PULSE2D_SPRITE(spell_sprite, "spell.bin", 64, 36);
    PULSE2D_SPRITE(explode_sprite, "explosion.bin", 96, 96);

    PULSE2D_STATIC_BODY(planet_object,
        {
            .position = { 3.5f, 0.0f },
              .width = { 1.0f, 1.0f }
    });

    PULSE2D_DYNAMIC_BODY(spell_object,
        {
            .position = { -3.5f, 2.5111f },
            .velocity = { 0.0f,  0.0f    },
            .width = { 1.0f,  0.5f    },
            .mass = 1.0f
    });
}

PULSE2D_ON_GAMESCENE(Sample_Level)
{
    PULSE2D_TICK_WORLD(Sample_Level);
    PULSE2D_POLL_SEESAW_GAMEPAD();
    auto& spell = PULSE2D_GET_BODY(spell_object);

    PULSE2D_ON_COLLISION()
    {
        if (!exploded) {
            exploded = true;
            spell.set_velocity({ 0.0f, 0.0f });
        }
    }

    if (!fired)
        SEESAW_ARCADE_DIRECTIONAL_MOVEMENT_INVERTED(spell_object, 5.22f);

    if (!fired and SEESAW_BUTTON_INPUT(SEESAW_A)) {
        spell.set_velocity({ 12.555f, 0.0f });
        fired = true;
    }

    // reset?
    if (spell.position.x > 5.5f or SEESAW_BUTTON_INPUT(SEESAW_START)) {
        fired = false;
        exploded = false;
        PULSE2D_DEFER_SCENE(Sample_Level);
    }

    if (exploded)
        PULSE2D_DRAW(planet_object, explode_sprite);
    else
        PULSE2D_DRAW(planet_object, planet_sprite);

    PULSE2D_DRAW(spell_object, spell_sprite, 3.111f);
    PULSE2D_RENDER(active_scene);
}

PULSE2D_ON_GAMESTART()
{
    Serial.begin(115200);

    // PULSE2D_POLL_SERIAL_CONNECTION();

    PULSE2D_INIT(0.0f, 0.0f, 10);

    PULSE2D_START_SEESAW_GAMEPAD();

    PULSE2D_SET_SCENE(Sample_Level);
}

PULSE2D_ON_GAMELOOP()
{
    PULSE2D_TICK_GAMESCENE();
}