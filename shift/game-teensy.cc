/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

PULSE2D_START_PULSE();

/*
 * Level:
 * 2 Entities
 * 3 Sprites
 */
PULSE2D_DEFINE_LEVEL(Sample_Level, 2, 3);

PULSE2D_GAME_LEVELS(Sample_Level);

PULSE2D_DEFINE bool exploded = false;

PULSE2D_ON_GAMESCENE_START(Sample_Level)
{
    PULSE2D_SPAWN_STATIC_BODY("planet",
        {
            .position = { 3.5f, 0.0f },
              .width = { 1.0f, 1.0f }
    });

    PULSE2D_SPAWN_BODY("spell",
        {
            .position = { -3.5f, -0.1111f },
            .velocity = { 5.22f, 0.0f     },
            .width = { 1.0f,  0.5f     },
            .mass = 1.0f
    });
    PULSE2D_SET_SPRITE(planet_sprite, "planet.bin", 96, 96);
    PULSE2D_SET_SPRITE(spell_sprite, "spell.bin", 64, 36);
    PULSE2D_SET_SPRITE(explode_sprite, "explosion.bin", 96, 96);
}

PULSE2D_ON_GAMESCENE(Sample_Level)
{
    PULSE2D_TICK_WORLD(Sample_Level);
    PULSE2D_ON_COLLISION()
    {
        if (!exploded)
            exploded = true;
    }

    PULSE2D_PRINT_STACKSIZE();

    if (exploded)
        PULSE2D_DRAW("planet", explode_sprite);
    else
        PULSE2D_DRAW("planet", planet_sprite);

    PULSE2D_DRAW("spell", spell_sprite, 3.111f);
    PULSE2D_RENDER(active_scene);
}

PULSE2D_ON_GAMESTART()
{
    Serial.begin(115200);

    PULSE2D_POLL_SERIAL_CONNECTION();

    PULSE2D_INIT(0.0f, 0.0f, 10);

    PULSE2D_SET_SCENE(Sample_Level);
}

PULSE2D_ON_GAMELOOP()
{
    PULSE2D_TICK_GAMESCENE();
}