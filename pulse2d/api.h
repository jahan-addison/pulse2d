/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

/****************************************************************************
 * Core API - Runtime
 *
 * Runtime<Scenes...> is the main interface for game logic. It owns the
 * engine, physics world, and the active scene as a std::variant, and
 * exposes every game action - draw, spawn, collide, animate, control - as
 * methods. Instantiate it once at file scope with PULSE_INIT_GAME:
 *
 *   PULSE_INIT_GAME(my_game, Main_Menu, Level_One, Boss_Fight);
 *
 * All methods dispatch into the current scene via execute_scene(), which
 * visits the variant and skips std::monostate (no active scene). This
 * means every call is safe to make before the first PULSE_SET_SCENE.
 *
 * The four gamepad control profiles map directly to Runtime methods:
 *
 *   set_arcade_directional_control("ship", 5.0f)
 *       - instant velocity from stick (Zelda, Pac-Man)
 *
 *   set_arcade_directional_inverted_control("ship", 5.0f)
 *       - same with Y axis flipped
 *
 *   set_dynamic_directional_control("ship", 12.0f)
 *       - thrust force each frame, velocity builds over time (Asteroids)
 *
 *   set_sliding_friction_directional_control("ship", 0.85f)
 *       - linear drag each frame, pair with dynamic control
 *
 * VFX animations are registered once in PULSE_ON_GAMESCENE_START and
 * spawned as one-shot instances during gameplay:
 *
 *   PULSE_ON_GAMESCENE_START(Level_One) {
 *       my_game.register_vfx("explosion", explosion_frames, 64, 64, 8, 12.0f);
 *   }
 *
 *   PULSE_ON_GAMESCENE(Level_One) {
 *       my_game.on_collision(laser, &enemy, [&] {
 *           auto coords = get_body_coordinates(laser);
 *           my_game.play_vfx("explosion", coords.x, coords.y);
 *           my_game.despawn("lasers", laser);
 *       });
 *       my_game.tick_vfx();
 *       my_game.render();
 *   }
 *
 * Call show_debug_rect() once after init() to enable axis-aligned bounding
 * box outlines for every body - useful when tuning collision shapes. Turn off
 * render_backgrounds() in the scene function so the rects are visible:
 *
 *   PULSE_ON_GAMESTART() {
 *       my_game.init(0.0f, 0.0f, 10);
 *       my_game.show_debug_rect();
 *   }
 *
 ****************************************************************************/

#include <concepts>
#include <etl/array.h>
#include <etl/error_handler.h>
#include <etl/map.h>
#include <etl/vector.h>
#include <pulse2d/gamepad/seesaw.h>
#include <pulse2d/graphics/body.h>
#include <pulse2d/graphics/world.h>
#include <pulse2d/pulse2d.h>
#include <pulse2d/renderer.h>
#include <pulse2d/scene.h>
#include <pulse2d/sprite.h>
#include <pulse2d/storage.h>
#include <pulse2d/util.h>
#include <variant>

/////////////////////////////
// Teensy Development API  //
/////////////////////////////

#if defined(PULSE2D_TEENSY)

static constexpr float PULSE = 1.0f / 60.0f;

namespace pulse2d::runtime {

template<typename... Scenes>
requires(std::derived_from<Scenes, detail::scene_base> && ...)
struct Runtime
{

    pulse2d::Static_Inplace_T<pulse2d::Pulse2d> engine;
    pulse2d::Static_Inplace_T<pulse2d::graphics::World> world;

    static inline std::variant<std::monostate, Scenes...> current_scene;

    template<typename Func>
    requires(std::invocable<Func, Scenes&> && ...)
    static PULSE2D_INLINE void execute_scene(Func&& func)
    {
        std::visit(
            [&](auto& scene) {
                using T = std::decay_t<decltype(scene)>;
                if constexpr (!std::is_same_v<T, std::monostate>) {
                    std::forward<Func>(func)(scene);
                }
            },
            current_scene);
    }

    /**
     * @brief
     * Initialize the engine and physics world.
     *
     * @param gravity_1 horizontal gravity component (0 for no gravity)
     * @param gravity_2 vertical gravity component (0 for no gravity)
     * @param solver physics solver iteration count
     * @return
     */
    PULSE2D_INLINE void init(float gravity_1, float gravity_2, int solver)
    {
        engine.emplace();
        world.emplace(
            pulse2d::graphics::math::Vec2{ gravity_1, gravity_2 }, solver);
        engine->init();
    }

    /**
     * @brief
     * Enable the audio subsystem. Allocates audio memory and starts the
     * SGTL5000 codec over I2C. Call once from PULSE_ON_GAMESTART after
     * init(), and only when an audio shield is physically attached to the
     * board. Must be called before play_music, play_sfx, play_sfx2, or
     * set_volume.
     * @scope: PULSE_ON_GAMESTART (after init)
     */
    PULSE2D_INLINE void enable_audio()
    {
        engine->audio().init();
        PULSE2D_DEBUG_SERIAL("Pulse2d: audio OK");
    }

    /**
     * @brief
     * Rasterize renderer bodies with an axis-aligned bounding box.
     * Note: Background rendering must be turned off for visibility.
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE void show_debug_rect()
    {
        engine->renderer().show_debug_rects = true;
    }

    /**
     * @brief
     * Flush the renderer's sprite queue to the display.
     * Must be the last call in PULSE_ON_GAMESCENE.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE void render() { engine->tick(*world); }

    /**
     * @brief
     * Step the physics simulation one frame
     *
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE void tick() { world->step(PULSE); }

    /**
     * @brief
     * Advance each layer's scroll offset and blit all background layers to the
     * framebuffer. Call before drawing any sprites - backgrounds are FIFO.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE void render_backgrounds()
    {
        execute_scene([&](auto& scene) {
            auto& _bg_renderer = engine->renderer();
            scene.background_manager.update_and_draw_layers(
                _bg_renderer, PULSE);
        });
    }

    //////////
    // Text //
    //////////

    /**
     * @brief
     * Queue text to draw using the built-in 5x7 bitmap font.
     * Flushed during render() after sprites, so it composites on top.
     * Supports '\n' for line breaks and integer size scaling.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param text string to render
     * @param x screen x position in pixels
     * @param y screen y position in pixels (top of the first glyph row)
     * @param color RGB565 value; use text_color(r, g, b) to pack from 8-bit
     * @param size integer scale multiplier, default 1
     */
    PULSE2D_INLINE void draw_text(std::string_view text,
        int x,
        int y,
        uint16_t color,
        float size = 1.0f)
    {
        engine->renderer().draw_text(text, x, y, color, size);
    }

    /**
     * @brief
     * Draw text using a custom Font_Def from font2bytes or a compatible
     * column-major 1bpp bitmap font tool.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param text string to render
     * @param x screen x position in pixels
     * @param y screen y position in pixels
     * @param color RGB565 color value
     * @param font Font_Def descriptor pointing to the glyph table
     * @param size float scale multiplier, default 1.0
     */
    PULSE2D_INLINE void draw_text(std::string_view text,
        int x,
        int y,
        uint16_t color,
        const pulse2d::Renderer::Font_Def& font,
        float size = 1.0f)
    {
        engine->renderer().draw_text(text, x, y, color, font, size);
    }

    /**
     * @brief
     * Pack red, green, blue 8-bit components into an RGB565 value for
     * draw_text. Compute once at startup and reuse; not per-frame.
     *
     * @param r red   0–255
     * @param g green 0–255
     * @param b blue  0–255
     */
    PULSE2D_INLINE static uint16_t text_color(uint8_t r, uint8_t g, uint8_t b)
    {
        return pulse2d::Renderer::text_color(r, g, b);
    }

    /**
     * @brief Cast a named Color constant to a uint16_t for any color parameter.
     */
    PULSE2D_INLINE static uint16_t color(pulse2d::Renderer::Color c)
    {
        return static_cast<uint16_t>(c);
    }

    /////////////
    // Gamepad //
    /////////////

    /**
     * @brief
     * Profile A: The Arcade Controller (Pokemon, Zelda, Pac-Man).
     * Sets velocity directly from stick position for instant response and
     * instant stops. Enable "vertical only" or "horizontal only" movement
     * with the third and fourth boolean arguments.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param body_name body identifier string
     * @param max_speed maximum velocity in world units per second
     * @param vertical_only restrict movement to the Y axis
     * @param horizontal_only restrict movement to the X axis
     * @return
     */
    PULSE2D_INLINE void set_arcade_directional_control(const char* body_name,
        float max_speed,
        bool vertical_only = false,
        bool horizontal_only = false)
    {
        execute_scene([&](auto& scene) {
            auto& _body = scene.get_body(body_name);
            pulse2d::gamepad::util::apply_arcade_movement(_body,
                gamepad_hal.pad->get_state(),
                max_speed,
                vertical_only,
                horizontal_only);
        });
    }

    /**
     * @brief
     * Profile A (inverted): Arcade controller with Y axis flipped.
     * Identical to set_arcade_directional_control but applies
     * apply_inverted_arcade_movement - stick up moves the body downward.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param body_name body identifier string
     * @param max_speed maximum velocity in world units per second
     * @param vertical_only restrict movement to the Y axis
     * @param horizontal_only restrict movement to the X axis
     * @return
     */
    PULSE2D_INLINE void set_arcade_directional_inverted_control(
        const char* body_name,
        float max_speed,
        bool vertical_only = false,
        bool horizontal_only = false)
    {
        execute_scene([&](auto& scene) {
            auto& _body = scene.get_body(body_name);
            pulse2d::gamepad::util::apply_inverted_arcade_movement(_body,
                gamepad_hal.pad->get_state(),
                max_speed,
                vertical_only,
                horizontal_only);
        });
    }

    /**
     * @brief
     * Profile B: The Momentum Controller (Asteroids, Mario).
     * Applies a thrust force scaled by acceleration each frame - velocity
     * builds up over time. Pair with set_sliding_friction_directional_control
     * to prevent indefinite sliding.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param body_name body identifier string
     * @param acceleration force magnitude applied per frame
     * @return
     */
    PULSE2D_INLINE void set_dynamic_directional_control(const char* body_name,
        float acceleration)
    {
        execute_scene([&](auto& scene) {
            auto& _body = scene.get_body(body_name);
            pulse2d::gamepad::util::apply_dynamic_thrust(
                _body, gamepad_hal.pad->get_state(), acceleration);
        });
    }

    /**
     * @brief
     * Profile C: Top-Down Friction. Applies linear drag to the body each frame.
     * Pair with set_dynamic_directional_control so the body doesn't slide
     * forever.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param body_name body identifier string
     * @param drag_amount drag coefficient applied per frame (0.0 = no drag)
     * @return
     */
    PULSE2D_INLINE void set_sliding_friction_directional_control(
        const char* body_name,
        float drag_amount)
    {
        execute_scene([&](auto& scene) {
            auto& _body = scene.get_body(body_name);
            pulse2d::gamepad::util::apply_linear_drag(_body, drag_amount);
        });
    }

    ///////////////
    // Animation //
    ///////////////

    /**
     * @brief
     * Advance the animator's frame accumulator and mutate the sprite's
     * flash_data pointer to the current frame.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param animator_inst Sprite_Animator instance
     * @param sprite_name named sprite to update
     * @return
     */
    PULSE2D_INLINE void tick_animation(pulse2d::Sprite_Animator& animator_inst,
        const char* sprite_name)
    {
        execute_scene([&](auto& scene) {
            pulse2d::Sprite& _spr = scene.get_sprite(sprite_name);
            animator_inst.tick(_spr, PULSE);
        });
    }

    /**
     * @brief
     * Register a VFX animation definition in the current scene's animation
     * manager.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param anim_name animation identifier
     * @param data_ptr pointer to the flash sprite sheet array
     * @param width frame width in pixels
     * @param height frame height in pixels
     * @param frames total number of frames
     * @param fps playback rate in frames per second
     * @return
     */
    PULSE2D_INLINE Runtime& register_vfx(const char* anim_name,
        uint16_t const* data_ptr,
        uint16_t width,
        uint16_t height,
        uint16_t frames,
        float fps)
    {
        execute_scene([&](auto& scene) {
            scene.animation_manager.anim_defs[anim_name] = {
                data_ptr, width, height, frames, (1.0f / fps)
            };
        });
        return *this;
    }

    /**
     * @brief
     * Spawn a VFX animation instance at the given screen coordinates.
     * Plays once and is removed automatically. Silently dropped if the queue
     * is full.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param anim_name animation identifier registered with tick_animation
     * @param pos_x screen x position in pixels
     * @param pos_y screen y position in pixels
     * @return
     */
    PULSE2D_INLINE void play_vfx(const char* anim_name,
        int16_t pos_x,
        int16_t pos_y)
    {
        execute_scene([&](auto& scene) {
            auto& anim_mgr = scene.animation_manager;
            if (!anim_mgr.active_animations.full()) {
                anim_mgr.active_animations.push_back(
                    { &anim_mgr.anim_defs[anim_name],
                        (int16_t)pos_x,
                        (int16_t)pos_y,
                        0.0f,
                        0 });
            }
        });
    }

    /**
     * @brief
     * Advance and draw all active VFX animations. Call after game objects
     * but before render().
     *
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE void tick_vfx()
    {
        execute_scene([&](auto& scene) {
            scene.animation_manager.tick_and_draw_animations(
                engine->renderer(), PULSE);
        });
    }

    ///////////
    // Audio //
    ///////////

    /**
     * @brief
     * Start playing a looping music track. The track restarts automatically
     * when tick_audio() is called each frame. Data must be in the
     * AudioPlayMemory format produced by wav2sketch.
     *
     * @scope: PULSE_ON_GAMESCENE_START or PULSE_ON_GAMESCENE
     * @param data pointer to the wav2sketch audio array
     * @return
     */
    PULSE2D_INLINE void play_music(const unsigned int* data)
    {
        engine->audio().play_music(data);
    }

    /**
     * @brief
     * Stop the current music track. Clears the loop pointer so tick_audio()
     * will not restart it.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE void stop_music() { engine->audio().stop_music(); }

    /**
     * @brief
     * Play a one-shot SFX clip on channel 1. Interrupts any clip currently
     * playing on this channel. Data must be in the AudioPlayMemory format
     * produced by wav2sketch.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param data pointer to the wav2sketch audio array
     * @return
     */
    PULSE2D_INLINE void play_sfx(const unsigned int* data)
    {
        engine->audio().play_sfx(data);
    }

    /**
     * @brief
     * Play a one-shot SFX clip on channel 2. Runs independently of channel 1
     * so both SFX channels can play simultaneously.
     * Data must be in the AudioPlayMemory format produced by wav2sketch.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param data pointer to the wav2sketch audio array
     * @return
     */
    PULSE2D_INLINE void play_sfx2(const unsigned int* data)
    {
        engine->audio().play_sfx2(data);
    }

    /**
     * @brief
     * Advance the audio engine one frame. Restarts looping music if it has
     * finished playing. Call once per frame in PULSE_ON_GAMESCENE.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE void tick_audio() { engine->audio().tick(); }

    /**
     * @brief
     * Set the master output volume for both music and SFX (0.0–1.0).
     *
     * @scope: PULSE_ON_GAMESCENE_START or PULSE_ON_GAMESCENE
     * @param v volume level
     * @return
     */
    PULSE2D_INLINE void set_volume(float v) { engine->audio().set_volume(v); }

    ///////////////
    // Collision //
    ///////////////

    /**
     * @brief
     * Conditional block that executes when at least one collision is active
     * in the physics world.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @return
     */
    PULSE2D_INLINE bool objects_collided() { return !world->arbiters.empty(); }

    /**
     * @brief
     * Iterate all active arbiters and call action when either body in a pair
     * matches the named body.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param body_name body identifier (looked up by name in current scene)
     * @param action callable invoked on match
     * @return
     */
    template<typename Func>
    requires(std::invocable<Func, graphics::Body*>)
    PULSE2D_INLINE void on_collision_with(const char* body_name, Func&& action)
    {
        execute_scene([&](auto& scene) {
            pulse2d::graphics::Body* target = &scene.get_body(body_name);

            for (auto it = world->arbiters.begin();
                it != world->arbiters.end();) {
                if (it->first.body1 == target or it->first.body2 == target) {
                    if (it->first.body1 == target)
                        std::forward<Func>(action)(it->first.body2);
                    else
                        std::forward<Func>(action)(it->first.body1);
                    it = world->arbiters.erase(it);
                } else {
                    ++it;
                }
            }
        });
    }

    /**
     * @brief
     * Like on_collision_with, but matches by body pointer instead of name.
     * Use inside render_pool lambdas where a pointer is already in scope.
     * Passes the other body pointer to action.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param target_body body pointer to match against each arbiter entry
     * @param action callable (lambda) invoked on match
     * @return
     */
    template<typename Func>
    requires(std::invocable<Func, graphics::Body*>)
    PULSE2D_INLINE void on_collision_with_body(
        pulse2d::graphics::Body* target_body,
        Func&& action)
    {
        for (auto it = world->arbiters.begin(); it != world->arbiters.end();) {
            if (it->first.body1 == target_body) {
                action(it->first.body2);
                it = world->arbiters.erase(it);
            } else if (it->first.body2 == target_body) {
                action(it->first.body1);
                it = world->arbiters.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief
     * Fire action when a single arbiter holds exactly the two given body
     * pointers (in either slot).
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param target_a first body pointer
     * @param target_b second body pointer
     * @param action callable (lambda) invoked on match
     * @return
     */
    template<typename Func>
    requires(std::invocable<Func>)
    PULSE2D_INLINE void on_collision(pulse2d::graphics::Body* target_a,
        pulse2d::graphics::Body* target_b,
        Func&& action)
    {
        for (auto it = world->arbiters.begin(); it != world->arbiters.end();) {
            bool match_ab =
                (it->first.body1 == target_a and it->first.body2 == target_b);
            bool match_ba =
                (it->first.body1 == target_b and it->first.body2 == target_a);

            if (match_ab or match_ba) {
                action();
                it = world->arbiters.erase(it);
            } else {
                ++it;
            }
        }
    }

    ////////////////
    // Background //
    ////////////////

    /**
     * @brief
     * Add a parallax scrolling layer to the current scene. Layers are drawn
     * in the order they are added.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param sprite_name named sprite to use as the layer
     * @param width image width in pixels (used to wrap the scroll offset)
     * @param speed scroll speed in pixels per second
     * @return
     */
    PULSE2D_INLINE Runtime& add_parallax_layer(const char* sprite_name,
        float width,
        float speed)
    {
        execute_scene([&](auto& scene) {
            scene.background_manager.background_layers.push_back(
                { sprite_name, width, speed, 0.0f });
        });
        return *this;
    }

    /**
     * @brief
     * Add a static (non-scrolling) background layer to the current scene.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param sprite_name named sprite to use as the layer
     * @param width image width in pixels
     * @return
     */
    PULSE2D_INLINE Runtime& add_background_layer(const char* sprite_name,
        float width)
    {
        execute_scene([&](auto& scene) {
            scene.background_manager.background_layers.push_back(
                { sprite_name, width, 0.0f, 0.0f });
        });
        return *this;
    }

    //////////////////
    // Object Pools //
    //////////////////

    /**
     * @brief
     * Initialize a named kinematic pool with a body descriptor template.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param pool_name pool identifier string
     * @param desc Body_Descriptor initializer
     * @return
     */
    PULSE2D_INLINE Runtime& init_pool(const char* pool_name,
        pulse2d::graphics::detail::Body_Descriptor desc)
    {
        execute_scene([&](auto& scene) {
            scene.pool_manager.instances[pool_name].set_descriptor(desc);
            scene.pool_manager.instance_timer[pool_name] = 0;
        });
        return *this;
    }

    /**
     * @brief
     * Spawn an object from the pool at the given position and velocity.
     * Rate-limited by delay - ignored if called before delay ms has elapsed.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param pool_name pool identifier string
     * @param delay minimum milliseconds between spawns
     * @param pos_x initial world x position
     * @param pos_y initial world y position
     * @param vel_x initial x velocity
     * @param vel_y initial y velocity
     * @return
     */
    PULSE2D_INLINE void spawn(const char* pool_name,
        uint32_t delay,
        float pos_x,
        float pos_y,
        float vel_x,
        float vel_y)
    {
        execute_scene([&](auto& scene) {
            const auto& _timer =
                scene.pool_manager.instance_timer.at(pool_name);
            if (_timer >= delay) {
                scene.pool_manager.instances.at(pool_name).deploy(
                    &world, pos_x, pos_y, vel_x, vel_y);
                scene.pool_manager.instance_timer[pool_name] = 0;
            }
        });
    }

    /**
     * @brief
     * Release a pooled body and return its memory to the pool.
     * Removes the body from the physics world.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param pool_name pool identifier string
     * @param body_ref pointer to the body to release
     * @return
     */
    PULSE2D_INLINE void despawn(const char* pool_name,
        pulse2d::graphics::Body* body_ref)
    {
        execute_scene([&](auto& scene) {
            scene.pool_manager.instances.at(pool_name).retract(
                &world, body_ref);
        });
    }

    /**
     * @brief
     * Iterate over all active objects in a pool and execute an action for each.
     * Iterates in reverse to allow safe despawning during iteration.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param pool_name pool identifier string
     * @param action lambda receiving a Body* for each active object
     * @return
     */
    template<typename Func>
    requires(std::invocable<Func, graphics::Body*>)
    PULSE2D_INLINE void render_pool(const char* pool_name, Func&& action)
    {
        execute_scene([&](auto& scene) {
            auto& _pool = scene.pool_manager.instances.at(pool_name);
            auto& _objs = _pool.active_objects();
            for (int _i = static_cast<int>(_objs.size()) - 1; _i >= 0; --_i)
                std::forward<Func>(action)(_objs[_i]);
        });
    }

    ////////////////////////
    // Sprites, Rendering //
    ////////////////////////

    /**
     * @brief
     * Register a compiled sprite array as a named sprite in the embedded pool.
     * Use for assets compiled into the binary.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param name sprite identifier string
     * @param data_ptr pointer to the flash RGB565 pixel array
     * @param w sprite width in pixels
     * @param h sprite height in pixels
     * @return
     */
    PULSE2D_INLINE Runtime& set_sprite_embedded(const char* name,
        uint16_t const* data_ptr,
        uint16_t w,
        uint16_t h)
    {
        execute_scene(
            [&](auto& scene) { scene.set_embedded(name, data_ptr, w, h); });
        return *this;
    }

    /**
     * @brief
     * Alias for set_sprite_embedded, conventional for parallax background
     * layers.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param name sprite identifier string
     * @param data_ptr pointer to the flash RGB565 pixel array
     * @param w sprite width in pixels
     * @param h sprite height in pixels
     * @return
     */
    PULSE2D_INLINE Runtime& set_background_sprite(const char* name,
        uint16_t const* data_ptr,
        uint16_t w,
        uint16_t h)
    {
        return set_sprite_embedded(name, data_ptr, w, h);
    }

    /**
     * @brief
     * Project a named body to screen coordinates and queue its sprite for
     * rendering.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param body_name named body identifier string
     * @param sprite_name named sprite identifier string
     * @return
     */
    PULSE2D_INLINE void draw(const char* body_name, const char* sprite_name)
    {
        execute_scene([&](auto& scene) {
            const auto& _body = scene.get_body(body_name);
            auto [sx, sy] = pulse2d::Renderer::units_to_pixels(
                _body.position.x, _body.position.y);
            const pulse2d::Sprite& _spr = scene.get_sprite(sprite_name);
            engine->renderer().add_sprite(&_spr,
                static_cast<int16_t>(sx - _spr.width / 2),
                static_cast<int16_t>(sy - _spr.height / 2));
        });
    }

    PULSE2D_INLINE void draw(const char* body_name,
        const char* sprite_name,
        float rotation)
    {
        execute_scene([&](auto& scene) {
            const auto& _body = scene.get_body(body_name);
            auto [sx, sy] = pulse2d::Renderer::units_to_pixels(
                _body.position.x, _body.position.y);
            const pulse2d::Sprite& _spr = scene.get_sprite(sprite_name);
            engine->renderer().add_sprite(&_spr,
                static_cast<int16_t>(sx - _spr.width / 2),
                static_cast<int16_t>(sy - _spr.height / 2),
                rotation);
        });
    }

    /**
     * @brief
     * Directly write a sprite to screen at pixel coordinates, bypassing body
     * projection. Use for HUD elements and fixed-position overlays.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param sprite_name named sprite identifier string
     * @param x screen x position in pixels
     * @param y screen y position in pixels
     * @return
     */
    PULSE2D_INLINE void draw_sprite(const char* sprite_name,
        int16_t x,
        int16_t y)
    {
        execute_scene([&](auto& scene) {
            const pulse2d::Sprite& _spr = scene.get_sprite(sprite_name);
            engine->renderer().add_sprite(&_spr, x, y);
        });
    }

    /**
     * @brief
     * Like draw, but takes a body pointer instead of a name.
     * Use inside render_pool lambdas.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param body_ptr pointer to the body
     * @param sprite_name named sprite identifier string
     * @return
     */
    PULSE2D_INLINE void draw_body(pulse2d::graphics::Body const* body_ptr,
        const char* sprite_name)
    {
        execute_scene([&](auto& scene) {
            auto [sx, sy] = pulse2d::Renderer::units_to_pixels(
                body_ptr->position.x, body_ptr->position.y);
            const pulse2d::Sprite& _spr = scene.get_sprite(sprite_name);
            engine->renderer().add_sprite(&_spr,
                static_cast<int16_t>(sx - _spr.width / 2),
                static_cast<int16_t>(sy - _spr.height / 2));
        });
    }

    PULSE2D_INLINE void draw_body(pulse2d::graphics::Body const* body_ptr,
        const char* sprite_name,
        float rotation)
    {
        execute_scene([&](auto& scene) {
            auto [sx, sy] = pulse2d::Renderer::units_to_pixels(
                body_ptr->position.x, body_ptr->position.y);
            const pulse2d::Sprite& _spr = scene.get_sprite(sprite_name);
            engine->renderer().add_sprite(&_spr,
                static_cast<int16_t>(sx - _spr.width / 2),
                static_cast<int16_t>(sy - _spr.height / 2),
                rotation);
        });
    }

    /**
     * @brief
     * Allocate an immovable body in the current scene's pool and register it
     * with the physics world.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param name body identifier string
     * @param desc Body_Descriptor initializer
     * @return
     */
    PULSE2D_INLINE Runtime& set_static_body(const char* name,
        pulse2d::graphics::detail::Body_Descriptor desc)
    {
        execute_scene([&](auto& scene) {
            scene.set(name, desc);
            world->add(&scene.get_body(name));
        });
        return *this;
    }

    /**
     * @brief
     * Allocate a body for player or externally driven objects.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param name body identifier string
     * @param desc Body_Descriptor initializer
     * @return
     */
    PULSE2D_INLINE Runtime& set_controlled_body(const char* name,
        pulse2d::graphics::detail::Body_Descriptor desc)
    {
        execute_scene([&](auto& scene) {
            if (desc.mass == 0.0f)
                desc.mass = 1.0f;
            scene.set(name, desc);
            scene.get_body(name).set_motion();
            world->add(&scene.get_body(name));
        });
        return *this;
    }

    /**
     * @brief
     * Allocate a fully simulated dynamic body and register it with the world.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param name body identifier string
     * @param desc Body_Descriptor initializer
     * @return
     */
    PULSE2D_INLINE Runtime& set_dynamic_body(const char* name,
        pulse2d::graphics::detail::Body_Descriptor desc)
    {
        execute_scene([&](auto& scene) {
            scene.set(name, desc);
            scene.get_body(name).set_motion();
            world->add(&scene.get_body(name));
        });
        return *this;
    }

    /**
     * @brief
     * Return a reference to a named body from the current scene.
     *
     * @scope: PULSE_ON_GAMESCENE
     * @param name body identifier string
     * @return graphics::Body&
     */
    PULSE2D_INLINE pulse2d::graphics::Body& get_body(const char* name)
    {
        pulse2d::graphics::Body* result = nullptr;
        execute_scene([&](auto& scene) { result = &scene.get_body(name); });
        return *result;
    }

    /**
     * @brief
     * Load a `.bin` sprite from the SD card. Width and height are read
     * from the file header.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param name sprite identifier string
     * @param path sprite absolute path on sd card
     * @return
     */
    PULSE2D_INLINE Runtime& set_sprite(const char* name, const char* path)
    {
        execute_scene([&](auto& scene) {
            scene.set(name, engine->storage(), path, 0, 0);
        });
        return *this;
    }

    /**
     * @brief
     * Load a sprite from the SD card with explicit dimensions. On Teensy,
     * validates that the `.bin` header matches; on host, scales the image
     * to the given dimensions via nearest-neighbour.
     *
     * @scope: PULSE_ON_GAMESCENE_START
     * @param name sprite identifier string
     * @param path sprite absolute path on sd card
     * @param w expected width
     * @param h expected height
     * @return
     */
    PULSE2D_INLINE Runtime& set_sprite(const char* name,
        const char* path,
        uint16_t w,
        uint16_t h)
    {
        execute_scene([&](auto& scene) {
            scene.set(name, engine->storage(), path, w, h);
        });
        return *this;
    }
};

} // namespace runtime

#endif