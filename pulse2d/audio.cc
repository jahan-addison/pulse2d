/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <pulse2d/audio.h>
#include <pulse2d/util.h> // for PULSE2D_TEENSY

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
 * @brief Allocate audio buffers and start the codec
 */
void Audio::init()
{
#if defined(PULSE2D_TEENSY)
    AudioMemory(k_audio_memory_blocks);
    codec_.enable();
    codec_.volume(k_default_volume);
#endif
}

void Audio::play_music(const unsigned int* data)
{
#if defined(PULSE2D_TEENSY)
    music_data_ = data;
    music_player_.play(data);
#else
    (void)data;
#endif
}

void Audio::stop_music()
{
#if defined(PULSE2D_TEENSY)
    music_data_ = nullptr;
    music_player_.stop();
#endif
}

void Audio::play_sfx(const unsigned int* data)
{
#if defined(PULSE2D_TEENSY)
    sfx_player_.play(data);
#else
    (void)data;
#endif
}

void Audio::play_sfx2(const unsigned int* data)
{
#if defined(PULSE2D_TEENSY)
    sfx_player2_.play(data);
#else
    (void)data;
#endif
}

void Audio::tick()
{
#if defined(PULSE2D_TEENSY)
    if (music_data_ and not music_player_.isPlaying())
        music_player_.play(music_data_);
#endif
}

void Audio::set_volume(float v)
{
#if defined(PULSE2D_TEENSY)
    codec_.volume(v);
#else
    (void)v;
#endif
}

} // namespace pulse2d
