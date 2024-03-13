#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "vector"
#include <string>
//Don't forget the include
#include "dev/oled_ssd130x.h"


using namespace daisy;
using namespace patch_sm;
using namespace daisysp;
using namespace std;



DaisyPatchSM hw;

//Declare the encoder
Encoder encoder;


//Declare the display
using MyDisplay = OledDisplay<SSD130x4WireSoftSpi128x64Driver>;
MyDisplay display;

// For the demo of a menu 
vector <string> menu {"DEL","TYP", "FILT","POS","RST"};


// Little routine to keep  of the encoder position within the size of a vector 
void manage_encoder(int32_t inc,int32_t &x,int m  ){  //provide the encoder increment, load the value for menu in x, the size of menu (vector size)
	x+=inc;
	if (x<0){
		x =m-1;
		}
	else{
		x=x % m;
		};
};


// show the menu 
void display_menu(int menu_pos,vector <string> m ){
	int pixel_sum=0;;
	display.DrawRect(0,0,127,9,false,true);	
	display.DrawRect(0,0,127,9,false,false);
	for (int i =0; i < m.size();i++){
		display.SetCursor(pixel_sum,0);
		if (i== menu_pos){
			display.WriteString(m[i].c_str(),Font_6x8,false);
			}
		else{
			display.WriteString(m[i].c_str(),Font_6x8,true);
		};
		pixel_sum+= (m[i].length()+1)*6;
	};
 }

bool click = false; 

void controls(){

	static int32_t increment=0,main_menu_pos=0 ;
	

	encoder.Debounce();
	increment=encoder.Increment();
	if(encoder.FallingEdge() ){click = !click;};

	manage_encoder(increment,main_menu_pos,menu.size() );
	display_menu(main_menu_pos,menu);
	display.SetCursor(0, 20);
	display.WriteString("Hello ThunderB",Font_7x10,true);
	display.SetCursor(0, 30);
	display.WriteString("Inverted",Font_7x10,false);
	display.SetCursor(0, 40);
	display.WriteString("Big",Font_11x18,click);

}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++)
	{
		OUT_L[i] = IN_L[i];
		OUT_R[i] = IN_R[i];
	}
}

int main(void)
{
	hw.Init();

 	hw.StartLog(false);

	/*check the pins ,	in my case 
	D10 => clock 
	D9 to mosi
	A2  DC
	A3 reset
	*/
	MyDisplay::Config display_config;
    display_config.driver_config.transport_config.pin_config.sclk = DaisyPatchSM::D10;
    display_config.driver_config.transport_config.pin_config.sclk_delay = 0;
    display_config.driver_config.transport_config.pin_config.mosi = DaisyPatchSM::D9;
    display_config.driver_config.transport_config.pin_config.dc = DaisyPatchSM::D1;
    display_config.driver_config.transport_config.pin_config.reset = DaisyPatchSM::D8;
    display.Init(display_config);
	
//hw.StartLog(false);
	encoder.Init(hw.A8, hw.A9, hw.B7); // check the pins  


	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAudio(AudioCallback);
	while(1) {
		controls();

		// Dont forget the display updarte or you see nothing
		display.Update();
	}
}
