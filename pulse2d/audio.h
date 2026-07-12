/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#pragma once

#include <pulse2d/util.h> // for PULSE2D_TEENSY

/****************************************************************************
 * Audio
 * Teensy audio library memory blocks, SGTL5000 codec, and two-channel
 * playback: one looping music track and two independent one-shot SFX
 * channels mixed through a 4-channel mixer before I2S output.
 *
 * Audio data must be in the AudioPlayMemory format produced by wav2sketch.
 *
 * All AudioStream and AudioConnection objects live at file scope in audio.cc.
 * AudioConnection stores raw C++ references and registers them in the
 * interrupt-driven audio update list on construction. Those objects must be
 * at stable global addresses for the lifetime of the program; placing them
 * inside a class that uses placement-new or any other relocating storage
 * corrupts the linked list and causes an immediate hard fault.
 *
 * Call tick() every game loop frame to restart looping music automatically.
 * Call init() once after Serial.begin() and before any play_* call.
 ****************************************************************************/

namespace pulse2d {

/**
 * @brief
 * Teensy I2S output via SGTL5000 with looping music and two-channel SFX.
 * Audio hardware objects are kept at global scope in audio.cc.
 */
class Audio
{
  public:
    Audio() = default;
    Audio(Audio const&) = delete;
    Audio& operator=(Audio const&) = delete;

  public:
    /**
     * @brief Allocate audio memory and enable the SGTL5000 codec.
     * Must be the first audio call; call it from PULSE_ON_GAMESTART after
     * Serial.begin(). Safe to skip if no audio hardware is attached.
     */
    void init();

    /**
     * @brief Start playing a looping music track. Restarts automatically
     * when tick() is called each frame.
     */
    void play_music(unsigned int const* data);

    /**
     * @brief Stop the current music track.
     */
    void stop_music();

    /**
     * @brief Play a one-shot SFX clip on channel 1. Interrupts any clip
     * already playing on this channel.
     */
    void play_sfx(unsigned int const* data);

    /**
     * @brief Play a one-shot SFX clip on channel 2. Independent of channel 1,
     * so both can play simultaneously (e.g. laser fire while an explosion
     * plays).
     */
    void play_sfx2(unsigned int const* data);

    /**
     * @brief Restart music if it has finished playing. Call once per frame.
     */
    void tick();

    /**
     * @brief Set the master output volume (0.0–1.0).
     */
    void set_volume(float v);

  private:
    static constexpr int k_audio_memory_blocks = 12;
    static constexpr float k_default_volume = 0.5f;

#if defined(PULSE2D_TEENSY)
    const unsigned int* music_data_{ nullptr };
#endif
};

} // namespace pulse2d
