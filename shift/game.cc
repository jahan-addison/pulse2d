/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 *
 * This file is part of pulse2d.
 * This software is released under the MIT License. You may use,
 * distribute, and modify this code under the terms of the license.
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

/****************************************************************************
 * Sample Game - Hit the planet with a fireball
 *
 * Use your Keyboard to cast a fireball spell at the planet body.
 ****************************************************************************/

#include <SDL2/SDL.h>               // for SDL_PollEvent, SDL_QUIT, SDL_Event
#include <cstdio>                   // for fprintf
#include <cstdlib>                  // for abort
#include <cxxabi.h>                 // for abi::__cxa_demangle
#include <etl/error_handler.h>      // for error_handler::set_callback
#include <etl/exception.h>          // for exception
#include <etl/map.h>                // for map
#include <execinfo.h>               // for backtrace, backtrace_symbols
#include <pulse2d/graphics/body.h>  // for Body
#include <pulse2d/graphics/math.h>  // for Vec2
#include <pulse2d/graphics/world.h> // for World
#include <pulse2d/pulse2d.h>        // for Pulse2d
#include <string>                   // for string
#include <string_view>              // for string_view

// ETL error callback
static void on_etl_error(const etl::exception& e)
{
    std::fprintf(stderr,
        "ETL error: %s (%s:%d)\n",
        e.what(),
        e.file_name(),
        e.line_number());
    std::abort();
}

enum class Action
{
    Up,
    Down,
    Shoot, // "A" key
    Reset, // "Z" key
    None
};

struct State
{
    constexpr inline void win_game()
    {
        fired = false;
        exploded = true;
    }
    constexpr inline void reset()
    {
        fired = false;
        exploded = false;
    }

  public:
    bool fired{ false };
    bool exploded{ false };
};

struct Object_Pool
{
    inline void add(std::string name, pulse2d::graphics::Body body)
    {
        body_.insert({ std::move(name), std::move(body) });
    }
    inline pulse2d::graphics::Body& get(std::string_view name)
    {
        return body_.at(name.data());
    }

  private:
    etl::map<std::string, pulse2d::graphics::Body, 16> body_{};
};

struct Sprite_Pool
{
    inline void add(pulse2d::Pulse2d& engine,
        std::string name,
        std::string_view s_path,
        uint16_t width,
        uint16_t height)
    {
        pulse2d::Sprite sprite =
            engine.storage().load_sprite(s_path.data(), width, height);

        sprites_.insert({ std::move(name), std::move(sprite) });
    }
    inline pulse2d::Sprite& get(std::string_view name)
    {
        return sprites_.at(name.data());
    }

  private:
    etl::map<std::string, pulse2d::Sprite, 3> sprites_{};
};

///////////////////////////////////////////////////////////////////////////////////////
// SDL2 game movement:
// https://www.libsdl.org/release/SDL-1.2.15/docs/html/guideinputkeyboard.html#AEN363
///////////////////////////////////////////////////////////////////////////////////////

void on_keydown(Action* active_direction, SDL_Event const& event)
{
    if (event.type == SDL_KEYDOWN)
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                *active_direction = Action::Up;
                break;
            case SDLK_DOWN:
                *active_direction = Action::Down;
                break;
            case SDLK_a:
                *active_direction = Action::Shoot;
                break;
            case SDLK_z:
                *active_direction = Action::Reset;
                break;
        }
}

void on_keyup(Action* active_direction, SDL_Event const& event)
{
    if (event.type == SDL_KEYUP)
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                if (*active_direction == Action::Up)
                    *active_direction = Action::None;
                break;
            case SDLK_DOWN:
                if (*active_direction == Action::Down)
                    *active_direction = Action::None;
                break;
            case SDLK_a:
                if (*active_direction == Action::Shoot)
                    *active_direction = Action::None;
                break;
            case SDLK_z:
                if (*active_direction == Action::Reset)
                    *active_direction = Action::None;
                break;
            default:
                break;
        }
}

void on_throw_fireball(Object_Pool& objects,
    State& state,
    float y_pos,
    Action* action)
{
    if (state.exploded)
        return;
    auto& fireball = objects.get("fireball");
    if (state.fired)
        objects.get("fireball").position.y = y_pos;
    if (*action != Action::Shoot)
        return;
    fireball.set_mass({ 1.0f, 0.5f }, 1.0f);
    fireball.position = { -4.333f, y_pos };
    fireball.velocity = { 1.611f, 0.0f };
    state.fired = true;
}

void set_planet_target(pulse2d::Pulse2d& engine,
    Object_Pool& objects,
    Sprite_Pool& sprites,
    bool exploded)
{
    auto& renderer = engine.renderer();
    auto& planet = objects.get("planet");
    auto [sx, sy] = pulse2d::Renderer::project_coordinates(
        planet.position.x, planet.position.y);
    // if exploded (the winning state) is set, use the exploded sprite
    auto& sprite = exploded ? sprites.get("explode") : sprites.get("planet");
    renderer.add_sprite(&sprite,
        static_cast<int16_t>(sx - sprite.width / 2),
        static_cast<int16_t>(sy - sprite.height / 2));
}

inline void reset_fire_ball(Object_Pool& objects,
    float* y_pos,
    bool position = true)
{
    auto& spell = objects.get("fireball");
    // reset position, too?
    if (position) {
        *y_pos = 2.833f;
        spell.position = { -4.333f, *y_pos };
    }
    spell.velocity = { 0.0f, 0.0f };
    spell.inv_mass = 0.0f;
    spell.I = FLT_MAX;
    spell.inv_i = 0.0f;
}

void set_fire_ball(pulse2d::Pulse2d& engine,
    Object_Pool& objects,
    Sprite_Pool& sprites,
    float* y_pos)
{
    auto& renderer = engine.renderer();
    auto& spell = objects.get("fireball");
    auto& sprite = sprites.get("fireball");

    // if the fireball goes off the screen, reset position
    if (spell.position.x > 5.0f)
        reset_fire_ball(objects, y_pos);

    auto [sx, sy] =
        pulse2d::Renderer::project_coordinates(spell.position.x, *y_pos);
    renderer.add_sprite(&sprite,
        static_cast<int16_t>(sx - sprite.width / 2),
        static_cast<int16_t>(sy - sprite.height / 2),
        3.111f);
}

inline void on_reset(Object_Pool& objects,
    State& state,
    float* y_pos,
    Action action)
{
    if (action == Action::Reset and state.exploded) {
        state.reset();
        reset_fire_ball(objects, y_pos);
    }
}

int main()
{
    etl::error_handler::set_callback<&on_etl_error>();

    pulse2d::Pulse2d engine{};
    State state{};
    Sprite_Pool sprites{};
    Object_Pool objects{};

    engine.init();

    pulse2d::graphics::World world({ 0.0f, 0.0f }, 10);

    float spell_ypos = 2.833f;

    objects.add("planet",
        pulse2d::graphics::Body{
            .position = { 3.5f, 0.0f },
              .width = { 1.0f, 1.0f }
    });

    objects.add("fireball",
        pulse2d::graphics::Body{
            .position = { -4.333f, spell_ypos }
    });

    world.add(&objects.get("planet"));
    world.add(&objects.get("fireball"));

    sprites.add(
        engine, "planet", "shift/images/Water_Ball_Frame_07.png", 96, 96);
    sprites.add(
        engine, "fireball", "shift/images/Fire_Spell_Frame_07.png", 64, 36);
    sprites.add(
        engine, "explode", "shift/images/Fire_Explosion_Frame_07.png", 96, 96);

    // engine.renderer().show_debug_rects = true;

    constexpr float k_dt = 1.0f / 60.0f;
    SDL_Event event;
    bool running = true;
    Action action = Action::None;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            on_keydown(&action, event);
            on_keyup(&action, event);

            on_throw_fireball(objects, state, spell_ypos, &action);
            on_reset(objects, state, &spell_ypos, action);
        }

        world.step(k_dt);

        // a collision!
        if (!state.exploded and not world.arbiters.empty()) {
            state.win_game();
            reset_fire_ball(objects, &spell_ypos, false);
            world.arbiters.clear();
        }

        if (action == Action::Up and not state.exploded) {
            spell_ypos += 0.018f;
        }
        if (action == Action::Down and not state.exploded)
            spell_ypos -= 0.018f;

        set_planet_target(engine, objects, sprites, state.exploded);
        set_fire_ball(engine, objects, sprites, &spell_ypos);

        engine.tick(world);
    }

    return 0;
}
