/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
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
 * Teensy audio library memory blocks, SGTL5000 codec, and two-channel
 * playback: one looping music track and one one-shot SFX channel mixed
 * through a 4-channel mixer before I2S output.
 *
 * Audio data must be in the AudioPlayMemory format produced by wav2sketch.
 *
 * Call tick() every game loop frame to restart looping music automatically.
 ****************************************************************************/

namespace pulse2d {

/**
 * @brief
 * Teensy audio shield I2S output via SGTL5000 with looping music and
 * one-shot SFX playback.
 */
class Audio
{
  public:
    Audio() = default;
    Audio(Audio const&) = delete;
    Audio& operator=(Audio const&) = delete;

  public:
    void init();

    /**
     * @brief Start playing a looping music track. Restarts automatically
     * when tick() is called each frame.
     */
    void play_music(const unsigned int* data);

    /**
     * @brief Stop the current music track.
     */
    void stop_music();

    /**
     * @brief Play a one-shot SFX clip on channel 1. Interrupts any clip
     * already playing on this channel.
     */
    void play_sfx(const unsigned int* data);

    /**
     * @brief Play a one-shot SFX clip on channel 2. Independent of channel 1,
     * so both can play simultaneously (e.g. laser fire while an explosion
     * plays).
     */
    void play_sfx2(const unsigned int* data);

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
    AudioPlayMemory music_player_;
    AudioPlayMemory sfx_player_;
    AudioPlayMemory sfx_player2_;
    AudioMixer4 mixer_;
    AudioOutputI2S out_;
    AudioControlSGTL5000 codec_;

    // Members are initialized in declaration order - players and mixer are
    // ready before AudioConnection constructors wire them.
    // Mixer layout: 0=music, 1=sfx, 2=sfx2, 3=reserved
    AudioConnection conn_music_{ music_player_, 0, mixer_, 0 };
    AudioConnection conn_sfx_{ sfx_player_, 0, mixer_, 1 };
    AudioConnection conn_sfx2_{ sfx_player2_, 0, mixer_, 2 };
    AudioConnection conn_left_{ mixer_, 0, out_, 0 };
    AudioConnection conn_right_{ mixer_, 0, out_, 1 };

    const unsigned int* music_data_{ nullptr };
#endif
};

} // namespace pulse2d
