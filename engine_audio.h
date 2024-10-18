#ifndef ENGINE_AUDIO_MODULE
#define ENGINE_AUDIO_MODULE

// #include "engine_audio_channel.h"
#include <inttypes.h>

#define ENGINE_AUDIO_SAMPLE_RATE 22050.0f
#define ENGINE_AUDIO_SAMPLE_DT 1.0f / ENGINE_AUDIO_SAMPLE_RATE
#define ENGINE_AUDIO_BUFFER_SIZE 1024

typedef struct SoundBuffer
{
    uint16_t samplesA[ENGINE_AUDIO_BUFFER_SIZE];
    uint16_t samplesB[ENGINE_AUDIO_BUFFER_SIZE];
    uint8_t currentAudioBank;
    uint8_t bufferReady;
} SoundBuffer;

extern SoundBuffer soundBuffer;

#endif  // ENGINE_AUDIO_MODULE