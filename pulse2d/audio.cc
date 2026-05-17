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
 * Teensy audio library memory blocks and enables the SGTL5000
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
    codec_.volume(0.5f);
#endif
}

} // namespace pulse2d
