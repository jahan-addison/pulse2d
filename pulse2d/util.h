/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cstddef>     // for std::byte
#include <cstdint>     // for uint32_t
#include <type_traits> // std::is_same_v
#include <utility>     // for std::forward

#ifdef __IMXRT1062__
#define PULSE2D_TEENSY __IMXRT1062__
#endif

#ifdef PULSE2D_TESTING
#define PULSE2D_PRIVATE public
#else
#define PULSE2D_PRIVATE private
#endif

//////////////////////
// Teensy Debugging //
//////////////////////

#if defined(PULSE2D_TEENSY)
/**
 * @brief Place a variable in OCRAM (the secondary 512 KB RAM bank on the
 *  i.MX RT1062)
 */
#define PULSE2D_EXTMEM __attribute__((section(".dmabuffers"), used))
#else
#define PULSE2D_EXTMEM
#endif
#if defined(PULSE2D_TEENSY) && defined(DEBUG)
#define PULSE2D_DEBUG_SERIAL(...)       \
    do {                                \
        if (Serial) {                   \
            Serial.print("[DEBUG] ");   \
            Serial.printf(__VA_ARGS__); \
            Serial.println();           \
        }                               \
    } while (0)
#define PULSE2D_POLL_SERIAL_CONNECTION()            \
    do {                                            \
        while (!Serial)                             \
            ;                                       \
        Serial.println("[DEBUG] setup: serial OK"); \
    } while (0)
#else
#define PULSE2D_DEBUG_SERIAL(...)
#define PULSE2D_POLL_SERIAL_CONNECTION()
#endif

/////////////////////////////
// Teensy Development DSL  //
/////////////////////////////

#if defined(PULSE2D_TEENSY)
#define PULSE2D_DEFINE static

#define PULSE2D_HARDWARE_DEFINE(type) \
    PULSE2D_DEFINE pulse2d::HARDWARE_Deferred_Init<type>
#endif

#define PULSE2D_ON_GAMESTART() void setup()
#define PULSE2D_ON_GAMELOOP() void loop()

#define PULSE2D_BODY static pulse2d::graphics::Body
#define PULSE2D_SPRITE static pulse2d::Sprite

#define PULSE2D_START_PULSE()                                \
    PULSE2D_HARDWARE_DEFINE(pulse2d::Pulse2d) engine;        \
    PULSE2D_HARDWARE_DEFINE(pulse2d::graphics::World) world; \
    PULSE2D_DEFINE constexpr float PULSE = 1.0f / 60.0f

#define PULSE2D_TICK_WORLD() \
    world->step(PULSE);      \
    auto& renderer = engine->renderer()

#define PULSE2D_TICK_PULSE() engine->tick(*world)

#define PULSE2D_ON_COLLISION() if (!world->arbiters.empty())

#define PULSE2D_DRAW(sprite)                                             \
    do {                                                                 \
        static_assert(std::is_same_v<decltype(sprite), pulse2d::Sprite>, \
            "Parameter must be defined with PULSE2D_SPRITE!");           \
        auto [sx, sy] = pulse2d::Renderer::project_coordinates(          \
            planet.position.x, planet.position.y);                       \
        const pulse2d::Sprite& active = sprite;                          \
        renderer.add_sprite(&active,                                     \
            static_cast<int16_t>(sx - active.width / 2),                 \
            static_cast<int16_t>(sy - active.height / 2));               \
    } while (0)

#define PULSE2D_ADD_BODY(name) world->add(&name)

#define PULSE2D_ADD_SPRITE(to, path, x, y)                   \
    to = engine->storage().load_sprite(path, x, y);          \
    Serial.printf("[DEBUG]: SPRITE %s: data=%p w=%u h=%u\n", \
        path,                                                \
        to.data,                                             \
        to.width,                                            \
        to.height)

#define PULSE2D_INIT(gravity_1, gravity_2, solver)                    \
    do {                                                              \
        engine.emplace();                                             \
        world.emplace(                                                \
            pulse2d::graphics::Vec2{ gravity_1, gravity_2 }, solver); \
        engine->init();                                               \
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

namespace pulse2d {

namespace teensy {

#if defined(PULSE2D_TEENSY)
extern "C" uint32_t _ebss;
extern "C" uint32_t _estack;

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

/**
 * @brief
 * Deferred in-place construction for objects with non-trivial
 * constructors that must not run before the runtime is ready
 * (e.g. Teensy hardware objects constructed before setup()).
 *
 *   Declare as a static global — T's constructor is never invoked at
 *   static-init time. Call emplace() once the runtime is ready to
 *   construct T in-place inside the internal aligned storage.
 *
 *   Example:
 *
 *     static pulse2d::HARDWARE_Deferred_Init<Pulse2d> engine;
 *
 *     void setup() {
 *         engine.emplace();          // default-construct
 *         engine->init();
 *     }
 *
 *     void loop() {
 *         engine->tick(world);
 *     }
 */
template<typename T>
class HARDWARE_Deferred_Init
{
  public:
    constexpr HARDWARE_Deferred_Init() = default;
    HARDWARE_Deferred_Init(HARDWARE_Deferred_Init const&) = delete;
    HARDWARE_Deferred_Init& operator=(HARDWARE_Deferred_Init const&) = delete;

    ~HARDWARE_Deferred_Init()
    {
        if (ptr_)
            ptr_->~T();
    }

    template<typename... Args>
    T& emplace(Args&&... args)
    {
        ptr_ = new (storage_) T(std::forward<Args>(args)...);
        return *ptr_;
    }

    T* get() { return ptr_; }
    T const* get() const { return ptr_; }

    T& operator*() { return *ptr_; }
    T const& operator*() const { return *ptr_; }

    T* operator->() { return ptr_; }
    T const* operator->() const { return ptr_; }

    explicit operator bool() const { return ptr_ != nullptr; }

  private:
    alignas(T) std::byte storage_[sizeof(T)];
    T* ptr_ = nullptr;
};

} // namespace pulse2d