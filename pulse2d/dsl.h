/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <etl/array.h>
#include <etl/error_handler.h>
#include <etl/map.h>
#include <etl/vector.h>
#include <pulse2d/gamepad/seesaw.h>
#include <pulse2d/graphics/body.h>
#include <pulse2d/graphics/world.h>
#include <pulse2d/renderer.h>
#include <pulse2d/scene.h>
#include <pulse2d/sprite.h>
#include <pulse2d/storage.h>
#include <pulse2d/util.h>
#include <variant>

/////////////////////////////
// Teensy Development DSL  //
/////////////////////////////

////////////////
// Game State //
////////////////

#if defined(PULSE2D_TEENSY)

/**
 * @brief
 * Define a BSS memory section
 *
 * @scope: global
 * @return
 *
 */
#define PULSE2D_DEFINE static

/**
 * @brief
 * Define a type in HARDWARE_Deferred_Init for deferred construction,
 * Types constructed in this way should require the arduino runtime.
 *
 * @scope: global
 * @param type the class type
 * @return
 *
 */
#define PULSE2D_HARDWARE_DEFINE(type) \
    PULSE2D_DEFINE pulse2d::HARDWARE_Deferred_Init<type>

/**
 * @brief
 * Maps to Arduino setup(). Runs once on power-on.
 *
 * @scope: global
 * @return
 */
#define PULSE2D_ON_GAMESTART() void setup()

/**
 * @brief
 * Maps to Arduino loop(). Runs every frame (~60 Hz).
 *
 * @scope: global
 * @return
 */
#define PULSE2D_ON_GAMELOOP() void loop()

/**
 * @brief
 * Declare the engine, physics world, and scene dispatch pointers.
 * Creates: engine, world, pending_transition, active_scene_fn, PULSE.
 *
 * @scope: global
 * @return
 */
#define PULSE2D_START_PULSE()                                \
    PULSE2D_HARDWARE_DEFINE(pulse2d::Pulse2d) engine;        \
    PULSE2D_HARDWARE_DEFINE(pulse2d::graphics::World) world; \
    void (*pending_transition)() = nullptr;                  \
    void (*active_scene_fn)() = nullptr;                     \
    PULSE2D_DEFINE constexpr float PULSE = 1.0f / 60.0f;

/**
 * @brief
 * Initialize the engine and physics world.
 *
 * @scope: PULSE2D_ON_GAMESTART
 * @param gravity_1 horizontal gravity component (0 for no gravity)
 * @param gravity_2 vertical gravity component (0 for no gravity)
 * @param solver physics solver iteration count
 * @return
 */
#define PULSE2D_INIT(gravity_1, gravity_2, solver)                            \
    do {                                                                      \
        static_assert(std::is_same_v<decltype(engine),                        \
                          pulse2d::HARDWARE_Deferred_Init<pulse2d::Pulse2d>>, \
            "Pulse2d Engine not defined, did you call "                       \
            "PULSE2D_START_PULSE()?");                                        \
        static_assert(                                                        \
            std::is_same_v<decltype(world),                                   \
                pulse2d::HARDWARE_Deferred_Init<pulse2d::graphics::World>>,   \
            "Pulse2d World not defined, did you call "                        \
            "PULSE2D_START_PULSE()?");                                        \
        engine.emplace();                                                     \
        world.emplace(                                                        \
            pulse2d::graphics::Vec2{ gravity_1, gravity_2 }, solver);         \
        engine->init();                                                       \
    } while (0)

/////////////
// Gamepad //
/////////////

/**
 * @brief
 * Declare the I2C driver and Seesaw gamepad at file scope.
 *
 * @scope: global
 * @return
 */
#define PULSE2D_ENABLE_SEESAW_GAMEPAD()               \
    static pulse2d::gamepad::Teensy_I2CDriver driver; \
    static pulse2d::gamepad::Seesaw_Gamepad pad(driver)

/**
 * @brief
 * Initialize the Seesaw gamepad hardware over I2C.
 *
 * @scope: PULSE2D_ON_GAMESTART
 * @return
 */
#define PULSE2D_START_SEESAW_GAMEPAD()                         \
    do {                                                       \
        if (!pad.init()) {                                     \
            Serial.println("[ERROR]: Seesaw Gamepad Failed!"); \
        }                                                      \
    } while (0)

/**
 * @brief
 * Poll all inputs for the current frame, and brings gamepad_state into scope.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define PULSE2D_POLL_SEESAW_GAMEPAD() \
    pad.poll();                       \
    [[maybe_unused]] auto& gamepad_state = pad.get_state()

// Button bitmask constants mapped to the physical PCB layout
#define SEESAW_A pulse2d::gamepad::Seesaw_Buttons::A
#define SEESAW_B pulse2d::gamepad::Seesaw_Buttons::B
#define SEESAW_X pulse2d::gamepad::Seesaw_Buttons::X
#define SEESAW_Y pulse2d::gamepad::Seesaw_Buttons::Y
#define SEESAW_START pulse2d::gamepad::Seesaw_Buttons::START
#define SEESAW_SELECT pulse2d::gamepad::Seesaw_Buttons::SELECT

/**
 * @brief
 * Raw analog stick X axis, normalized -1.0f to +1.0f.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTIONAL_X_INPUT() gamepad_state.stick_x

/**
 * @brief
 * Raw analog stick Y axis, normalized -1.0f to +1.0f.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTIONAL_Y_INPUT() gamepad_state.stick_y

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude leftward.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_LEFT() (gamepad_state.stick_x < -0.5f)

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude rightward.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_RIGHT() (gamepad_state.stick_x > 0.5f)

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude upward.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_UP() (gamepad_state.stick_y < -0.5f)

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude downward.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_DOWN() (gamepad_state.stick_y > 0.5f)

////////////////////////////////////////////////////////////////////////////////
// Profile A: The Arcade Controller (Pokemon, Zelda, Pac-Man)
// @scope: PULSE2D_ON_GAMESCENE
// Sets velocity directly from stick position for instant response and instant
// stops.
// Note: Enable "vertical only" or "horizontal only" movement by the third and
// forth boolean arguments
////////////////////////////////////////////////////////////////////////////////
#define SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL(body_name, max_speed, ...)      \
    do {                                                                      \
        auto& _body = active_scene.get_body(#body_name);                      \
        pulse2d::gamepad::util::apply_arcade_movement(                        \
            (_body), pad.get_state(), (max_speed)__VA_OPT__(, ) __VA_ARGS__); \
    } while (0)

#define SEESAW_SET_ARCADE_DIRECTIONAL_CONTROL_INVERTED(                       \
    body_name, max_speed, ...)                                                \
    do {                                                                      \
        auto& _body = active_scene.get_body(#body_name);                      \
        pulse2d::gamepad::util::apply_inverted_arcade_movement(               \
            (_body), pad.get_state(), (max_speed)__VA_OPT__(, ) __VA_ARGS__); \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// Profile B: The Momentum Controller (Asteroids, Mario).
// @scope: PULSE2D_ON_GAMESCENE
// Applies a thrust force scaled by acceleration each frame - velocity builds up
// over time.
////////////////////////////////////////////////////////////////////////////////
#define SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL(body_name, acceleration) \
    do {                                                               \
        auto& _body = active_scene.get_body(#body_name);               \
        pulse2d::gamepad::util::apply_dynamic_thrust(                  \
            (_body), pad.get_state(), (acceleration));                 \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// Profile C: Top-Down Friction. Applies linear drag to the body each frame.
// @scope: PULSE2D_ON_GAMESCENE
// Pair with SEESAW_SETDYNAMIC_DIRECTIONAL_CONTROL so the body doesn't slide
// forever.
////////////////////////////////////////////////////////////////////////////////
#define SEESAW_SET_SLIDING_FRICTION_DIRECTIONAL_CONTROL(                   \
    body_name, drag_amount)                                                \
    do {                                                                   \
        auto& _body = active_scene.get_body(#body_name);                   \
        pulse2d::gamepad::util::apply_linear_drag((_body), (drag_amount)); \
    } while (0)

/**
 * @brief
 * True while the named button is held.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param NAME button constant (SEESAW_A, SEESAW_B, ...)
 * @return
 */
// clang-format off
#define SEESAW_BUTTON_INPUT(NAME) gamepad_state.buttons & NAME
// clang-format on

////////////
// Scenes //
////////////

/**
 * @brief
 * Declare a scene struct with fixed-size body, sprite, and joint pools.
 * Sizes are checked at compile time against hardware limits.
 *
 * @scope: global
 * @param name scene identifier
 * @param bodies maximum physics bodies
 * @param sprites maximum loaded sprites
 * @param ... optional joint pool size (default: 0)
 * @return
 */
#define PULSE2D_DEFINE_SCENE(name, bodies, sprites, ...)                     \
    struct name                                                              \
        : pulse2d::Pulse2d_Scene<bodies, sprites __VA_OPT__(, ) __VA_ARGS__> \
    {}

/**
 * @brief
 * Clear the physics world and storage, then enter and register the given scene.
 *
 * @scope: PULSE2D_ON_GAMESTART, PULSE2D_ON_GAMESCENE
 * @param scene scene type to transition into
 * @return
 */
#define PULSE2D_SET_SCENE(scene)                    \
    do {                                            \
        world->clear();                             \
        engine->storage().reset();                  \
        current_scene.emplace<scene>();             \
        pulse2d_scene_enter_##scene();              \
        active_scene_fn = pulse2d_scene_fn_##scene; \
    } while (0)

/**
 * @brief
 * Defer transition to another scene until the end of the current scene tick.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param to scene type to transition into
 * @return
 */
#define PULSE2D_DEFER_SCENE(to) \
    pending_transition = []() { PULSE2D_SET_SCENE(to); }

/**
 * @brief
 * Declare a std::variant holding all scene types used in the game.
 * Creates the current_scene global variable.
 *
 * @scope: global
 * @param ... scene types
 * @return
 */
#define PULSE2D_GAME_SCENES(...)                                           \
    PULSE2D_DEFINE std::variant<std::monostate __VA_OPT__(, ) __VA_ARGS__> \
        current_scene

/**
 * @brief
 * Step the physics simulation one frame. Brings active_scene and renderer
 * into scope — must be the first call in PULSE2D_ON_GAMESCENE.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param scene scene type of the current scene
 * @return
 */
#define PULSE2D_TICK_WORLD(scene)                                         \
    [[maybe_unused]] auto& active_scene = std::get<scene>(current_scene); \
    world->step(PULSE);                                                   \
    [[maybe_unused]] auto& renderer = engine->renderer()

/**
 * @brief
 * Flush the renderer's sprite queue to the display.
 * Must be the last call in PULSE2D_ON_GAMESCENE.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define PULSE2D_RENDER(active_scene) engine->tick(*world)

/**
 * @brief
 * Define the entry function for a scene, called once by PULSE2D_SET_SCENE.
 *
 * @scope: global
 * @param scene scene identifier
 * @return
 */
#define PULSE2D_ON_GAMESCENE_START(scene) void pulse2d_scene_enter_##scene()

/**
 * @brief
 * Define the per-frame function for a scene.
 *
 * @scope: global
 * @param scene scene identifier
 * @return
 */
#define PULSE2D_ON_GAMESCENE(scene) void pulse2d_scene_fn_##scene()

/**
 * @brief
 * Call the active scene's per-frame function, then resolve any pending
 * scene transition. The only call needed in PULSE2D_ON_GAMELOOP.
 *
 * @scope: PULSE2D_ON_GAMELOOP
 * @return
 */
#define PULSE2D_TICK_GAMESCENE()             \
    do {                                     \
        if (active_scene_fn != nullptr)      \
            active_scene_fn();               \
        if (pending_transition != nullptr) { \
            pending_transition();            \
            pending_transition = nullptr;    \
        }                                    \
    } while (0)

/**
 * @brief
 * Manually tick the game engine. Prefer PULSE2D_RENDER which calls this
 * automatically.
 *
 * @scope: PULSE2D_ON_GAMELOOP
 * @return
 */
#define PULSE2D_TICK_PULSE() engine->tick(*world)

///////////////
// Animation //
///////////////

////////////////
// Persistent //
///////////////

/**
 * @brief
 * Declare a named Sprite_Animator at file scope.
 * The instance persists across frames and scenes.
 *
 * @scope: global
 * @param name animator identifier
 * @return
 */
#define PULSE2D_DEFINE_ANIMATOR(name) \
    pulse2d::Sprite_Animator name {}

/**
 * @brief
 * Define an immutable animation sequence blueprint as a static constexpr.
 * time_per_frame (1.0f / fps) is computed at compile time.
 *
 * @scope: global
 * @param name animation identifier
 * @param sheet_ptr pointer to the flash sprite sheet array
 * @param width frame width in pixels
 * @param height frame height in pixels
 * @param frames total number of frames
 * @param fps playback rate in frames per second
 * @return
 */
#define PULSE2D_ANIMATION_DEFINITION(                            \
    name, sheet_ptr, width, height, frames, fps)                 \
    static constexpr pulse2d::assets::Animation_Def name = {     \
        (sheet_ptr), (width), (height), (frames), (1.0f / (fps)) \
    }

/**
 * @brief
 * Load an animation definition into a running animator.
 * Resets the accumulator and plays from frame 0.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param animator_inst Sprite_Animator instance
 * @param anim_def Animation_Def to play
 * @return
 */
#define PULSE2D_SET_ANIMATION(animator_inst, anim_def) \
    (animator_inst).set_state(&(anim_def))

/**
 * @brief
 * Advance the animator's frame accumulator and mutate the sprite's
 * flash_data pointer to the current frame.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param animator_inst Sprite_Animator instance
 * @param sprite_name named sprite to update
 * @return
 */
#define PULSE2D_TICK_ANIMATION(animator_inst, sprite_name)             \
    do {                                                               \
        pulse2d::Sprite& _spr = active_scene.get_sprite(#sprite_name); \
        (animator_inst).tick((_spr), PULSE);                           \
    } while (0)

///////////////////
// VFX, One-shot //
///////////////////

/**
 * @brief
 * Register a VFX animation definition in the current scene's animation manager.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param anim_name animation identifier
 * @param data_ptr pointer to the flash sprite sheet array
 * @param w frame width in pixels
 * @param h frame height in pixels
 * @param frames total number of frames
 * @param fps playback rate in frames per second
 * @return
 */
#define PULSE2D_DEFINE_VFX(anim_name, data_ptr, w, h, frames, fps)           \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.animation_manager.anim_defs[#anim_name] = {        \
                        data_ptr, w, h, frames, (1.0f / (float)fps)          \
                    };                                                       \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Spawn a VFX animation instance at the given screen coordinates.
 * Plays once and is removed automatically. Silently dropped if the queue is
 * full.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param anim_name animation identifier registered with PULSE2D_DEFINE_VFX
 * @param pos_x screen x position in pixels
 * @param pos_y screen y position in pixels
 * @return
 */
#define PULSE2D_PLAY_VFX(anim_name, pos_x, pos_y)                            \
    do {                                                                     \
        std::visit(                                                          \
            [&](auto& scene) {                                               \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    auto& anim_mgr = scene.animation_manager;                \
                    if (!anim_mgr.active_animations.full()) {                \
                        anim_mgr.active_animations.push_back(                \
                            { &anim_mgr.anim_defs[#anim_name],               \
                                (float)(pos_x),                              \
                                (float)(pos_y),                              \
                                0.0f,                                        \
                                0 });                                        \
                    }                                                        \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Advance and draw all active VFX animations. Call after game objects
 * but before PULSE2D_RENDER.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define PULSE2D_TICK_VFX()                                       \
    do {                                                         \
        active_scene.animation_manager.tick_and_draw_animations( \
            renderer, PULSE);                                    \
    } while (0)

///////////////
// Collision //
///////////////

/**
 * @brief
 * Conditional block that executes when at least one collision is active
 * in the physics world.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define PULSE2D_ON_COLLISION() if (!world->arbiters.empty())

/**
 * @brief
 * Conditional block that executes when a specific named arbiter is active.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param with arbiter name (string key of the collision pair)
 * @return
 */
#define PULSE2D_ON_COLLISION_WITH(with) if (world->arbiters.contains(#with))

////////////////
// Background //
////////////////

/**
 * @brief
 * Add a parallax scrolling layer to the current scene. Layers are drawn
 * in the order they are added.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param sprite_name named sprite to use as the layer
 * @param width image width in pixels (used to wrap the scroll offset)
 * @param speed scroll speed in pixels per second
 * @return
 */
#define PULSE2D_ADD_PARALLAX_LAYER(sprite_name, width, speed)                \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.background_manager.background_layers.push_back(    \
                        { (#sprite_name), (width), (speed), 0.0f });         \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Add a static (non-scrolling) background layer to the current scene.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param sprite_name named sprite to use as the layer
 * @param width image width in pixels
 * @return
 */
#define PULSE2D_ADD_BACKGROUND_LAYER(sprite_name, width)                     \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.background_manager.background_layers.push_back(    \
                        { (#sprite_name), (width), 0.0f, 0.0f });            \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Advance each layer's scroll offset and blit all background layers to the
 * framebuffer. Call before drawing any sprites — backgrounds are FIFO.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define PULSE2D_RENDER_BACKGROUNDS()                                         \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    auto& _bg_renderer = engine->renderer();                 \
                    scene.background_manager.update_and_draw_layers(         \
                        _bg_renderer, PULSE);                                \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

//////////////////
// Object Pools //
//////////////////

/**
 * @brief
 * Initialize a named kinematic pool with a body descriptor template.
 * The descriptor sets the baseline properties for all spawned objects.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param pool_instance pool identifier
 * @param ... Body_Descriptor initializer
 * @return
 */
#define PULSE2D_INIT_POOL(pool_instance, ...)                                \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.pool_manager.instances[#pool_instance]             \
                        .set_descriptor(__VA_ARGS__);                        \
                    scene.pool_manager.instance_timer[#pool_instance] = 0;   \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Spawn an object from the pool at the given position and velocity.
 * Rate-limited by delay — ignored if called before delay ms has elapsed.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param pool_instance pool identifier
 * @param delay minimum milliseconds between spawns
 * @param pos_x initial world x position
 * @param pos_y initial world y position
 * @param vel_x initial x velocity
 * @param vel_y initial y velocity
 * @return
 */
#define PULSE2D_SPAWN(pool_instance, delay, pos_x, pos_y, vel_x, vel_y)   \
    do {                                                                  \
        auto& _timer =                                                    \
            active_scene.pool_manager.instance_timer.at(#pool_instance);  \
        if (_timer >= delay) {                                            \
            active_scene.pool_manager.instances.at(#pool_instance)        \
                .deploy(&world, pos_x, pos_y, vel_x, vel_y);              \
            active_scene.pool_manager.instance_timer[#pool_instance] = 0; \
        }                                                                 \
    } while (0)

/**
 * @brief
 * Release a pooled body and return its memory to the pool.
 * Removes the body from the physics world.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param pool_instance pool identifier
 * @param body_ref pointer to the body to release
 * @return
 */
#define PULSE2D_DESPAWN(pool_instance, body_ref)               \
    do {                                                       \
        active_scene.pool_manager.instances.at(#pool_instance) \
            .retract(&world, (body_ref));                      \
    } while (0)

/**
 * @brief
 * Iterate over all active objects in a pool and execute an action for each.
 * Iterates in reverse to allow safe despawning during iteration.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param pool_instance pool identifier
 * @param action lambda receiving a Body* for each active object
 * @return
 */
#define PULSE2D_RENDER_POOL(pool_instance, action)                            \
    do {                                                                      \
        auto& _pool = active_scene.pool_manager.instances.at(#pool_instance); \
        auto& _objs = _pool.active_objects();                                 \
        for (int _i = static_cast<int>(_objs.size()) - 1; _i >= 0; --_i) {    \
            action(_objs[_i]);                                                \
        }                                                                     \
    } while (0)

////////////////////////
// Sprites, Rendering //
////////////////////////

/**
 * @brief
 * Register a flash-resident sprite array as a named sprite in the current
 * scene's pool.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param name sprite identifier
 * @param data_ptr pointer to the flash RGB565 pixel array
 * @param w sprite width in pixels
 * @param h sprite height in pixels
 * @return
 */
#define PULSE2D_SPRITE_FLASH(name, data_ptr, w, h)                           \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.set_from_flash(#name, data_ptr, w, h);             \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Project a physics body's world-space position to screen pixel coordinates.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param body_ptr pointer to the body
 * @return
 */
#define PULSE2D_BODY_COORDINATES(body_ptr)  \
    pulse2d::Renderer::project_coordinates( \
        (body_ptr)->position.x, (body_ptr)->position.y);

/**
 * @brief
 * Project a named body to screen coordinates and queue its sprite for
 * rendering.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param body_name named body identifier
 * @param sprite_name named sprite identifier
 * @param ... optional rotation in radians
 * @return
 */
#define PULSE2D_DRAW(body_name, sprite_name, ...)                            \
    do {                                                                     \
        auto& _body = active_scene.get_body(#body_name);                     \
        auto [sx, sy] = pulse2d::Renderer::project_coordinates(              \
            _body.position.x, _body.position.y);                             \
        const pulse2d::Sprite& _spr = active_scene.get_sprite(#sprite_name); \
        renderer.add_sprite(&_spr,                                           \
            static_cast<int16_t>(sx - _spr.width / 2),                       \
            static_cast<int16_t>(sy - _spr.height / 2) __VA_OPT__(, )        \
                __VA_ARGS__);                                                \
    } while (0)

/**
 * @brief
 * Like PULSE2D_DRAW, but takes a body pointer instead of a name.
 * Use inside PULSE2D_RENDER_POOL lambdas.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param body_ptr pointer to the body
 * @param sprite_name named sprite identifier
 * @param ... optional rotation in radians
 * @return
 */
#define PULSE2D_DRAW_BODY(body_ptr, sprite_name, ...)                        \
    do {                                                                     \
        auto [sx, sy] = pulse2d::Renderer::project_coordinates(              \
            (body_ptr)->position.x, (body_ptr)->position.y);                 \
        const pulse2d::Sprite& _spr = active_scene.get_sprite(#sprite_name); \
        renderer.add_sprite(&_spr,                                           \
            static_cast<int16_t>(sx - _spr.width / 2),                       \
            static_cast<int16_t>(sy - _spr.height / 2) __VA_OPT__(, )        \
                __VA_ARGS__);                                                \
    } while (0)

/**
 * @brief
 * Allocate an immovable body in the current scene's pool and register it
 * with the physics world.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param name body identifier
 * @param ... Body_Descriptor initializer
 * @return
 */
#define PULSE2D_STATIC_BODY(name, ...)                                       \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    pulse2d::graphics::detail::Body_Descriptor desc =        \
                        __VA_ARGS__;                                         \
                    scene.set(#name, desc);                                  \
                    world->add(&scene.get_body(#name));                      \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Allocate a body for player or externally driven objects. Semantically
 * distinct from PULSE2D_STATIC_BODY — use when the body is moved by gamepad
 * input or direct velocity assignment rather than physics forces.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param name body identifier
 * @param ... Body_Descriptor initializer
 * @return
 */
#define PULSE2D_CONTROLLED_BODY(name, ...)                                   \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    pulse2d::graphics::detail::Body_Descriptor desc =        \
                        __VA_ARGS__;                                         \
                    scene.set(#name, desc);                                  \
                    world->add(&scene.get_body(#name));                      \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Allocate a fully simulated dynamic body and register it with the world.
 * The body responds to forces, gravity, and collisions.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param name body identifier
 * @param ... Body_Descriptor initializer
 * @return
 */
#define PULSE2D_DYNAMIC_BODY(name, ...)                                      \
    do {                                                                     \
        std::visit(                                                          \
            [&](auto& scene) {                                               \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    pulse2d::graphics::detail::Body_Descriptor desc =        \
                        __VA_ARGS__;                                         \
                    scene.set(#name, desc);                                  \
                    scene.get_body(#name).set_motion();                      \
                    world->add(&scene.get_body(#name));                      \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/**
 * @brief
 * Return a reference to a named body from active_scene.
 * Requires PULSE2D_TICK_WORLD to have been called first.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param name body identifier
 * @return
 */
#define PULSE2D_GET_BODY(name) active_scene.get_body(#name)

/**
 * @brief
 * Load a sprite into the current scene's pool from the SD card.
 *
 * @scope: PULSE2D_ON_GAMESCENE_START
 * @param name sprite identifier
 * @param path sprite absolute path on sd card
 * @param w sprite width
 * @param h sprite height
 * @return
 */
#define PULSE2D_SPRITE(name, path, w, h)                                     \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.set(#name, engine->storage(), path, w, h);         \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

///////////
// Debug //
///////////

// Named free function required by etl::error_handler::set_callback<F>().
#if defined(DEBUG) && defined(PULSE2D_TEENSY)
namespace pulse2d::debug {
inline void etl_serial_error_handler(etl::exception const& e)
{
    if (Serial) {
        Serial.printf("[ETL] Error in %s:%d with '%s'\n",
            e.file_name(),
            e.line_number(),
            e.what());
        Serial.flush();
    }
}
} // namespace pulse2d::debug
#endif

#if defined(DEBUG)
/**
 * @brief
 * Print stack usage to serial every 300 frames (~5 seconds at 60 fps).
 * Compiled away in non-debug builds.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define PULSE2D_PRINT_STACKSIZE()                                            \
    do {                                                                     \
        static uint32_t frame = 0;                                           \
        if (++frame % 300 == 0)                                              \
            Serial.printf("stack used: %lu bytes\n", pulse2d::stack_used()); \
    } while (0)

/**
 * @brief
 * Register a Serial callback for ETL assertion failures.
 * Compiled away in non-debug builds.
 *
 * @scope: PULSE2D_ON_GAMESTART
 * @return
 */
#define PULSE2D_REGISTER_ETL_ERROR_HANDLER() \
    etl::error_handler::set_callback<pulse2d::debug::etl_serial_error_handler>()
#else
#define PULSE2D_PRINT_STACKSIZE()
#define PULSE2D_REGISTER_ETL_ERROR_HANDLER()
#endif
#endif
