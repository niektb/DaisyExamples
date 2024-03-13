#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "granular_processor.h"
#include "dev/oled_ssd130x.h"
#include "daisy_seed.h"
#include "vector"
#include <string>

#define NUM_PARAMS 9
#define NUM_PAGES 2
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
using namespace std;

GranularProcessorClouds processor;
DaisyPatchSM hw;
Encoder encoder;
using MyDisplay = OledDisplay<SSD130x4WireSoftSpi128x64Driver>;
MyDisplay display;

// Pre-allocate big blocks in main memory and CCM. No malloc here.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128]; // __attribute__ ((section (".ccmdata")));

char paramNames[NUM_PARAMS][17];
char pbModeNames[4][17];
char qualityNames[4][17];

int mymod(int a, int b)
{
    return (b + (a % b)) % b;
}

class ParamControl
{
  public:
    ParamControl() {}
    ~ParamControl() {}

    void Init(AnalogControl* control, Parameters* params)
    {
        params_    = params;
        control_   = control;
        param_num_ = 0;
        oldval_    = 0.f;
    }

    void incParamNum(int inc)
    {
        param_num_ += inc;
        param_num_ = mymod(param_num_, NUM_PARAMS);
    }

    char* getName(int inc = 0)
    {
        return paramNames[mymod(param_num_ + inc, NUM_PARAMS)];
    }

    bool knobTouched(float newval)
    {
        bool ret = fabsf(newval - oldval_) > .001f;
        oldval_  = newval;
        return ret;
    }

    void Process()
    {
        float val = control_->Process();
        if(!knobTouched(val))
        {
            return;
        }

        switch(param_num_)
        {
            case 0: params_->position = val; break;
            case 1: params_->size = val; break;
            case 2:
                params_->pitch = powf(9.798f * (val - .5f), 2.f);
                params_->pitch *= val < .5f ? -1.f : 1.f;
                break;
            case 3: params_->density = val; break;
            case 4: params_->texture = val; break;
            case 5: params_->dry_wet = val; break;
            case 6: params_->stereo_spread = val; break;
            case 7: params_->feedback = val; break;
            case 8: params_->reverb = val; break;
        }
    }

  private:
    AnalogControl* control_;
    Parameters*    params_;
    int            param_num_;
    float          oldval_;
};

ParamControl paramControls[4];

int  cursorpos;
int  menupage;
bool selected;
bool held;
bool freeze_btn;
int  pbMode;
int  quality;
int  increment;

Parameters* parameters;

void Controls();

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	Controls();

	FloatFrame input[size];
    FloatFrame output[size];

    for(size_t i = 0; i < size; i++)
    {
        input[i].l  = in[0][i] * .5f + in[2][i] * .5f;
        input[i].r  = in[1][i] * .5f + in[3][i] * .5f;
        output[i].l = output[i].r = 0.f;
    }

    processor.Process(input, output, size);

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = out[2][i] = output[i].l;
        out[1][i] = out[3][i] = output[i].r;
    }
}

//set up the param names for the oled code
void InitStrings()
{
    sprintf(paramNames[0], "position");
    sprintf(paramNames[1], "size");
    sprintf(paramNames[2], "pitch");
    sprintf(paramNames[3], "density");
    sprintf(paramNames[4], "texture");
    sprintf(paramNames[5], "dry wet");
    sprintf(paramNames[6], "stereo spread");
    sprintf(paramNames[7], "feedback");
    sprintf(paramNames[8], "reverb");

    sprintf(pbModeNames[0], "Granular");
    sprintf(pbModeNames[1], "Stretch");
    sprintf(pbModeNames[2], "Looping Delay");
    sprintf(pbModeNames[3], "Spectral");

    sprintf(qualityNames[0], "16 bit stereo");
    sprintf(qualityNames[1], "16 bit mono");
    sprintf(qualityNames[2], "8 bit ulaw streo");
    sprintf(qualityNames[3], "8 bit ulaw mono");
}

int main(void)
{
	hw.Init();
	MyDisplay::Config display_config;
    display_config.driver_config.transport_config.pin_config.sclk = DaisyPatchSM::D10;
    display_config.driver_config.transport_config.pin_config.sclk_delay = 0;
    display_config.driver_config.transport_config.pin_config.mosi = DaisyPatchSM::D9;
    display_config.driver_config.transport_config.pin_config.dc = DaisyPatchSM::D1;
    display_config.driver_config.transport_config.pin_config.reset = DaisyPatchSM::D8;
    display.Init(display_config);
	encoder.Init(hw.A8, hw.A9, hw.B7);
	hw.SetAudioBlockSize(32); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	float sample_rate = hw.AudioSampleRate();

// Set the desired refresh rate (e.g., 100 milliseconds)
    const float refreshInterval = 0.1f; // in seconds
    // Initialize last update time
    float lastUpdateTime = System::GetNow();

    //init the luts
    InitResources(sample_rate);

    processor.Init(sample_rate,
                   block_mem,
                   sizeof(block_mem),
                   block_ccm,
                   sizeof(block_ccm));

    parameters = processor.mutable_parameters();
	processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);
	processor.set_quality(0);
    InitStrings();

    for(int i = 0; i < 4; i++)
    {
        paramControls[i].Init(&hw.controls[i], parameters);
    }

    paramControls[1].incParamNum(1);
    paramControls[2].incParamNum(3);
    paramControls[3].incParamNum(5);

    cursorpos  = 0;
    selected   = false;
    held       = false;
    freeze_btn = false;
    menupage   = 0;
    increment  = 0;

	System::Delay(1000);
	//hw.StartLog(false);
    hw.StartAdc();
    hw.StartAudio(AudioCallback);

	while(1) {
		processor.Prepare();

        display.Fill(false);
        display.DrawLine(0, 10, 128, 10, true);

        display.SetCursor(0, cursorpos * 13 + 13);
        display.WriteString(selected ? "x" : "o", Font_7x10, true);

        switch(menupage)
        {
            case 0:
                display.SetCursor(0, 0);
                display.WriteString("Controls", Font_7x10, true);

                //param names
                for(int i = 0; i < 4; i++)
                {
                    display.SetCursor(10, i * 13 + 13);
                    display.WriteString(
                        paramControls[i].getName((i == cursorpos) ? increment
                                                                  : 0),
                        Font_7x10,
                        !(selected && (i == cursorpos)));
                };
                break;
            case 1:
                display.SetCursor(0, 0);
                display.WriteString("Buttons", Font_7x10, true);

                display.SetCursor(10, 13);
                display.WriteString(
                    "freeze", Font_7x10, !parameters->freeze);

                display.SetCursor(10, 1 * 13 + 13);
                display.WriteString(pbModeNames[pbMode], Font_7x10, true);

                display.SetCursor(10, 2 * 13 + 13);
                display.WriteString(qualityNames[quality], Font_7x10, true);
                break;
        }

		float currentTime = System::GetNow();
		float elapsed = currentTime - lastUpdateTime;

		if (elapsed >= refreshInterval) {
        	display.Update();
			lastUpdateTime = currentTime;
		}
		System::Delay(6); // in ms, from the Field Nimbus
	}
}

void Controls()
{
    hw.ProcessAllControls();

    //process knobs
    for(int i = 0; i < 4; i++)
    {
        paramControls[i].Process();
    }

    //long press switch page
    menupage += encoder.TimeHeldMs() > 1000.f && !held;
    menupage %= NUM_PAGES;
    held = encoder.TimeHeldMs() > 1000.f; //only change pages once
    held &= encoder.Pressed();            //reset on release
    selected &= !held;                       //deselect on page change

    selected ^= encoder.RisingEdge();

    //encoder turn
    if(selected)
    {
        if(menupage == 0)
        {
            increment += encoder.Increment();
        }
        else
        {
            switch(cursorpos)
            {
                case 0: freeze_btn ^= abs(encoder.Increment()); break;
                case 1:
                    pbMode += encoder.Increment();
                    pbMode = mymod(pbMode, 4);
                    processor.set_playback_mode((PlaybackMode)pbMode);
                    break;
                case 2:
                    quality += encoder.Increment();
                    quality = mymod(quality, 4);
                    processor.set_quality(quality);
                    break;
            }
        }
    }
    else
    {
        if(increment != 0)
        {
            paramControls[cursorpos].incParamNum(increment);
            increment = 0;
        }
        cursorpos += encoder.Increment();
        cursorpos = mymod(cursorpos, menupage ? 3 : 4);
    }

    // gate ins
    parameters->freeze  = hw.gate_in_1.State() || freeze_btn;
    parameters->trigger = hw.gate_in_2.Trig();
    parameters->gate    = hw.gate_in_2.State();
}