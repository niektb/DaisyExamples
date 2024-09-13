#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "granular_processor.h"

using namespace daisysp;
using namespace daisy;
using namespace patch_sm;

GranularProcessorClouds processor;
DaisyPatchSM              hw;
Switch toggle;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128];

Parameters* parameters;

void controls()
{
    hw.ProcessAllControls();
    toggle.Debounce();
    bool shift  = toggle.Pressed();

    if (shift == true)
    {
        hw.WriteCvOut(2,5);
        
        float posKnob = hw.GetAdcValue(CV_1);
        parameters->position = posKnob;

        float sizeKnob = hw.GetAdcValue(CV_2);
        parameters->size = fmap(sizeKnob, 0.1f, 0.99f); //too small values for the size leads to crashes.

        float densityKnob = hw.GetAdcValue(CV_3);
        parameters->density = densityKnob;

        float dryWetKnob = hw.GetAdcValue(CV_4);
        parameters->dry_wet = dryWetKnob;
    }
    else if (shift == false)
    {
        hw.WriteCvOut(2,0);

        float pitchKnob = hw.GetAdcValue(CV_1);
        parameters->pitch = pitchKnob;

        float panKnob = hw.GetAdcValue(CV_2);
        parameters->stereo_spread = panKnob;

        float textureknob = hw.GetAdcValue(CV_3);
        parameters->texture = textureknob;

        float revKnob = hw.GetAdcValue(CV_4);
        parameters->reverb = revKnob;
    }
}


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    controls();

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
    toggle.Init(hw.B8);

    hw.SetAudioBlockSize(32); // clouds won't work with blocks bigger than 32
    float sample_rate = hw.AudioSampleRate();
    //init the luts
    InitResources(sample_rate);

    processor.Init(sample_rate,
                   block_mem,
                   sizeof(block_mem),
                   block_ccm,
                   sizeof(block_ccm));

    parameters = processor.mutable_parameters();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        processor.Prepare();
    }
}
