/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include "seesaw.h"

#if defined(PULSE2D_TEENSY)

#include <cstdint>        // for uint8_t
#include <etl/array.h>    // for array
#include <pulse2d/util.h> // for PULSE2D_DEBUG_SERIAL

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

namespace pulse2d::gamepad {

/**
 * @brief Check the Seesaw hardware ID and configure button GPIO pins as
 * pulled-up inputs.
 */
bool Seesaw_Gamepad::init()
{
    PULSE2D_DEBUG_SERIAL("Seesaw Init Sequence...");

    uint8_t reset_cmd = 0xFF;
    if (!write_register(BASE_STATUS, STATUS_SWRST, &reset_cmd, 1)) {
        PULSE2D_DEBUG_SERIAL(
            "[WARN]: Soft Reset NACK'd (Bus might be hung, continuing...)");
    }

    // safely loop to avoid delayMicroseconds integer overflow
    for (int i = 0; i < 50; ++i) {
        i2c.delay_us(1000);
    }

    // check Hardware ID to verify connection
    etl::array<uint8_t, 1> hw_id = { 0x00 };
    if (!read_register(BASE_STATUS, STATUS_HW_ID, hw_id.data(), hw_id.size())) {
        PULSE2D_DEBUG_SERIAL(
            "[FATAL]: I2C Read Failed on HW_ID! Check SDA/SCL wiring.");
        return false;
    }

    PULSE2D_DEBUG_SERIAL("Read HW_ID: 0x%02X\n", hw_id[0]);

    // note: accept legacy SAMD09 (0x55) AND modern ATtiny81x (0x86, 0x87)
    if (hw_id[0] != 0x55 && hw_id[0] != 0x87 && hw_id[0] != 0x86) {
        PULSE2D_DEBUG_SERIAL(
            "[FATAL]: HW_ID mismatch! Unrecognized Seesaw chip.");
        return false;
    }

    PULSE2D_DEBUG_SERIAL("Chip detected. Configuring GPIO pins...");

    etl::array<uint8_t, 4> pin_mask = { static_cast<uint8_t>(
                                            Seesaw_Buttons::MASK >> 24),
        static_cast<uint8_t>(Seesaw_Buttons::MASK >> 16),
        static_cast<uint8_t>(Seesaw_Buttons::MASK >> 8),
        static_cast<uint8_t>(Seesaw_Buttons::MASK) };

    if (!write_register(
            BASE_GPIO, GPIO_DIRCLR_BULK, pin_mask.data(), pin_mask.size())) {
        PULSE2D_DEBUG_SERIAL("[FATAL]: Failed to set GPIO Direction.");
        return false;
    }
    i2c.delay_us(1000);

    if (!write_register(
            BASE_GPIO, GPIO_PULLENSET, pin_mask.data(), pin_mask.size())) {
        PULSE2D_DEBUG_SERIAL("[FATAL]: Failed to enable Pull Resistors.");
        return false;
    }
    i2c.delay_us(1000);

    if (!write_register(
            BASE_GPIO, GPIO_BULK_SET, pin_mask.data(), pin_mask.size())) {
        PULSE2D_DEBUG_SERIAL("[FATAL]: Failed to set GPIO Pull-UPs.");
        return false;
    }

    PULSE2D_DEBUG_SERIAL("Seesaw init sequence complete!");
    return true;
}

/**
 * @brief Read the current button states and thumbstick ADC values into state
 */
void Seesaw_Gamepad::poll()
{
    // read button states
    etl::array<uint8_t, 4> bulk_gpio = { 0, 0, 0, 0 };
    if (read_register(
            BASE_GPIO, GPIO_BULK, bulk_gpio.data(), bulk_gpio.size())) {
        uint32_t current_pins = (static_cast<uint32_t>(bulk_gpio[0]) << 24) |
                                (static_cast<uint32_t>(bulk_gpio[1]) << 16) |
                                (static_cast<uint32_t>(bulk_gpio[2]) << 8) |
                                (static_cast<uint32_t>(bulk_gpio[3]));

        state.buttons = (~current_pins) & Seesaw_Buttons::MASK;
    }

    // read ADC thumbstick values
    uint16_t raw_x = read_adc(14);
    uint16_t raw_y = read_adc(15);

    // normalize (10-bit ADC: 0 to 1023 -> -1.0f to +1.0f)
    float nx = ((static_cast<float>(raw_x) / 1023.0f) * 2.0f) - 1.0f;
    float ny = ((static_cast<float>(raw_y) / 1023.0f) * 2.0f) - 1.0f;

    // apply a 15% Deadzone to eliminate hardware drift
    constexpr float DEADZONE = 0.15f;

    state.stick_x = (nx > -DEADZONE && nx < DEADZONE) ? 0.0f : nx;
    state.stick_y = (ny > -DEADZONE && ny < DEADZONE) ? 0.0f : ny;
}

/**
 * @brief Write payload bytes to a Seesaw register identified by base and reg
 */
bool Seesaw_Gamepad::write_register(uint8_t base,
    uint8_t reg,
    uint8_t const* payload,
    size_t len)
{
    etl::array<uint8_t, 16> buffer;
    if (len + 2 > buffer.max_size())
        return false;

    buffer[0] = base;
    buffer[1] = reg;
    for (size_t i = 0; i < len; ++i) {
        buffer[2 + i] = payload[i];
    }

    return i2c.write(I2C_ADDR, buffer.data(), len + 2);
}

/**
 * @brief Point the Seesaw chip to a register and read out_len bytes into
 * out_buffer.
 */
bool Seesaw_Gamepad::read_register(uint8_t base,
    uint8_t reg,
    uint8_t* out_buffer,
    size_t out_len)
{
    etl::array<uint8_t, 2> prefix = { base, reg };

    // point the Seesaw chip to the register we want
    if (!i2c.write(I2C_ADDR, prefix.data(), prefix.size()))
        return false;

    // seesaw delay (250 microseconds is standard)
    i2c.delay_us(1000);

    return i2c.read(I2C_ADDR, out_buffer, out_len);
}

/**
 * @brief Request a 10-bit ADC sample from the given Seesaw channel; returns 511
 * on failure.
 */
uint16_t Seesaw_Gamepad::read_adc(uint8_t channel)
{
    etl::array<uint8_t, 2> prefix = { BASE_ADC,
        static_cast<uint8_t>(ADC_CHANNEL_OFFSET + channel) };

    if (!i2c.write(I2C_ADDR, prefix.data(), prefix.size()))
        return 511;    // return center on fail
    i2c.delay_us(500); // ADC needs a slightly longer sample delay

    etl::array<uint8_t, 2> val;
    if (!i2c.read(I2C_ADDR, val.data(), val.size()))
        return 511;

    // Seesaw sends MSB first
    return (static_cast<uint16_t>(val[0]) << 8) | val[1];
}

/**
 * @brief Transmit length bytes from data to device_address over Wire.
 */
bool Teensy_I2CDriver::write(uint8_t device_address,
    uint8_t const* data,
    size_t length)
{
    wire_.beginTransmission(device_address);

    // Explicitly push every byte to bypass Print class truncation traps
    for (size_t i = 0; i < length; ++i) {
        wire_.write(data[i]);
    }

    return wire_.endTransmission() == 0;
}

/**
 * @brief Request length bytes from device_address over Wire into buffer.
 */
bool Teensy_I2CDriver::read(uint8_t device_address,
    uint8_t* buffer,
    size_t length)
{
    uint8_t received = wire_.requestFrom(device_address,
        static_cast<uint8_t>(length),
        static_cast<uint8_t>(true));

    if (received != static_cast<uint8_t>(length)) {
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        buffer[i] = static_cast<uint8_t>(wire_.read());
    }

    return true;
}

/**
 * @brief Block for the requested number of microseconds using Teensy's
 * delayMicroseconds.
 */
void Teensy_I2CDriver::delay_us(uint32_t microseconds)
{
    delayMicroseconds(microseconds);
}

} // gamepad

#endif