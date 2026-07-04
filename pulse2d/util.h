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
#ifndef PULSE2D_TEENSY
#define PULSE2D_TEENSY __IMXRT1062__
#endif
#endif

#ifdef PULSE2D_TESTING
#define PULSE2D_PRIVATE public
#else
#define PULSE2D_PRIVATE private
#endif

#define PULSE2D_INLINE inline __attribute__((always_inline))

#if defined(PULSE2D_TEENSY)

/**
 * @brief Place a variable in OCRAM (the secondary 512 KB RAM bank on the
 *  i.MX RT1062)
 */
#define PULSE2D_EXTMEM __attribute__((section(".dmabuffers"), used))
/**
 * @brief Keep a constant in FLASH (never copied to DTCM at boot).
 *
 * The Teensy 4.x linker script merges .rodata into the .data output
 * section which is bulk-copied from FLASH to DTCM at startup. Large
 * read-only tables (palettes, sprite sheets, background images) would
 * exhaust DTCM immediately. Placing them in .progmem keeps them in
 * the 8 MB QSPI flash where they are accessed via the hardware cache.
 *
 * Usage:
 *   PULSE2D_FLASHMEM static constexpr uint16_t my_image[W * H] = { ... };
 */
#define PULSE2D_FLASHMEM __attribute__((section(".progmem")))
#else
#define PULSE2D_EXTMEM
#define PULSE2D_FLASHMEM
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
#else
#define PULSE2D_DEBUG_SERIAL(...)
#endif

// Always-on error print - active in all builds, not gated on DEBUG.
// Use for conditions that are always bugs (null sprites, dimension mismatches).
#if defined(PULSE2D_TEENSY)
#define PULSE2D_ERROR_SERIAL(...)       \
    do {                                \
        if (Serial) {                   \
            Serial.print("[ERROR] ");   \
            Serial.printf(__VA_ARGS__); \
            Serial.println();           \
            Serial.flush();             \
        }                               \
    } while (0)
#else
#include <cstdio>
#define PULSE2D_ERROR_SERIAL(...)                     \
    do {                                              \
        std::fprintf(stderr, "[ERROR] " __VA_ARGS__); \
        std::fprintf(stderr, "\n");                   \
    } while (0)
#endif

namespace pulse2d {

namespace util {

// The overload pattern
template<class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

} // namespace util

/**
 * @brief
 * Static placement-new in-place construction for objects that require the
 * Arduino runtime. Such as access to various hardware states, Such as the TFT
 * display or sd storage.
 *
 * Declare as a static global, then call emplace() once the runtime is ready
 * to construct T in-place inside the internal aligned storage.
 *
 *   Example:
 *
 *     static pulse2d::Static_Inplace_T<Pulse2d> engine;
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
class Static_Inplace_T
{
  public:
    constexpr Static_Inplace_T() = default;
    Static_Inplace_T(Static_Inplace_T const&) = delete;
    Static_Inplace_T& operator=(Static_Inplace_T const&) = delete;

    ~Static_Inplace_T()
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