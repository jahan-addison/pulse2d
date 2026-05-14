/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 *
 * This file is part of pulse2d.
 * This software is released under the MIT License. You may use,
 * distribute, and modify this code under the terms of the license.
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <pulse2d/util.h> // for PULSE2D_TEENSY

#if defined(PULSE2D_TEENSY)
#include <Audio.h> // for AudioMemory, AudioOutputI2S, AudioControlSGTL5000
#include <SPI.h>   // for SPI
#include <Wire.h>  // for Wire
#endif

/****************************************************************************
 * Audio
 *
 * Teensy audio library memory blocks and enables the SGTL5000
 * codec at 50% volume. Both calls do nothing in host builds.
 *
 ****************************************************************************/

namespace pulse2d {

/**
 * @brief
 * Teensy audio shield I2S output via SGTL5000. No-op in SDL2 builds.
 */
class Audio
{
  public:
    Audio() = default;
    Audio(Audio const&) = delete;
    Audio& operator=(Audio const&) = delete;

  public:
    void init();

  private:
#if defined(PULSE2D_TEENSY)
    static constexpr int k_audio_memory_blocks = 8;

    AudioOutputI2S out_;
    AudioControlSGTL5000 codec_;
#endif
};

} // namespace pulse2d
