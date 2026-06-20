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
#include <pulse2d/api.h>
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

#if defined(PULSE2D_TEENSY)

//////////////////
// Type aliases //
//////////////////

// Short names for engine types used throughout game code.
using pulse2d_body = pulse2d::graphics::Body;
using pulse2d_world = pulse2d::graphics::World;
using pulse2d_arbiter = pulse2d::graphics::Arbiter;
using pulse2d_joint = pulse2d::graphics::Joint;

// Portable primitive aliases - useful for pool lambda parameters and state.
using p_bool = bool;
using p_float = float;

using p_uint32 = uint32_t;
using p_uint16 = uint16_t;
using p_uint8 = uint8_t;
using p_int32 = int32_t;
using p_int16 = int16_t;
using p_int8 = int8_t;

//////////////////
// Math helpers //
//////////////////

// Explicit narrowing casts: use these when passing arithmetic results to APIs
// that take a specific integer width (e.g. to_int16(coords.x + offset)).
#define to_uint32(x) static_cast<uint32_t>(x)
#define to_uint16(x) static_cast<uint16_t>(x)
#define to_uint8(x) static_cast<uint8_t>(x)
#define to_int32(x) static_cast<int32_t>(x)
#define to_int16(x) static_cast<int16_t>(x)
#define to_int8(x) static_cast<int8_t>(x)

// Alias for the physics math namespace (Vec2, Mat22, etc.).
namespace pulse2d_math = pulse2d::graphics::math;

////////////////
// Game State //
////////////////

/**
 * @brief
 * Define a BSS memory section
 *
 * @scope: global
 * @return
 *
 */
#define PULSE_DEFINE static

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
#define PULSE_HARDWARE_DEFINE(type) \
    PULSE_DEFINE pulse2d::HARDWARE_Deferred_Init<type>

/**
 * @brief
 * Instantiate the runtime for the game and set list of scenes
 *
 * @scope: global
 * @param game_name name of game
 * @param scene1
 * @param scene2
 * ...
 * @return
 *
 */
#define PULSE_INIT_GAME(game_name, ...)       \
    using game_name_t = Runtime<__VA_ARGS__>; \
    static game_name_t game_name {}

/**
 * @brief
 * Maps to Arduino setup(). Runs once on power-on.
 *
 * @scope: global
 * @return
 */
#define PULSE_ON_GAMESTART() void setup()

/**
 * @brief
 * Maps to Arduino loop(). Runs every frame (~60 Hz).
 *
 * @scope: global
 * @return
 */
#define PULSE_ON_GAMELOOP() void loop()

/**
 * @brief
 * Declare the scene dispatch function pointers.
 * Creates: pending_transition, active_scene_fn.
 * Use PULSE_INIT_GAME to create the Runtime instance (engine, world, scenes).
 *
 * @scope: global
 * @return
 */
#define PULSE2D_START_PULSE()               \
    void (*pending_transition)() = nullptr; \
    void (*active_scene_fn)() = nullptr;

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
#define PULSE_DEFINE_SCENE(name, bodies, sprites, ...)                       \
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
#define PULSE_SET_SCENE(game, scene)                \
    do {                                            \
        game.world->clear();                        \
        game.engine->storage().reset();             \
        game.current_scene.emplace<scene>();        \
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
#define PULSE_DEFER_SCENE(to) \
    pending_transition = []() { PULSE2D_SET_SCENE(to); }

/**
 * @brief
 * Define the entry function for a scene, called once by PULSE2D_SET_SCENE.
 *
 * @scope: global
 * @param scene scene identifier
 * @return
 */
#define PULSE_ON_GAMESCENE_START(scene) void pulse2d_scene_enter_##scene()

/**
 * @brief
 * Define the per-frame function for a scene.
 *
 * @scope: global
 * @param scene scene identifier
 * @return
 */
#define PULSE_ON_GAMESCENE(scene) void pulse2d_scene_fn_##scene()

/**
 * @brief
 * Call the active scene's per-frame function, then resolve any pending
 * scene transition. The only call needed in PULSE2D_ON_GAMELOOP.
 *
 * @scope: PULSE2D_ON_GAMELOOP
 * @return
 */
#define PULSE_TICK_GAMESCENE()               \
    do {                                     \
        if (active_scene_fn != nullptr)      \
            active_scene_fn();               \
        if (pending_transition != nullptr) { \
            pending_transition();            \
            pending_transition = nullptr;    \
        }                                    \
    } while (0)

/////////////
// Gamepad //
/////////////

/**
 * @brief
 * Initialize the Seesaw gamepad hardware over I2C.
 *
 * @scope: PULSE2D_ON_GAMESTART
 * @return
 */
PULSE2D_INLINE void start_seesaw_gamepad()
{
    if (!pad.init()) {
        Serial.println("[ERROR]: Seesaw Gamepad Failed!");
    }
}

/**
 * @brief
 * Poll all inputs for the current frame, and brings gamepad_state into scope.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
#define PULSE_POLL_SEESAW_GAMEPAD() \
    pad.poll();                     \
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
#define PULSE_DEFINE_ANIMATOR(name) \
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
#define PULSE_ANIMATION_DEFINITION(                              \
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
PULSE2D_INLINE void register_animation(pulse2d::Sprite_Animator& animator_inst,
    pulse2d::assets::Animation_Def const& anim_def)
{
    animator_inst.set_state(&anim_def);
}

/**
 * @brief
 * Project a physics body's world-space position to screen pixel coordinates.
 * Returns a Renderer::Screen {int16_t x, int16_t y} struct. Use as an
 * expression: auto coords = PULSE2D_BODY_COORDINATES(ptr); then access
 * coords.x and coords.y. Apply to_int16() when doing arithmetic on the fields
 * before passing to PULSE2D_PLAY_VFX.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @param body_ptr pointer to the body
 * @return Renderer::Screen with int16_t x and y pixel coordinates
 */
pulse2d::Renderer::Screen get_body_coordinates(pulse2d_body const* body_ptr)
{
    return pulse2d::Renderer::project_coordinates(
        body_ptr->position.x, body_ptr->position.y);
}

///////////
// Debug //
///////////

#define PULSE_POLL_SERIAL_CONNECTION()              \
    do {                                            \
        while (!Serial)                             \
            ;                                       \
        Serial.println("[DEBUG] setup: serial OK"); \
    } while (0)

// Named free function required by etl::error_handler::set_callback<F>().
#if defined(DEBUG)
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

/**
 * @brief
 * Print stack usage to serial every 300 frames (~5 seconds at 60 fps).
 * Compiled away in non-debug builds.
 *
 * @scope: PULSE2D_ON_GAMESCENE
 * @return
 */
PULSE2D_INLINE void pulse_print_stacksize()
{
    static uint32_t frame = 0;
    if (++frame % 300 == 0)
        Serial.printf("stack used: %lu bytes\n", ::pulse2d::stack_used());
}

/**
 * @brief
 * Register a Serial callback for ETL assertion failures.
 * Compiled away in non-debug builds.
 *
 * @scope: PULSE2D_ON_GAMESTART
 * @return
 */
PULSE2D_INLINE void pulse_register_etl_error_handler()
{
    etl::error_handler::set_callback<
        pulse2d::debug::etl_serial_error_handler>();
}
#else
PULSE2D_INLINE void pulse_print_stacksize() {}
PULSE2D_INLINE void pulse_register_etl_error_handler() {}
#endif
#endif
