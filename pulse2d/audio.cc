/*****************************************************************************
 * Copyright (c) 2026 Jahan Addison
 *
 * This file is part of pulse2d.
 * This software is released under the MIT License. You may use,
 * distribute, and modify this code under the terms of the license.
 *
 * See the LICENSE file in the project root for the full text.
 ****************************************************************************/

#include <pulse2d/audio.h>
#include <pulse2d/util.h> // for PULSE2D_TEENSY

/****************************************************************************
 * Audio
 *
 * Allocates Teensy audio library memory blocks and enables the SGTL5000
 * codec at 50% volume. Both calls do nothing in SDL2 (host) builds.
 *
 ****************************************************************************/

namespace pulse2d {

/**
 * @brief Allocate audio buffers and start the codec (no-op on host)
 */
void Audio::init()
{
#if defined(PULSE2D_TEENSY)
    AudioMemory(k_audio_memory_blocks);
    codec_.enable();
    codec_.volume(0.5f);
#endif
}

} // namespace pulse2d
