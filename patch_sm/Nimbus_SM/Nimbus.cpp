#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "granular_processor.h"

using namespace daisysp;
using namespace daisy;
using namespace patch_sm;

GranularProcessorClouds processor;
DaisyPatchSM              hw;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128];

Parameters* parameters;


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{

    //float dryWet = hw.GetAdcValue(CV_1);
    //parameters->dry_wet = dryWet;

    FloatFrame input[size];
    FloatFrame output[size];

    for(size_t i = 0; i < size; i++)
    {
        input[i].l  = in[0][i];
        input[i].r  = in[1][i];
        output[i].l = output[i].r = 0.f;
        
    }

    processor.Process(input, output, size);

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = output[i].l;
        out[1][i] = output[i].r;
    }
}

int main(void)
{

    hw.Init();
    hw.SetAudioBlockSize(32); // clouds won't work with blocks bigger than 32
    float sample_rate = hw.AudioSampleRate();
    //init the luts
    InitResources(sample_rate);

    processor.Init(sample_rate,
                   block_mem,
                   sizeof(block_mem),
                   block_ccm,
                   sizeof(block_ccm));

    //processor.set_bypass(false);
    //processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    if (processor.playback_mode() == PLAYBACK_MODE_GRANULAR)
    {
        float led = parameters->dry_wet;
        hw.WriteCvOut(2,led*5);
    }
    else
    {
        hw.WriteCvOut(2,0);
    }
    while(1)
    {
        processor.Prepare();        
    }
}
