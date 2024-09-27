#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "granular_processor.h"

using namespace daisysp;
using namespace daisy;
using namespace patch_sm;

//#define DEBUG

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

bool mode_button_held = false;

// These variables keep track of last button press, used to determine a timeout for the VU Meter to take control of the LED.
float toggle_rising_edge_time = 0;
float button_rising_edge_time = 0;

bool waitForTimeout = true;

float posKnob;
float posCV;
float sizeKnob;
float sizeCV;
float densityKnob;
float densityCV;
float pitchKnob;
float pitchCV;
float textureknob;
float dryWetKnob;
float feedbackKnob;
float spreadKnob;
float revKnob;

void defaultParam() {
    // initialize to avoid big jumps when toggling B8 for the first time.
    parameters->density = .5f;
    parameters->feedback = .01f;
    parameters->dry_wet = .5f;
    parameters->texture = .5f;
    parameters->pitch = .5f;
    parameters->size = .5f;
    parameters->reverb = .0f;
    parameters->position = .5f;
    parameters->stereo_spread = 1.f;
}

void updateParam(bool shift){
    if (shift) {   
        posKnob = hw.GetAdcValue(CV_1);
        sizeKnob = hw.GetAdcValue(CV_2);
        densityKnob = hw.GetAdcValue(CV_3);
        pitchKnob = hw.GetAdcValue(CV_4);
    }
    else //if (shift == false)
    {
        textureknob = hw.GetAdcValue(CV_1);
        parameters->texture = DSY_CLAMP(textureknob, 0, 1);

        dryWetKnob = hw.GetAdcValue(CV_2);
        parameters->dry_wet = DSY_CLAMP(dryWetKnob,0,1);

        feedbackKnob = hw.GetAdcValue(CV_3);
        parameters->feedback = DSY_CLAMP(feedbackKnob, 0.01f, 1.f);

        // if we're holding down the button with shift in off position, we'll be adjusting Stereo Spread
        if (mode_button_held) {
            spreadKnob = hw.GetAdcValue(CV_4);
            parameters->stereo_spread = DSY_CLAMP(spreadKnob, 0, 1); // not mapped, preset to a max spread
        } else {
            revKnob = hw.GetAdcValue(CV_4);
            parameters->reverb = DSY_CLAMP(revKnob, 0, 1);
        }
    }

    // always update CV and related Param
    posCV = hw.GetAdcValue(CV_5);
    parameters->position = DSY_CLAMP((posKnob + posCV),0,1);
    sizeCV = hw.GetAdcValue(CV_6);
    parameters->size = DSY_CLAMP(sizeKnob+sizeCV,0.01,1); //too small values for the size leads to crashes.
    densityCV = hw.GetAdcValue(CV_7);
    parameters->density = DSY_CLAMP((densityKnob + densityCV),0,1);
    pitchCV = hw.GetAdcValue(CV_8);
    float val = DSY_CLAMP((pitchKnob+pitchCV), 0, 1);
    parameters->pitch = powf(9.798f * (val - .5f), 2.f);
    parameters->pitch *= val < .5f ? -1.f : 1.f;
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

    bool toggle_rising_edge = toggle.RisingEdge();
    bool toggle_falling_edge = toggle.FallingEdge();

    bool button_rising_edge = mode_button.RisingEdge();
    bool button_falling_edge = mode_button.FallingEdge();

#ifdef DEBUG
    if (toggle_rising_edge){
        hw.PrintLine("toggle RisingEdge.");
    }

    if(toggle_falling_edge) {
        //shift_rising_edge_time = System::GetNow();
        hw.PrintLine("toggle FallingEdge.");
    }

    if (button_rising_edge){
        hw.PrintLine("mode_button RisingEdge.");
    }

    if (button_falling_edge && mode_button_held) {
        hw.PrintLine("mode_button held and released.");
    }

    if (button_falling_edge && !mode_button_held) {
        hw.PrintLine("mode_button FallingEdge.");
    }

    if (mode_button.TimeHeldMs() >= 3000 && !mode_button_held) {
        hw.PrintLine("mode_button held.");
    }

    if ((System::GetNow() - toggle_rising_edge_time > 10000) && (System::GetNow() - button_rising_edge_time > 10000) && waitForTimeout) {
        hw.PrintLine("LED Timeout.");
    }
#endif

    // These variables keep track of last button press (i.e. User Input), used to determine a timeout for the VU Meter to take control of the LED.
    if (toggle_rising_edge) {
        toggle_rising_edge_time = System::GetNow();
    }

    if (button_rising_edge){
        button_rising_edge_time = System::GetNow();
    }

    // determine wether the button is held. Since TimeHeldMs() is immediately reset upon button release, we set a flag to keep track of this. Resetting happens below.
    if (mode_button.TimeHeldMs() >= 3000 && !mode_button_held) {
        mode_button_held = true;
    }

    if (shift) {
        if (button_falling_edge && !mode_button_held) {
            quality = (quality + 1) %4;
            processor.set_quality((GrainQuality)quality);
        }
        led_brightness = IndexToBrightness(quality, 4);
    } else {
        if (button_falling_edge && !mode_button_held) {
            pbmode = (pbmode + 1) % 4;
            processor.set_playback_mode((PlaybackMode)pbmode);
        }
        led_brightness = IndexToBrightness(pbmode, 4);
    }

    updateParam(shift);

    if ((System::GetNow() - toggle_rising_edge_time > 10000) && (System::GetNow() - button_rising_edge_time > 10000) && waitForTimeout) {
        waitForTimeout = false;
    } else if (((System::GetNow() - toggle_rising_edge_time <= 10000) || (System::GetNow() - button_rising_edge_time <= 10000)) && !waitForTimeout) {
        // means user input has happened, so we wait for timeout again
        waitForTimeout = true;
    }

    if (mode_button_held) {
        // Flash the LED
        led_brightness = (System::GetNow() & 255) > 127 ? 1.f : 0.f;
    } else if (!waitForTimeout) {
        // add level indicator here
        led_brightness = 0;
    } else {
    }

    hw.WriteCvOut(CV_OUT_2, led_brightness * 5.f);

    // resetting flags here because otherwise the checks inside the if(shift) body never return true
    if (button_falling_edge && mode_button_held) {
        mode_button_held = false;
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

#ifdef DEBUG
    hw.StartLog(true);
    hw.PrintLine("Nimbus SM started.");
#endif

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

    defaultParam(); // initialize parameters
    updateParam(toggle.Pressed()); // override with panel controls

    toggle_rising_edge_time = System::GetNow();
    button_rising_edge_time = System::GetNow();

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        processor.Prepare();
    }
}
