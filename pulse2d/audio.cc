/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 * License: MIT
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <pulse2d/audio.h>
#include <pulse2d/util.h>

/****************************************************************************
 * Audio
 * All AudioStream and AudioConnection objects live at file scope so that
 * the addresses registered into the interrupt-driven audio update linked list
 * are stable for the lifetime of the program.
 *
 * AudioConnection stores raw C++ references to its source and destination
 * AudioStream objects and calls activate() during construction, which inserts
 * the connection into the source stream's output list. If the containing
 * object is ever relocated after that point (placement-new, copy, move), the
 * stored pointer becomes dangling and the next audio update ISR fires into
 * garbage, causing an immediate hard fault. Keeping everything at file scope
 * avoids this.
 *
 * Mixer layout: 0 = music, 1 = sfx, 2 = sfx2, 3 = reserved
 ****************************************************************************/

#if defined(PULSE2D_TEENSY)
#include <Audio.h>
#include <Wire.h>

DMAMEM static audio_block_t g_audio_memory[12];

static AudioPlayMemory g_music_player;
static AudioPlayMemory g_sfx_player;
static AudioPlayMemory g_sfx_player2;
static AudioMixer4 g_mixer;
static AudioOutputI2S g_out;
static AudioControlSGTL5000 g_codec;

static AudioConnection g_conn_music(g_music_player, 0, g_mixer, 0);
static AudioConnection g_conn_sfx(g_sfx_player, 0, g_mixer, 1);
static AudioConnection g_conn_sfx2(g_sfx_player2, 0, g_mixer, 2);
static AudioConnection g_conn_left(g_mixer, 0, g_out, 0);
static AudioConnection g_conn_right(g_mixer, 0, g_out, 1);
#endif

namespace pulse2d {

void Audio::init()
{
#if defined(PULSE2D_TEENSY)
    AudioStream::initialize_memory(g_audio_memory, k_audio_memory_blocks);
    g_codec.enable();
    g_codec.volume(k_default_volume);
#endif
}

void Audio::play_music(const unsigned int* data)
{
#if defined(PULSE2D_TEENSY)
    music_data_ = data;
    g_music_player.play(data);
#else
    (void)data;
#endif
}

void Audio::stop_music()
{
#if defined(PULSE2D_TEENSY)
    music_data_ = nullptr;
    g_music_player.stop();
#endif
}

void Audio::play_sfx(const unsigned int* data)
{
#if defined(PULSE2D_TEENSY)
    g_sfx_player.play(data);
#else
    (void)data;
#endif
}

void Audio::play_sfx2(const unsigned int* data)
{
#if defined(PULSE2D_TEENSY)
    g_sfx_player2.play(data);
#else
    (void)data;
#endif
}

void Audio::tick()
{
#if defined(PULSE2D_TEENSY)
    if (music_data_ and not g_music_player.isPlaying())
        g_music_player.play(music_data_);
#endif
}

void Audio::set_volume(float v)
{
#if defined(PULSE2D_TEENSY)
    g_codec.volume(v);
#else
    (void)v;
#endif
}

} // namespace pulse2d
