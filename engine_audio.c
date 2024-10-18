#include <stdlib.h>
#include <string.h>


#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include <math.h>
#include "engine_audio.h"

#define AUDIO_PWM_PIN 23
#define AUDIO_CALLBACK_PWM_PIN 24
#define AUDIO_ENABLE_PIN 20

// Pin for PWM audio sample wrap callback (faster than repeating timer, by a lot)
uint audio_callback_pwm_pin_slice;
pwm_config audio_callback_pwm_pin_config;

uint8_t *current_source_data = NULL;

SoundBuffer soundBuffer;

// void ENGINE_FAST_FUNCTION(engine_audio_handle_buffer)(audio_channel_class_obj_t *channel, bool *complete){
//     // When 'buffer_byte_offset = 0' that means the buffer hasn't been filled before, fill it (see that after this function it is immediately incremented)
//     // When 'buffer_byte_offset >= channel->buffer_end' that means the index has run out of data, fill it with more
//     if(channel->buffers_byte_offsets[channel->reading_buffer_index] == 0 || channel->buffers_byte_offsets[channel->reading_buffer_index] >= channel->buffers_ends[channel->reading_buffer_index]){
//         // Reset for the second case above
//         channel->buffers_byte_offsets[channel->reading_buffer_index] = 0;

//         // Using the sound resource base, fill this channel's
//         // buffer with audio data from the source resource
//         // channel->buffer_end = channel->source->fill_buffer(channel->source, channel->buffer, channel->source_byte_offset, CHANNEL_BUFFER_SIZE);

//         if(channel->source == NULL){
//             return;
//         }

//         sound_resource_base_class_obj_t *source = channel->source;

//         current_source_data = source->get_data(channel, CHANNEL_BUFFER_SIZE, &channel->buffers_ends[channel->reading_buffer_index]);

//         // memcpy((uint8_t*)channel->buffer, (uint8_t*)current_source_data, channel->buffer_end);

//         // https://github.com/raspberrypi/pico-examples/blob/eca13acf57916a0bd5961028314006983894fc84/flash/xip_stream/flash_xip_stream.c#L45-L48
//         // while (!(xip_ctrl_hw->stat & XIP_STAT_FIFO_EMPTY))
//         //     (void) xip_ctrl_hw->stream_fifo;
//         // xip_ctrl_hw->stream_addr = (uint32_t)current_source_data;
//         // xip_ctrl_hw->stream_ctr = channel->buffer_end;

//         // Just in case we were too quick, wait while previous DMA might still be active
//         if(dma_channel_is_busy(channel->dma_channel)){
//             ENGINE_WARNING_PRINTF("AudioModule: Waiting on previous DMA transfer to complete, this ideally shouldn't happen");
//             dma_channel_wait_for_finish_blocking(channel->dma_channel);
//         }

//         channel->reading_buffer_index = 1 - channel->reading_buffer_index;
//         channel->filling_buffer_index = 1 - channel->filling_buffer_index;

//         // https://github.com/raspberrypi/pico-examples/blob/master/flash/xip_stream/flash_xip_stream.c#L58-L70
//         dma_channel_configure(
//             channel->dma_channel,                                   // Channel to be configured
//             &channel->dma_config,                                   // The configuration we just created
//             channel->buffers[channel->filling_buffer_index],        // The initial write address
//             current_source_data,                                    // The initial read address
//             channel->buffers_ends[channel->filling_buffer_index],   // Number of transfers; in this case each is 1 byte
//             true                                                    // Start immediately
//         );

//         // Filled amount will always be equal to or less than to
//         // 0 the 'size' passed to 'fill_buffer'. In the case it was
//         // filled with something, increment to the amount filled
//         // further. In the case it is filled with nothing, that means
//         // the last fill made us reach the end of the source data,
//         // figure out if this channel should stop or loop. If loop,
//         // run again right away to fill with more data after resetting
//         // 'source_byte_offset'
//         if(channel->buffers_ends[channel->filling_buffer_index] > 0){
//             channel->source_byte_offset += channel->buffers_ends[channel->filling_buffer_index];
//         }else{
//             // Gets reset no matter what, whether looping or not
//             channel->source_byte_offset = 0;

//             // If not looping, disable/remove the source and stop this
//             // channel from being played, otherwise, fill with start data
//             if(channel->loop == false){
//                 *complete = true;
//             }else{
//                 // Run right away to fill buffer with starting data since looping
//                 engine_audio_handle_buffer(channel, complete);
//             }
//         }

//         // Not the best solution but when these are both 1 that means
//         // no data has been loaded yet, block until data is loaded into
//         // 1 then load the other buffer while not blocking so that it
//         // can be switched to next time for reading
//         if(channel->reading_buffer_index == channel->filling_buffer_index){
//             dma_channel_wait_for_finish_blocking(channel->dma_channel);

//             // Flip only the buffer to fill since we're going to fill it now
//             channel->filling_buffer_index = 1 - channel->filling_buffer_index;

//             // Fill the other buffer right now
//             engine_audio_handle_buffer(channel, complete);
//         }
//     }
// }
uint32_t audioWaveUpdateCounter;
float audioWaveOut;
void setRGB(uint8_t r, uint8_t g, uint8_t b);

// Samples each channel, adds, normalizes, and sets PWM#

void repeating_audio_callback(void){
    uint16_t currentAudioSamplePosition = audioWaveUpdateCounter % ENGINE_AUDIO_BUFFER_SIZE;
    uint8_t currentAudioBank = audioWaveUpdateCounter / ENGINE_AUDIO_BUFFER_SIZE % 2;
    if (soundBuffer.currentAudioBank != currentAudioBank) {
        if (!soundBuffer.bufferReady) {
            setRGB(1, 0, 0);
        }
        else
        {
            setRGB(0, 1, 0);
        }
        soundBuffer.bufferReady = 0;
        soundBuffer.currentAudioBank = currentAudioBank;
    }
    int16_t *bufferBank = currentAudioBank == 0 ? soundBuffer.samplesA : soundBuffer.samplesB;
    int16_t sample = bufferBank[currentAudioSamplePosition] / 32 + 256;
    if (sample < 0)
    {
        sample = 0;
    }
    else
    if (sample > 511)
    {
        sample = 511;
    }
    pwm_set_gpio_level(AUDIO_PWM_PIN, (uint16_t) sample);
    
    audioWaveUpdateCounter++;
    pwm_clear_irq(audio_callback_pwm_pin_slice);
}


void engine_audio_setup_playback(){
    // Setup amplifier but make sure it is disabled while PWM is being setup
    gpio_init(AUDIO_ENABLE_PIN);
    gpio_set_dir(AUDIO_ENABLE_PIN, GPIO_OUT);
    gpio_put(AUDIO_ENABLE_PIN, 0);

    // Setup PWM audio pin, bit-depth, and frequency. Duty cycle
    // is only adjusted PWM parameter as samples are retrievd from
    // channel sources
    uint audio_pwm_pin_slice = pwm_gpio_to_slice_num(AUDIO_PWM_PIN);
    gpio_set_function(AUDIO_PWM_PIN, GPIO_FUNC_PWM);
    pwm_config audio_pwm_pin_config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&audio_pwm_pin_config, 1);
    pwm_config_set_wrap(&audio_pwm_pin_config, 512);   // 125MHz / 1024 = 122kHz
    pwm_init(audio_pwm_pin_slice, &audio_pwm_pin_config, true);

    // Now allow sound to play by enabling the amplifier
    gpio_put(AUDIO_ENABLE_PIN, 1);
}


void engine_audio_adjust_playback_with_freq(uint32_t core_clock_hz){
    pwm_config_set_wrap(&audio_callback_pwm_pin_config, (uint16_t)((float)(core_clock_hz) / 22050.0f) - 1);
}


void Audio_init()
{
    // ENGINE_PRINTF("EngineAudio: Setting up...\n");

    // // Fill channels array with channels. This has to be done
    // // before any callbacks try to access the channels array
    // // (otherwise it would be trying to access all NULLs)
    // // Setup should be run once per lifetime
    // for(uint8_t icx=0; icx<CHANNEL_COUNT; icx++){
    //     audio_channel_class_obj_t *channel = audio_channel_class_new(&audio_channel_class_type, 0, 0, NULL);
    //     channels[icx] = channel;
    // }

    //generate the interrupt at the audio sample rate to set the PWM duty cycle
    audio_callback_pwm_pin_slice = pwm_gpio_to_slice_num(AUDIO_CALLBACK_PWM_PIN);
    pwm_clear_irq(audio_callback_pwm_pin_slice);
    pwm_set_irq_enabled(audio_callback_pwm_pin_slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP_0, repeating_audio_callback);
    irq_set_priority(PWM_IRQ_WRAP_0, 1);
    irq_set_enabled(PWM_IRQ_WRAP_0, true);
    audio_callback_pwm_pin_config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&audio_callback_pwm_pin_config, 1);
    engine_audio_adjust_playback_with_freq(150 * 1000 * 1000);
    pwm_init(audio_callback_pwm_pin_slice, &audio_callback_pwm_pin_config, true);

    engine_audio_setup_playback();
}


// void engine_audio_stop_all(){
//     ENGINE_INFO_PRINTF("EngineAudio: Stopping all channels...");

//     for(uint8_t icx=0; icx<CHANNEL_COUNT; icx++){
//         // Check that each channel is not NULL since reset
//         // can be called before hardware init
//         if(channels[icx] != NULL){
//             audio_channel_stop(channels[icx]);
//         }
//     }
// }


// void engine_audio_play_on_channel(mp_obj_t sound_resource_obj, audio_channel_class_obj_t *channel, mp_obj_t loop_obj){
//     // Before anything, make sure to stop the channel
//     // incase of two `.play(...)` calls in a row
//     audio_channel_stop(channel);

//     // Mark the channel as busy so that the interrupt
//     // doesn't use it
//     channel->busy = true;

//     if(mp_obj_is_type(sound_resource_obj, &wave_sound_resource_class_type)){
//         sound_resource_base_class_obj_t *source = sound_resource_obj;

//         // Very important to set this link! The source needs access to the channel that
//         // is playing it (if one is) so that it can remove itself from the linked channel's
//         // source
//         source->channel = channel;
//     }else if(mp_obj_is_type(sound_resource_obj, &tone_sound_resource_class_type)){
//         tone_sound_resource_class_obj_t *source = sound_resource_obj;

//         // Very important to set this link! The source needs access to the channel that
//         // is playing it (if one is) so that it can remove itself from the linked channel's
//         // source
//         source->channel = channel;
//     }else if(mp_obj_is_type(sound_resource_obj, &rtttl_sound_resource_class_type)){
//         rtttl_sound_resource_class_obj_t *source = sound_resource_obj;

//         // Very important to set this link! The source needs access to the channel that
//         // is playing it (if one is) so that it can remove itself from the linked channel's
//         // source
//         source->channel = channel;
//     }

//     channel->source = sound_resource_obj;
//     channel->loop = mp_obj_get_int(loop_obj);
//     channel->done = false;

//     // Now let the interrupt use it
//     channel->busy = false;
// }


// /*  --- doc ---
//     NAME: stop
//     ID: audio_stop
//     DESC: Stops playing audio on channel at index
//     PARAM: [type=int]       [name=channel_index]  [value=0 ~ 3]
//     RETURN: None
// */
// static mp_obj_t engine_audio_stop(mp_obj_t channel_index_obj){
//     uint8_t channel_index = mp_obj_get_int(channel_index_obj);

//     if(channel_index > 3){
//         mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("AudioModule: ERROR: Tried to stop audio channel using an index that does not exist"));
//     }

//     audio_channel_stop(channels[channel_index]);

//     return mp_const_none;
// }
// MP_DEFINE_CONST_FUN_OBJ_1(engine_audio_stop_obj, engine_audio_stop);


// /*  --- doc ---
//     NAME: play
//     ID: audio_play
//     DESC: Starts playing an audio source on a given channel and looping or not. It is up to the user to change the gains of the returned channels so that the audio does not clip.
//     PARAM: [type=object]    [name=sound_resource] [value={ref_link:WaveSoundResource} or {ref_link:ToneSoundResource}]
//     PARAM: [type=int]       [name=channel_index]  [value=0 ~ 3]
//     PARAM: [type=boolean]   [name=loop]           [value=True or False]
//     RETURN: {ref_link:AudioChannel}
// */
// static mp_obj_t engine_audio_play(mp_obj_t sound_resource_obj, mp_obj_t channel_index_obj, mp_obj_t loop_obj){
//     // Should probably make sure this doesn't
//     // interfere with DMA or interrupt: TODO
//     uint8_t channel_index = mp_obj_get_int(channel_index_obj);
//     audio_channel_class_obj_t *channel = channels[channel_index];

//     engine_audio_play_on_channel(sound_resource_obj, channel, loop_obj);

//     return MP_OBJ_FROM_PTR(channel);
// }
// MP_DEFINE_CONST_FUN_OBJ_3(engine_audio_play_obj, engine_audio_play);


// /*  --- doc ---
//     NAME: set_volume
//     ID: set_volume
//     DESC: Sets the master volume clamped between 0.0 and 1.0. In the future, this will be persistent and stored/restored using a settings file (TODO)
//     PARAM: [type=float] [name=set_volume] [value=any]
//     RETURN: None
// */
// static mp_obj_t engine_audio_set_volume(mp_obj_t new_volume){
//     // Don't clamp so that users can clip their waveforms
//     // which adds more area under the curve and therefore
//     // is louder (sounds worse though)
//     master_volume = mp_obj_get_float(new_volume);
//     return mp_const_none;
// }

// static mp_obj_t engine_audio_get_volume(){
//     return mp_obj_new_float(master_volume);
// }

// static mp_obj_t engine_audio_module_init(){
//     engine_main_raise_if_not_initialized();
//     return mp_const_none;
// }