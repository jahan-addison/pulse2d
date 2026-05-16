/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

PULSE2D_START_PULSE();

PULSE2D_BODY planet;
PULSE2D_BODY spell;
PULSE2D_SPRITE planet_sprite;
PULSE2D_SPRITE spell_sprite;
PULSE2D_SPRITE explode_sprite;

PULSE2D_DEFINE bool exploded = false;

PULSE2D_ON_GAMESTART()
{
    Serial.begin(115200);

    PULSE2D_POLL_SERIAL_CONNECTION();

    PULSE2D_INIT(0.0f, 0.0f, 10);

    planet.set({
        .position = { 3.5f, 0.0f },
          .width = { 1.0f, 1.0f }
    });

    spell.set({
        .position = { -5.0f, -0.1111f },
        .velocity = { 0.5f,  0.0f     },
        .width = { 1.0f,  0.5f     },
        .mass = 1.0f,
    });

    spell.set_motion();

    PULSE2D_ADD_BODY(planet);
    PULSE2D_ADD_BODY(spell);

    PULSE2D_SET_SPRITE(planet_sprite, "planet.bin", 96, 96);
    PULSE2D_SET_SPRITE(spell_sprite, "spell.bin", 64, 36);
    PULSE2D_SET_SPRITE(explode_sprite, "explosion.bin", 96, 96);
}

PULSE2D_ON_GAMELOOP()
{
    PULSE2D_TICK_WORLD();
    PULSE2D_ON_COLLISION()
    {
        if (!exploded)
            exploded = true;
    }

    PULSE2D_PRINT_STACKSIZE();

    if (exploded)
        PULSE2D_DRAW(explode_sprite);
    else
        PULSE2D_DRAW(planet_sprite);

    PULSE2D_DRAW(spell_sprite);
    PULSE2D_TICK_PULSE();
}