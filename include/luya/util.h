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
#include <utility> // for std::forward

#ifdef __IMXRT1062__
#define LUYA_TEENSY __IMXRT1062__
#endif

#ifdef LUYA_TESTING
#define LUYA_PRIVATE public
#else
#define LUYA_PRIVATE private
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
#else
#define LUYA_DEBUG_SERIAL(...)
#endif

namespace luya {

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
 *   Usage:
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