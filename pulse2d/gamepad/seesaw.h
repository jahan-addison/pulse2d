/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <etl/array.h>

/****************************************************************************
 * Seesaw Gamepad
 *
 * I2C driver for the Adafruit Seesaw Gamepad QT. Checks the hardware
 * ID on init, configures the button GPIO pins as pulled-up inputs, and
 * reads the thumbstick over ADC each poll.
 *
 * Example:
 *
 *.  static pulse2d::gamepad::Teensy_I2CDriver driver;
 *   static pulse2d::gamepad::Seesaw_Gamepad pad(driver);
 *
 * void setup() {
 *     Wire.begin();
 *     pad.init();
 * }
 *
 * void loop() {
 *     pad.poll();
 *     auto& s = pad.get_state();
 * }
 *
 ****************************************************************************/

#if defined(PULSE2D_TEENSY)

#include <Wire.h>

namespace pulse2d::gamepad {

/**
 * @brief
 *   HAL Hardware Abstraction Layer
 */
struct I2CDriver
{
    virtual ~I2CDriver() = default;

    virtual bool write(uint8_t device_address,
        uint8_t const* data,
        size_t length) = 0;
    virtual bool read(uint8_t device_address,
        uint8_t* buffer,
        size_t length) = 0;
    // The Seesaw chip physically requires a tiny pause to process commands
    virtual void delay_us(uint32_t microseconds) = 0;
};

/**
 * @brief
 *   I2C driver backed by Teensy's Wire peripheral
 *
 *   Pass a different bus explicitly: Teensy_I2CDriver driver(Wire1);
 */
struct Teensy_I2CDriver : public I2CDriver
{
    explicit Teensy_I2CDriver(TwoWire& wire = Wire)
        : wire_(wire)
    {
        Wire.begin();
        Wire.setClock(400000); // 400kHz Fast Mode
    }

    bool write(uint8_t device_address,
        uint8_t const* data,
        size_t length) override;

    bool read(uint8_t device_address, uint8_t* buffer, size_t length) override;

    void delay_us(uint32_t microseconds) override;

  private:
    TwoWire& wire_;
};

/**
 * @brief
 *   Snapshot of gamepad input captured by the last call to poll().
 */
struct Seesaw_State
{
    float stick_x = 0.0f; // normalized: -1.0f (Left) to 1.0f (Right)
    float stick_y = 0.0f; // normalized: -1.0f (Up) to 1.0f (Down)
    uint32_t buttons = 0; // bitmask of currently pressed buttons
};

/**
 * @brief
 *   Button bitmask constants mapped to the physical Seesaw Gamepad PCB layout.
 */
namespace Seesaw_Buttons {
constexpr uint32_t A = (1ul << 5);
constexpr uint32_t B = (1ul << 1);
constexpr uint32_t X = (1ul << 6);
constexpr uint32_t Y = (1ul << 2);
constexpr uint32_t START = (1ul << 16);
constexpr uint32_t SELECT = (1ul << 0);
constexpr uint32_t MASK = A | B | X | Y | START | SELECT;
}

/**
 * @brief
 *   Adafruit Seesaw Gamepad QT Driver
 *
 *   Controls the polling loop, register I/O, ADC reads, and normalized
 *   state.
 */
class Seesaw_Gamepad
{
  public:
    explicit Seesaw_Gamepad(I2CDriver& i2c_driver)
        : i2c(i2c_driver)
    {
    }

  public:
    [[nodiscard]] bool init();
    [[nodiscard]] inline Seesaw_State const& get_state() const { return state; }

    void poll();

  private:
    bool write_register(uint8_t base,
        uint8_t reg,
        uint8_t const* payload,
        size_t len);

    bool read_register(uint8_t base,
        uint8_t reg,
        uint8_t* out_buffer,
        size_t out_len);

    uint16_t read_adc(uint8_t channel);

  private:
    I2CDriver& i2c;
    Seesaw_State state;

    static constexpr uint8_t I2C_ADDR = 0x50;

    // register map
    static constexpr uint8_t BASE_STATUS = 0x00;
    static constexpr uint8_t STATUS_HW_ID = 0x01;
    static constexpr uint8_t STATUS_SWRST = 0x7F;

    static constexpr uint8_t BASE_GPIO = 0x01;
    static constexpr uint8_t GPIO_DIRCLR_BULK = 0x03;
    static constexpr uint8_t GPIO_BULK = 0x04;
    static constexpr uint8_t GPIO_BULK_SET = 0x05;
    static constexpr uint8_t GPIO_PULLENSET = 0x0B;

    static constexpr uint8_t BASE_ADC = 0x09;
    static constexpr uint8_t ADC_CHANNEL_OFFSET = 0x07;
};

namespace util {

template<typename T>
concept Physics_Body = requires(T body) {
    body.velocity.x;
    body.velocity.y;
    body.force.x;
    body.force.y;
};

struct Normalized_Input
{
    float x;
    float y;
    float magnitude;
};

[[nodiscard]] inline Normalized_Input get_clamped_stick(
    Seesaw_State const& state)
{
    float mag_sq =
        (state.stick_x * state.stick_x) + (state.stick_y * state.stick_y);

    // deadzone check
    if (mag_sq == 0.0f) {
        return { 0.0f, 0.0f, 0.0f };
    }

    // clamp diagonal overflow (the Pythagorean trap)
    if (mag_sq > 1.0f) {
        float mag = std::sqrt(mag_sq);
        return { state.stick_x / mag, state.stick_y / mag, 1.0f };
    }

    // inside the circle, return as-is
    return { state.stick_x, state.stick_y, std::sqrt(mag_sq) };
}

// Profile A: The Arcade Controller (Pokémon, Zelda, Pac-Man)
// Uses Kinematic direct-velocity for instant response and instant stops.
template<Physics_Body T>
inline void apply_arcade_movement(T& body,
    Seesaw_State const& input,
    float max_speed)
{
    auto stick = get_clamped_stick(input);
    body.velocity.x = stick.x * max_speed;
    body.velocity.y = stick.y * max_speed;
}

// Profile A-Inverse: The Inverted Arcade Controller
template<Physics_Body T>
inline void apply_inverted_arcade_movement(T& body,
    Seesaw_State const& input,
    float max_speed)
{
    auto stick = get_clamped_stick(input);
    body.velocity.x = stick.x * -max_speed;
    body.velocity.y = stick.y * -max_speed;
}

// Profile B: The Momentum Controller (Asteroids, Mario)
// Uses Dynamic physics forces for acceleration and sliding.
template<Physics_Body T>
inline void apply_dynamic_thrust(T& body,
    Seesaw_State const& input,
    float acceleration)
{
    auto stick = get_clamped_stick(input);
    body.force.x += stick.x * acceleration;
    body.force.y += stick.y * acceleration;
}

// Profile C: Top-Down Friction (Linear Damping)
// Required for top-down dynamic games so the player doesn't slide forever.
template<Physics_Body T>
inline void apply_linear_drag(T& body, float drag_coefficient)
{
    body.force.x -= body.velocity.x * drag_coefficient;
    body.force.y -= body.velocity.y * drag_coefficient;
}

} // namespace util

} // namespace gamepad

#endif