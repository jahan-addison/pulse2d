/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <etl/array.h>
#include <etl/map.h>
#include <etl/vector.h>
#include <pulse2d/graphics/body.h>
#include <pulse2d/graphics/world.h>
#include <pulse2d/sprite.h>
#include <pulse2d/storage.h>
#include <pulse2d/util.h>
#include <variant>

/////////////////////////////
// Teensy Development DSL  //
/////////////////////////////

#if defined(PULSE2D_TEENSY)
#define PULSE2D_DEFINE static

#define PULSE2D_HARDWARE_DEFINE(type) \
    PULSE2D_DEFINE pulse2d::HARDWARE_Deferred_Init<type>

#define PULSE2D_ON_GAMESTART() void setup()
#define PULSE2D_ON_GAMELOOP() void loop()

#define PULSE2D_BODY static pulse2d::graphics::Body
#define PULSE2D_SPRITE static pulse2d::Sprite

#define PULSE2D_START_PULSE()                                \
    PULSE2D_HARDWARE_DEFINE(pulse2d::Pulse2d) engine;        \
    PULSE2D_HARDWARE_DEFINE(pulse2d::graphics::World) world; \
    void (*pending_transition)() = nullptr;                  \
    void (*active_scene_fn)() = nullptr;                     \
    PULSE2D_DEFINE constexpr float PULSE = 1.0f / 60.0f;

#define PULSE2D_DEFINE_LEVEL(name, bodies, sprites, ...) \
    struct name                                          \
        : pulse2d::teensy::Pulse2d_Level<bodies,         \
              sprites __VA_OPT__(, ) __VA_ARGS__>        \
    {}

#define PULSE2D_SET_SCENE(scene)                            \
    world->clear();                                         \
    engine->storage().reset();                              \
    pulse2d::teensy::Pulse2d_Scene_Base::total_bodies = 0;  \
    pulse2d::teensy::Pulse2d_Scene_Base::total_sprites = 0; \
    current_scene.emplace<scene>();                         \
    pulse2d_scene_enter_##scene();                          \
    active_scene_fn = pulse2d_scene_fn_##scene

#define PULSE2D_GAME_LEVELS(...)                                           \
    PULSE2D_DEFINE std::variant<std::monostate __VA_OPT__(, ) __VA_ARGS__> \
        current_scene

#define PULSE2D_TICK_WORLD(SceneName)                        \
    auto& active_scene = std::get<SceneName>(current_scene); \
    world->step(PULSE);                                      \
    auto& renderer = engine->renderer()

#define PULSE2D_RENDER(active_scene) engine->tick(*world)

#define PULSE2D_ON_GAMESCENE_START(SceneName) \
    void pulse2d_scene_enter_##SceneName()

#define PULSE2D_ON_GAMESCENE(SceneName) void pulse2d_scene_fn_##SceneName()

#define PULSE2D_TICK_GAMESCENE()             \
    do {                                     \
        if (active_scene_fn != nullptr)      \
            active_scene_fn();               \
        if (pending_transition != nullptr) { \
            pending_transition();            \
            pending_transition = nullptr;    \
        }                                    \
    } while (0)

#define PULSE2D_TICK_PULSE() engine->tick(*world)

#define PULSE2D_ON_COLLISION() if (!world->arbiters.empty())

#define PULSE2D_ON_COLLISION_WITH(with) if (world->arbiters.contains(#with))

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

#define PULSE2D_GET_BODY(name) active_scene.get_body(name)

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

#if defined(DEBUG)
#define PULSE2D_PRINT_STACKSIZE()                                          \
    do {                                                                   \
        static uint32_t frame = 0;                                         \
        if (++frame % 300 == 0)                                            \
            Serial.printf(                                                 \
                "stack used: %lu bytes\n", pulse2d::teensy::stack_used()); \
    } while (0)
#else
#define PULSE2D_PRINT_STACKSIZE()
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
    etl::array<pulse2d::graphics::Body, T_Body> bodies;
    etl::array<pulse2d::graphics::Joint, T_Joint> joints;
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
