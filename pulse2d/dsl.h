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
// Define a variable
#define PULSE2D_DEFINE static

// Define a type in HARDWARE_Deferred_Init for deferred construction
#define PULSE2D_HARDWARE_DEFINE(type) \
    PULSE2D_DEFINE pulse2d::HARDWARE_Deferred_Init<type>

// Maps to Arduino setup(). Runs once on power-on.
#define PULSE2D_ON_GAMESTART() void setup()
// Maps to Arduino loop(). Runs every frame.
#define PULSE2D_ON_GAMELOOP() void loop()

// Define a static Body in global (BSS) storage
#define PULSE2D_BODY static pulse2d::graphics::Body
// Define a static Sprite in global (BSS) storage
#define PULSE2D_SPRITE static pulse2d::Sprite

// Declare the engine, physics world, and scene dispatch pointers
#define PULSE2D_START_PULSE()                                \
    PULSE2D_HARDWARE_DEFINE(pulse2d::Pulse2d) engine;        \
    PULSE2D_HARDWARE_DEFINE(pulse2d::graphics::World) world; \
    void (*pending_transition)() = nullptr;                  \
    void (*active_scene_fn)() = nullptr;                     \
    PULSE2D_DEFINE constexpr float PULSE = 1.0f / 60.0f;

// Initialize the engine and physics world
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

// Declare the I2C driver and gamepad
#define PULSE2D_ENABLE_SEESAW_GAMEPAD()               \
    static pulse2d::gamepad::Teensy_I2CDriver driver; \
    static pulse2d::gamepad::Seesaw_Gamepad pad(driver)

// Initialize the gamepad hardware
#define PULSE2D_START_SEESAW_GAMEPAD()                         \
    do {                                                       \
        if (!pad.init()) {                                     \
            Serial.println("[ERROR]: Seesaw Gamepad Failed!"); \
        }                                                      \
    } while (0)

// Polls all inputs for the current frame and brings gamepad_state into scope
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

// Raw analog stick axis, normalized -1.0f to +1.0f
#define SEESAW_DIRECTIONAL_X_INPUT() gamepad_state.stick_x
#define SEESAW_DIRECTIONAL_Y_INPUT() gamepad_state.stick_y

// Direction state when dpad is pushed more than halfway in the given direction
#define SEESAW_DIRECTION_IS_LEFT() (gamepad_state.stick_x < -0.5f)
#define SEESAW_DIRECTION_IS_RIGHT() (gamepad_state.stick_x > 0.5f)

#define SEESAW_DIRECTION_IS_UP() (gamepad_state.stick_y < -0.5f)
#define SEESAW_DIRECTION_IS_DOWN() (gamepad_state.stick_y > 0.5f)

////////////////////////////////////////////////////////////////////////////////
// Profile A: The Arcade Controller (Pokemon, Zelda, Pac-Man)
// Sets velocity directly from stick position for instant response and instant
// stops.
////////////////////////////////////////////////////////////////////////////////
#define SEESAW_ARCADE_DIRECTIONAL_MOVEMENT(body_name, max_speed) \
    do {                                                         \
        auto& _body = active_scene.get_body(body_name);          \
        pulse2d::gamepad::util::apply_arcade_movement(           \
            (_body), pad.get_state(), (max_speed));              \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// Profile A, inverted: Same as SEESAW_ARCADE_DIRECTIONAL_MOVEMENT with the Y
// and X axis inverted.
////////////////////////////////////////////////////////////////////////////////
#define SEESAW_ARCADE_DIRECTIONAL_MOVEMENT_INVERTED(body_name, max_speed) \
    do {                                                                  \
        auto& _body = active_scene.get_body(body_name);                   \
        pulse2d::gamepad::util::apply_inverted_arcade_movement(           \
            (_body), pad.get_state(), (max_speed));                       \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// Profile B: The Momentum Controller (Asteroids, Mario).
// Applies a thrust force scaled by acceleration each frame - velocity builds up
// over time.
////////////////////////////////////////////////////////////////////////////////
#define SEESAW_DYNAMIC_DIRECTIONAL_MOVEMENT(body_name, acceleration) \
    do {                                                             \
        auto& _body = active_scene.get_body(body_name);              \
        pulse2d::gamepad::util::apply_dynamic_thrust(                \
            (_body), pad.get_state(), (acceleration));               \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// Profile C: Top-Down Friction. Applies linear drag to the body each frame.
// Pair with SEESAW_DYNAMIC_DIRECTIONAL_MOVEMENT so the body doesn't slide
// forever.
////////////////////////////////////////////////////////////////////////////////
#define SEESAW_SLIDING_FRICTION_DIRECTIONAL_MOVEMENT(body_name, drag_amount) \
    do {                                                                     \
        auto& _body = active_scene.get_body(body_name);                      \
        pulse2d::gamepad::util::apply_linear_drag((_body), (drag_amount));   \
    } while (0)

// Check whether a button is currently held
// clang-format off
#define SEESAW_BUTTON_INPUT(NAME) gamepad_state.buttons & NAME
// clang-format on

////////////
// Scenes //
////////////

// Declare a scene with body, sprite, and joint pool sizes
#define PULSE2D_DEFINE_SCENE(name, bodies, sprites, ...) \
    struct name                                          \
        : pulse2d::teensy::Pulse2d_Level<bodies,         \
              sprites __VA_OPT__(, ) __VA_ARGS__>        \
    {}

// Clear the physics world and storage, then enters and registers the given
// scene.
#define PULSE2D_SET_SCENE(scene)                                \
    do {                                                        \
        world->clear();                                         \
        engine->storage().reset();                              \
        pulse2d::teensy::Pulse2d_Scene_Base::total_bodies = 0;  \
        pulse2d::teensy::Pulse2d_Scene_Base::total_sprites = 0; \
        current_scene.emplace<scene>();                         \
        pulse2d_scene_enter_##scene();                          \
        active_scene_fn = pulse2d_scene_fn_##scene;             \
    } while (0)

// Declare the all scene types used in the game.
#define PULSE2D_GAME_SCENES(...)                                           \
    PULSE2D_DEFINE std::variant<std::monostate __VA_OPT__(, ) __VA_ARGS__> \
        current_scene

// Steps the physics simulation one frame. Brings active_scene and renderer into
// scope
#define PULSE2D_TICK_WORLD(scene)                                         \
    [[maybe_unused]] auto& active_scene = std::get<scene>(current_scene); \
    world->step(PULSE);                                                   \
    [[maybe_unused]] auto& renderer = engine->renderer()

// Flush renderer's sprite queue to the display
#define PULSE2D_RENDER(active_scene) engine->tick(*world)

// Define the entry function for a scene. Called once by PULSE2D_SET_SCENE
#define PULSE2D_ON_GAMESCENE_START(scene) void pulse2d_scene_enter_##scene()

// Define the per-frame function for a scene
#define PULSE2D_ON_GAMESCENE(scene) void pulse2d_scene_fn_##scene()

// Call the active scene's per-frame function, then resolve any pending
// transition. Note: This is the only call needed in PULSE2D_ON_GAMELOOP
#define PULSE2D_TICK_GAMESCENE()             \
    do {                                     \
        if (active_scene_fn != nullptr)      \
            active_scene_fn();               \
        if (pending_transition != nullptr) { \
            pending_transition();            \
            pending_transition = nullptr;    \
        }                                    \
    } while (0)

// Tick the game engine
#define PULSE2D_TICK_PULSE() engine->tick(*world)

///////////////
// Collision //
///////////////

// Conditional block that executes when at least one collision is active in the
// world
#define PULSE2D_ON_COLLISION() if (!world->arbiters.empty())

// Conditional block that executes when a specific named arbiter is active
#define PULSE2D_ON_COLLISION_WITH(with) if (world->arbiters.contains(#with))

////////////////
// Background //
////////////////

// Add parallax background scrolling layer
#define PULSE2D_ADD_SCROLLING_LAYER(sprite_name, width, speed)               \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.background_layers.push_back(                       \
                        { (#sprite_name), (width), (speed), 0.0f });         \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

// Ticks and renders all backgrounds directly to the screen
#define PULSE2D_RENDER_BACKGROUNDS()                                         \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    auto& _bg_renderer = engine->renderer();                 \
                    scene.update_and_draw_parallax(_bg_renderer, PULSE);     \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

/////////////
// Sprites //
/////////////

// Set a sprite from flash memory
#define PULSE2D_SET_SPRITE_FLASH(name, data_ptr, w, h)                       \
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

// Project a body's world-space position to screen coordinates and queue its
// sprite. Note: optional third argument to set a fixed rotation in radians
#define PULSE2D_DRAW(body_name, sprite_name, ...)                            \
    do {                                                                     \
        auto& _body = active_scene.get_body(body_name);                      \
        auto [sx, sy] = pulse2d::Renderer::project_coordinates(              \
            _body.position.x, _body.position.y);                             \
        const pulse2d::Sprite& _spr = active_scene.get_sprite(#sprite_name); \
        renderer.add_sprite(&_spr,                                           \
            static_cast<int16_t>(sx - _spr.width / 2),                       \
            static_cast<int16_t>(sy - _spr.height / 2) __VA_OPT__(, )        \
                __VA_ARGS__);                                                \
    } while (0)

// Allocate an immovable body in the current scene's pool and registers it with
// the world.
#define PULSE2D_SPAWN_STATIC_BODY(name, ...)                                 \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    pulse2d::graphics::detail::Body_Descriptor desc =        \
                        __VA_ARGS__;                                         \
                    scene.set(name, desc);                                   \
                    world->add(&scene.get_body(name));                       \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

// Allocate a dynamic body in the current scene's pool, enables physics, and
// registers it with the world.
#define PULSE2D_SPAWN_BODY(name, ...)                                        \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    pulse2d::graphics::detail::Body_Descriptor desc =        \
                        __VA_ARGS__;                                         \
                    scene.set(name, desc);                                   \
                    scene.get_body(name).set_motion();                       \
                    world->add(&scene.get_body(name));                       \
                }                                                            \
            },                                                               \
            current_scene);                                                  \
    } while (0)

// Return a reference to a named body from active_scene
#define PULSE2D_GET_BODY(name) active_scene.get_body(name)

// Load a sprite into the current scene's pool from the SD card
#define PULSE2D_SET_SPRITE(name, path, x, y)                                 \
    do {                                                                     \
        std::visit(                                                          \
            [](auto& scene) {                                                \
                if constexpr (!std::is_same_v<std::decay_t<decltype(scene)>, \
                                  std::monostate>) {                         \
                    scene.set(#name, engine->storage(), path, x, y);         \
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
inline void etl_serial_error_handler(const etl::exception& e)
{
    if (Serial) {
        Serial.print("[ETL ERROR] ");
        Serial.print(e.file_name());
        Serial.print(":");
        Serial.print(e.line_number());
        Serial.print(" ");
        Serial.println(e.what());
        Serial.flush();
    }
}
} // namespace pulse2d::debug
#endif

#if defined(DEBUG)
// Print stack usage to serial every 300 frames
#define PULSE2D_PRINT_STACKSIZE()                                          \
    do {                                                                   \
        static uint32_t frame = 0;                                         \
        if (++frame % 300 == 0)                                            \
            Serial.printf(                                                 \
                "stack used: %lu bytes\n", pulse2d::teensy::stack_used()); \
    } while (0)

// Register a Serial error handler for ETL assertion failures.
#define PULSE2D_REGISTER_ETL_ERROR_HANDLER() \
    etl::error_handler::set_callback<pulse2d::debug::etl_serial_error_handler>()
#else
#define PULSE2D_PRINT_STACKSIZE()
#define PULSE2D_REGISTER_ETL_ERROR_HANDLER()
#endif
#endif

namespace pulse2d::teensy {

#if defined(PULSE2D_TEENSY)
extern "C" uint32_t _ebss;
extern "C" uint32_t _estack;

struct CStrLess
{
    bool operator()(const char* a, const char* b) const noexcept
    {
        return __builtin_strcmp(a, b) < 0;
    }
};

struct Parallax_Layer
{
    const char* sprite_name;
    float width;
    float scroll_speed;
    float current_offset = 0.0f;
};

struct Pulse2d_Scene_Base
{
    inline static std::size_t total_bodies = 0;
    inline static std::size_t total_sprites = 0;
};

template<std::size_t T_Body, std::size_t T_Sprite, std::size_t T_Joint = 0>
struct Pulse2d_Scene : Pulse2d_Scene_Base
{
    static_assert(T_Body <= MAX_PHYSICS_BODIES,
        "T_Body exceeds MAX_PHYSICS_BODIES");
    static_assert(T_Joint <= MAX_PHYSICS_JOINTS,
        "T_Joint exceeds MAX_PHYSICS_JOINTS");

    inline graphics::Body& get_body(const char* name)
    {
        return bodies.at(body_pool.at(name));
    }
    inline Sprite& get_sprite(const char* name)
    {
        return sprites.at(sprite_pool.at(name));
    }
    inline void set(const char* name,
        pulse2d::graphics::detail::Body_Descriptor& body)
    {
        if (total_bodies >= MAX_PHYSICS_BODIES) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] body pool full, cannot spawn '%s'\n", name);
            return;
        }
        body_pool[name] = active_bodies;
        bodies.at(active_bodies++).set(body);
        ++total_bodies;
    }
    inline void set(const char* name,
        pulse2d::Storage& storage,
        const char* path,
        uint16_t x,
        uint16_t y)
    {
        if (total_sprites >= pulse2d::Storage::k_max_loaded_sprites) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite pool full, cannot load '%s'\n", path);
            return;
        }
        sprite_pool[name] = active_sprites;
        sprites.at(active_sprites++) = storage.load_sprite(path, x, y);
        ++total_sprites;
        if (sprites[active_sprites - 1].data == nullptr) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite load failed: '%s' -> '%s'\n", name, path);
        } else {
            PULSE2D_DEBUG_SERIAL("sprite '%s' ready: %ux%u\n",
                name,
                sprites[active_sprites - 1].width,
                sprites[active_sprites - 1].height);
        }
    }

    inline void set_from_flash(const char* name,
        uint16_t const* flash_data,
        uint16_t w,
        uint16_t h)
    {
        if (total_sprites >= pulse2d::Storage::k_max_loaded_sprites ||
            active_sprites >= T_Sprite) {
            PULSE2D_DEBUG_SERIAL(
                "[WARN] sprite pool full, cannot load flash sprite '%s'\n",
                name);
            return;
        }
        sprite_pool[name] = active_sprites;

        sprites.at(active_sprites).data = flash_data;
        sprites.at(active_sprites).width = w;
        sprites.at(active_sprites).height = h;

        ++active_sprites;
        ++total_sprites;

        PULSE2D_DEBUG_SERIAL("flash sprite '%s' ready: %ux%u\n", name, w, h);
    }

    inline void update_and_draw_parallax(pulse2d::Renderer& renderer,
        float delta_time)
    {
        for (auto& layer : background_layers) {
            // Skip any layer whose sprite was not registered (e.g. because
            // T_Sprite in PULSE2D_DEFINE_SCENE is fewer than the number of
            // PULSE2D_ADD_SCROLLING_LAYER calls, or set_from_flash was never
            // called for it). Without this guard, sprite_pool.at() would hit
            // an ETL assertion and hard-fault the board on the first loop tick.
            auto it = sprite_pool.find(layer.sprite_name);
            if (it == sprite_pool.end()) {
                PULSE2D_DEBUG_SERIAL(
                    "[WARN] parallax: sprite '%s' not registered, skipping\n",
                    layer.sprite_name);
                continue;
            }

            layer.current_offset += layer.scroll_speed * delta_time;
            layer.current_offset = std::fmod(layer.current_offset, layer.width);

            float draw_x = -layer.current_offset;

            const pulse2d::Sprite& _spr = sprites.at(it->second);

            renderer.add_sprite(&_spr, static_cast<int16_t>(draw_x), 0);
            renderer.add_sprite(
                &_spr, static_cast<int16_t>(draw_x + layer.width), 0);
        }
    }

    etl::array<pulse2d::graphics::Body, T_Body> bodies;
    etl::array<pulse2d::graphics::Joint, T_Joint> joints;
    etl::vector<Parallax_Layer, 8> background_layers;
    etl::array<pulse2d::Sprite, T_Sprite> sprites;

    etl::map<const char*, std::size_t, T_Body, CStrLess> body_pool;
    etl::map<const char*, std::size_t, T_Sprite, CStrLess> sprite_pool;

    std::size_t active_bodies{ 0 };
    std::size_t active_sprites{ 0 };
};

template<std::size_t T_Body, std::size_t T_Sprite, std::size_t T_Joint = 0>
using Pulse2d_Level = Pulse2d_Scene<T_Body, T_Sprite, T_Joint>;

uint32_t inline stack_used()
{
    const uint32_t* p = (const uint32_t*)&_ebss;
    uint32_t count = 0;
    while (*p++ == 0xA5A5A5A5)
        count += 4;
    return (uint32_t)&_estack - (uint32_t)&_ebss - count; // bytes consumed
}
#endif

} // namespace teensy
