/*****************************************************************************
 * Copyright (c) Jahan Addison
 *
 * This software is dual-licensed under the Apache License, Version 2.0 or
 * the GNU General Public License, Version 3.0 or later.
 *
 * You may use this work, in part or in whole, under the terms of either
 * license.
 *
 * See the LICENSE.Apache-v2 and LICENSE.GPL-v3 files in the project root
 * for the full text of these licenses.
 ****************************************************************************/

#pragma once

#include <cstddef> // for std::byte
#include <cstdint> // for uint32_t
#include <utility> // for std::forward

#ifdef __IMXRT1062__
#define LUYA_TEENSY __IMXRT1062__
#endif

#ifdef LUYA_TESTING
#define LUYA_PRIVATE public
#else
#define LUYA_PRIVATE private
#endif

#if defined(LUYA_TEENSY)
/**
 * @brief Place a variable in OCRAM (the secondary 512 KB RAM bank on the
 *  i.MX RT1062)
 */
#define LUYA_EXTMEM __attribute__((section(".dmabuffers"), used))
#else
#define LUYA_EXTMEM
#endif

#if defined(LUYA_TEENSY) && defined(DEBUG)
#define LUYA_DEBUG_SERIAL(...)          \
    do {                                \
        if (Serial) {                   \
            Serial.print("[DEBUG] ");   \
            Serial.printf(__VA_ARGS__); \
            Serial.println();           \
        }                               \
    } while (0)
#define LUYA_POLL_SERIAL_CONNECTION()               \
    do {                                            \
        while (!Serial)                             \
            ;                                       \
        Serial.println("[DEBUG] setup: serial OK"); \
    } while (0)
#else
#define LUYA_DEBUG_SERIAL(...)
#define LUYA_POLL_SERIAL_CONNECTION()
#endif

#if defined(LUYA_TEENSY)
/**
 * @brief
 * On teensy, place the variable in the .bss section (zero-initialised RAM).
 * Note that the constructor is never called, so the variable must be trivially
 * constructible and destructible. Use LUYA_HARDWARE_DEFINE for
 * non-trivial types that depend on hardware that must be constructed at
 * runtime.
 *
 */
#define LUYA_DEFINE static

#define LUYA_HARDWARE_DEFINE(type) \
    LUYA_DEFINE luya::HARDWARE_Deferred_Init<type>
#endif

#define LUYA_DEFINE_ENGINE()                   \
    LUYA_HARDWARE_DEFINE(luya::Engine) engine; \
    LUYA_HARDWARE_DEFINE(luya::physics::World) world;

#define LUYA_INIT(gravity_1, gravity_2, solver)                             \
    do {                                                                    \
        engine.emplace();                                                   \
        world.emplace(luya::physics::Vec2{ gravity_1, gravity_2 }, solver); \
        engine->init();                                                     \
    } while (0)

namespace luya {

namespace teensy {

#if defined(LUYA_TEENSY)
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
 *     static luya::HARDWARE_Deferred_Init<Engine> engine;
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

} // namespace luya