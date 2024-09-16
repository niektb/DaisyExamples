#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "granular_processor.h"

using namespace daisysp;
using namespace daisy;
using namespace patch_sm;

GranularProcessorClouds processor;
DaisyPatchSM              hw;
Switch toggle, mode_button;
// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[(118784*2)];
uint8_t block_ccm[(65536*2) - 128];

Parameters* parameters;

int pbmode = 0;
float led_mode = 5;
int quality = 0;
float led_quality = 0;

void controls()
{
    hw.ProcessAllControls();
    toggle.Debounce();
    mode_button.Debounce();

    bool shift  = toggle.Pressed();

    bool freezeGate = hw.gate_in_1.State();
    parameters->freeze = freezeGate;

    dsy_gpio_write(&hw.gate_out_1, freezeGate);
    dsy_gpio_write(&hw.gate_out_2, freezeGate);

    if (shift == true)
    {   
        if (mode_button.RisingEdge())
        {
            pbmode = (pbmode + 1) %4;
            processor.set_playback_mode((PlaybackMode)pbmode);
            led_mode = 5 - pbmode;
        }
        hw.WriteCvOut(2,led_mode);

        float posKnob = hw.GetAdcValue(CV_1);
        float posCV = hw.GetAdcValue(CV_5);
        parameters->position = DSY_CLAMP((posKnob + posCV),0,1);

        float sizeKnob = hw.GetAdcValue(CV_2);
        float sizeCV = hw.GetAdcValue(CV_6);
        parameters->size = DSY_CLAMP((sizeKnob + sizeCV),0.01,1); //too small values for the size leads to crashes.

        float densityKnob = hw.GetAdcValue(CV_3);
        float densityCV = hw.GetAdcValue(CV_7);
        parameters->density = DSY_CLAMP((densityKnob + densityCV),0,1);

        float textureknob = hw.GetAdcValue(CV_4);
        float textureCV = hw.GetAdcValue(CV_8);
        parameters->texture = DSY_CLAMP((textureknob + textureCV),0,1);
    }
    else if (shift == false)
    {
        if (mode_button.RisingEdge())
        {
            quality = (quality + 1) %4;
            processor.set_quality((GrainQuality)quality);
            led_quality = 1 + quality;
        }
        hw.WriteCvOut(2,led_quality);

        float pitchKnob = hw.GetAdcValue(CV_1);
        parameters->pitch = pitchKnob;

        float panKnob = hw.GetAdcValue(CV_2);
        parameters->stereo_spread = panKnob;

        float dryWetKnob = hw.GetAdcValue(CV_3);
        parameters->dry_wet = dryWetKnob;

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
    float sample_rate = hw.AudioSampleRate();
    toggle.Init(hw.B8);
    mode_button.Init(hw.B7,
                    sample_rate,
                    Switch::TYPE_MOMENTARY,
                    Switch::POLARITY_INVERTED,
                    Switch::PULL_UP);

    hw.SetAudioBlockSize(32); // clouds won't work with blocks bigger than 32
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
