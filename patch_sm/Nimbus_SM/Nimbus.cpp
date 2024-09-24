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
int quality = 0;
float led_brightness = 0;

float IndexToBrightness(int index, int total)
{
    return static_cast<float>(index + 1) / static_cast<float>(total);
}

void controls()
{
    hw.ProcessAllControls();
    toggle.Debounce();
    mode_button.Debounce();

    bool shift  = toggle.Pressed();

    bool freezeGate = hw.gate_in_1.State();
    parameters->freeze = freezeGate;

    parameters->trigger = hw.gate_in_2.Trig();
    bool Gate = hw.gate_in_2.State();
    parameters->gate    = hw.gate_in_2.State();

    dsy_gpio_write(&hw.gate_out_1, freezeGate);
    dsy_gpio_write(&hw.gate_out_2, Gate);

    if (shift == true)
    {   
        if (mode_button.RisingEdge())
        {
            quality = (quality + 1) %4;
            processor.set_quality((GrainQuality)quality);
            led_brightness = IndexToBrightness(quality, 4);
        }

        float posKnob = hw.GetAdcValue(CV_1);
        float posCV = hw.GetAdcValue(CV_5);
        parameters->position = DSY_CLAMP((posKnob + posCV),0,1);

        float sizeKnob = hw.GetAdcValue(CV_2);
        parameters->size = DSY_CLAMP(sizeKnob,0.01,1); //too small values for the size leads to crashes.

        float densityKnob = hw.GetAdcValue(CV_3);
        float densityCV = hw.GetAdcValue(CV_7);
        parameters->density = DSY_CLAMP((densityKnob + densityCV),0,1);

        float pitchKnob = hw.GetAdcValue(CV_4);
        float pitchCV = hw.GetAdcValue(CV_8);
        float val = DSY_CLAMP((pitchKnob+pitchCV), 0, 1);
        parameters->pitch = powf(9.798f * (val - .5f), 2.f);
        parameters->pitch *= val < .5f ? -1.f : 1.f;
    }
    else if (shift == false)
    {
        if (mode_button.RisingEdge())
        {
            pbmode = (pbmode + 1) % 4;
            processor.set_playback_mode((PlaybackMode)pbmode);
            led_brightness = IndexToBrightness(pbmode, 4);
        }

        float textureknob = hw.GetAdcValue(CV_1);
        parameters->texture = DSY_CLAMP(textureknob, 0, 1);

        float dryWetKnob = hw.GetAdcValue(CV_2);
        float dryWetCV = hw.GetAdcValue(CV_6);
        parameters->dry_wet = DSY_CLAMP((dryWetKnob + dryWetCV),0,1);

        float feedbackKnob = hw.GetAdcValue(CV_3);
        parameters->feedback = DSY_CLAMP(feedbackKnob, 0.01f, 1.f);

        float revKnob = hw.GetAdcValue(CV_4);
        parameters->reverb = DSY_CLAMP(revKnob, 0, 1);

        parameters->stereo_spread = 1.0f; // not mapped, preset to a max spread
    }
    hw.WriteCvOut(CV_OUT_2, led_brightness * 5.f);
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

    // initialize to avoid big jumps when toggling B8 for the first time.
    parameters->density = .5f;
    parameters->feedback = .01f;
    parameters->dry_wet = .5f;
    parameters->texture = .5f;
    parameters->pitch = .5f;
    parameters->size = .5f;
    parameters->reverb = .0f;
    parameters->position = .5f;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        processor.Prepare();
    }
}
