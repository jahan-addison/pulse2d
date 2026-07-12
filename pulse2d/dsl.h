/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

/****************************************************************************
 * Internal DSL
 *
 * Macros and inline free functions that form the structural skeleton of a
 * pulse2d game. The DSL handles everything that belongs at file scope or
 * that does not require the engine or physics world:
 *
 * Type aliases, lifecycle hooks, scene declarations, animation blueprints,
 * gamepad input, coordinate helpers, and debug utilities.
 *
 * A minimal game skeleton looks like this:
 *
 *   #include PULSE2D_HEADER
 *   #include PULSE2D_GRAPHICS
 *
 *   PULSE2D_START_PULSE();
 *
 *   PULSE_DEFINE_SCENE(Level_One, 8, 6);
 *   PULSE_INIT_GAME(my_game, Level_One);
 *
 *   PULSE_ON_GAMESCENE_START(Level_One) {
 *       my_game.set_sprite("ship", "ship.bin", 48, 48);
 *       my_game.set_controlled_body("ship", {
 *          .position = { 0.0f, 0.0f },
 *          .width    = { 1.0f, 1.0f } });
 *   }
 *
 *   PULSE_ON_GAMESCENE(Level_One) {
 *       my_game.tick();
 *       PULSE_POLL_SEESAW_GAMEPAD();
 *       my_game.set_arcade_directional_control("ship", 5.0f);
 *       my_game.draw("ship", "ship");
 *       my_game.render();
 *   }
 *
 *   PULSE_ON_GAMESTART() {
 *       Serial.begin(115200);
 *       my_game.init(0.0f, 0.0f, 10);
 *       PULSE_ENABLE_SEESAW_GAMEPAD();
 *       PULSE_SET_SCENE(my_game, Level_One);
 *   }
 *
 *   PULSE_ON_GAMELOOP() {
 *     PULSE_TICK_GAMESCENE();
 *   }
 *
 ****************************************************************************/

#include <etl/array.h>
#include <etl/error_handler.h>
#include <etl/map.h>
#include <etl/vector.h>
#include <pulse2d/api.h>
#include <pulse2d/config.h>
#include <pulse2d/gamepad/seesaw.h>
#include <pulse2d/graphics/body.h>
#include <pulse2d/graphics/world.h>
#include <pulse2d/renderer.h>
#include <pulse2d/scene.h>
#include <pulse2d/sprite.h>
#include <pulse2d/state.h>
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
using pulse2d_color = pulse2d::Renderer::Color;

// Alias for the Runtime template - used in PULSE_SCENE_FN function signatures.
template<pulse2d::Scene... Scenes>
using pulse2d_scene_runtime = pulse2d::runtime::Runtime<Scenes...>;

// Portable primitive aliases - useful for pool lambda parameters and state.

using p_ui32 = uint32_t;
using p_ui16 = uint16_t;
using p_ui8 = uint8_t;
using p_i32 = int32_t;
using p_i16 = int16_t;
using p_i8 = int8_t;

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

// Other Aliases
namespace pulse2d_math = pulse2d::graphics::math;
namespace pulse2d_util = pulse2d::util;
namespace pulse2d_state = pulse2d::state;
namespace sml = boost::sml;

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
 * Define a type in Static_Inplace_T for deferred construction,
 * Types constructed in this way should require the arduino runtime.
 *
 * @scope: global
 * @param type the class type
 * @return
 *
 */
#define PULSE_HARDWARE_DEFINE(type) PULSE_DEFINE pulse2d::Static_Inplace_T<type>

/**
 * @brief
 * Declare the canonical static state instance for a level namespace.
 * Always named `state`; access it as `level_name::state` from the game
 * translation unit.
 *
 * Define a State struct in the level namespace first, then call this macro
 * immediately after to give it static storage duration and zero-initialize it.
 *
 *   namespace level_one {
 *       struct State {
 *           int   score    = 0;
 *           float speed    = 1.0f;
 *           bool  complete = false;
 *       };
 *       PULSE_DEFINE_SCENE_STATE(State);
 *   }
 *
 *   // from game translation unit:
 *   level_one::state.score += 10;
 *
 * @scope: namespace (level header)
 * @param Type the State struct type
 */
#define PULSE_DEFINE_SCENE_STATE(Type) \
    PULSE_DEFINE Type state {}

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
    void (*active_scene_fn)() = nullptr;    \
    using namespace pulse2d::runtime

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
    {};                                                                      \
    void pulse2d_scene_enter_##name();                                       \
    void pulse2d_scene_fn_##name()

/**
 * @brief
 * Declare a template function constrained to valid scene types
 * (pulse2d::Scene). The type parameter pack is always named Scenes and is
 * available in the function signature and body. Use for level or scene utility
 * functions that take a Runtime<Scenes...>& without being tied to a concrete
 * scene.
 *
 * @note Functions must spell the type parameter as Scenes - the macro fixes
 *       the name so call sites can deduce Scenes from the Runtime<Scenes...>&
 *       argument without an explicit template argument.
 *
 *   PULSE_SCENE_FN void setup_walls(pulse2d_scene_runtime<Scenes...>& game)
 *   {
 *       game.set_static_body("floor", { .position = { 0.0f, -5.0f },
 *                                       .width    = { 10.0f, 0.5f } });
 *   }
 *
 *   // called from PULSE_ON_GAMESCENE_START(Level_One) - Scenes deduced as
 * Level_One setup_walls(my_game);
 *
 * @scope: global
 * @return
 */
#define PULSE_SCENE_FN                 \
    template<pulse2d::Scene... Scenes> \
    PULSE2D_INLINE

/**
 * @brief
 * Clear the physics world and storage, then enter and register the given scene.
 *
 * @scope: PULSE_ON_GAMESTART
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
 * Safe to call mid-tick: the current scene function runs to completion
 * (including render()) before the transition executes.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @param game game runtime instance
 * @param scene scene type to transition into
 * @return
 */
#define PULSE_DEFER_SCENE(game, scene) \
    pending_transition = []() { PULSE_SET_SCENE(game, scene); }

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
 * scene transition. The only call needed in PULSE_ON_GAMELOOP.
 *
 * @scope: PULSE_ON_GAMELOOP
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

/**
 *
 * @brief
 * Enable and initialize the Seesaw gamepad hardware over I2C.
 *
 * @scope: PULSE_ON_GAMESTART
 * @return
 */
#define PULSE_ENABLE_SEESAW_GAMEPAD() gamepad_hal.start()

/**
 * @brief
 * Poll all inputs for the current frame, and brings gamepad_state into scope.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @return
 */
#define PULSE_POLL_SEESAW_GAMEPAD() \
    gamepad_hal.pad->poll();        \
    [[maybe_unused]] auto& gamepad_state = gamepad_hal.pad->get_state();

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
 * @scope: PULSE_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTIONAL_X_INPUT() gamepad_state.stick_x

/**
 * @brief
 * Raw analog stick Y axis, normalized -1.0f to +1.0f.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTIONAL_Y_INPUT() gamepad_state.stick_y

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude leftward.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_LEFT() (gamepad_state.stick_x < -0.5f)

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude rightward.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_RIGHT() (gamepad_state.stick_x > 0.5f)

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude upward.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_UP() (gamepad_state.stick_y < -0.5f)

/**
 * @brief
 * True when the analog stick exceeds 0.5 magnitude downward.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @return
 */
#define SEESAW_DIRECTION_IS_DOWN() (gamepad_state.stick_y > 0.5f)

/**
 * @brief
 * True while the named button is held.
 *
 * @scope: PULSE_ON_GAMESCENE
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
 * @scope: PULSE_ON_GAMESCENE
 * @param animator_inst Sprite_Animator instance
 * @param anim_def Animation_Def to play
 * @return
 */
PULSE2D_INLINE
void register_animation(pulse2d::Sprite_Animator& animator_inst,
    pulse2d::assets::Animation_Def const& anim_def)
{
    animator_inst.set_state(&anim_def);
}

/**
 * @brief
 * Project a physics body's world-space position to screen pixel coordinates.
 *
 * @scope: PULSE_ON_GAMESCENE
 * @param body_ptr pointer to the body
 * @return Renderer::Screen with int16_t x and y pixel coordinates
 */
PULSE2D_INLINE pulse2d::Renderer::Screen get_body_coordinates(
    pulse2d_body const* body_ptr)
{
    return pulse2d::Renderer::units_to_pixels(
        body_ptr->position.x, body_ptr->position.y);
}

/**
 * @brief
 * Get the pixel coordinates of units in physics space
 *
 * @scope: PULSE_ON_GAMESCENE
 * @param body_ptr pointer to the body
 * @return Renderer::Screen with int16_t x and y pixel coordinates
 */
PULSE2D_INLINE pulse2d::Renderer::Screen units_to_pixels(float unit_w,
    float unit_h)
{
    return pulse2d::Renderer::units_to_pixels(unit_w, unit_h);
}

//////////////
// Renderer //
//////////////

constexpr pulse2d::graphics::math::Vec2 pixels_to_units(float px_w,
    float px_h,
    float px_per_unit = pulse2d::config::pixels_per_unit)
{
    return { px_w / px_per_unit, px_h / px_per_unit };
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
PULSE2D_INLINE void etl_serial_error_handler(etl::exception const& e)
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
 * @scope: PULSE_ON_GAMESCENE
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
 * @scope: PULSE_ON_GAMESCENE
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
