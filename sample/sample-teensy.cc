/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include PULSE2D_HEADER
#include PULSE2D_GRAPHICS

PULSE2D_START_PULSE();

PULSE_DEFINE_SCENE(Sample_Level, 2, 3);

PULSE_INIT_GAME(sample_game, Sample_Level);

///////////
// State //
///////////

PULSE_DEFINE bool exploded = false;
PULSE_DEFINE bool fired = false;

/////////////////
// Scene Setup //
/////////////////

PULSE_ON_GAMESCENE_START(Sample_Level)
{
    sample_game.set_sprite("planet_sprite", "planet.bin");
    sample_game.set_sprite("spell_sprite", "spell.bin");
    sample_game.set_sprite("explode_sprite", "explosion.bin");

    sample_game.set_static_body("planet_object",
        {
            .position = { 3.5f, 0.0f },
              .width = { 1.0f, 1.0f }
    });

    sample_game.set_dynamic_body("spell_object",
        {
            .position = { -3.5f, 2.5111f },
            .velocity = { 0.0f,  0.0f    },
            .width = { 1.0f,  0.5f    },
            .mass = 1.0f
    });
}

///////////
// Scene //
///////////

PULSE_ON_GAMESCENE(Sample_Level)
{
    sample_game.tick();

    PULSE_POLL_SEESAW_GAMEPAD();

    pulse2d_body& spell = sample_game.get_body("spell_object");

    sample_game.on_collision_with("planet_object", [&] {
        if (!exploded) {
            exploded = true;
            spell.set_velocity({ 0.0f, 0.0f });
        }
    });

    if (!fired)
        sample_game.set_arcade_directional_inverted_control(
            "spell_object", 5.22f, true);

    if (!fired and SEESAW_BUTTON_INPUT(SEESAW_A)) {
        spell.set_velocity({ 12.555f, 0.0f });
        fired = true;
    }

    if (spell.position.x > 5.5f or SEESAW_BUTTON_INPUT(SEESAW_START)) {
        fired = false;
        exploded = false;
        PULSE_SET_SCENE(sample_game, Sample_Level);
    }

    if (exploded)
        sample_game.draw("planet_object", "explode_sprite");
    else
        sample_game.draw("planet_object", "planet_sprite");

    sample_game.draw("spell_object", "spell_sprite", 3.111f);
    sample_game.render();
}

////////////////
// Game Setup //
////////////////

PULSE_ON_GAMESTART()
{
    Serial.begin(115200);

    sample_game.init(0.0f, 0.0f, 10);
    start_seesaw_gamepad();
    PULSE_SET_SCENE(sample_game, Sample_Level);
}

PULSE_ON_GAMELOOP()
{
    PULSE_TICK_GAMESCENE();
}