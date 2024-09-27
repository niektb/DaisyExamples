# Nimbus

## Author

Ported by Niek ten Brinke & Ben Sergentanis
Originally by Emilie Gillet

## Description

Nimbus is a port of Mutable Instrument's Clouds. Clouds is a granular  
audio processor, specializing in making huge clouds of sound from even the tiniest source.  

Ported from [pichenettes/eurorack](https://github.com/pichenettes/eurorack)

## TODO list for the Patch SM Port:
- Implement soft takeover when toggling the switch
- Check if V/Oct CV input tracks properly
- Idea: Implement VU meter which is used to indicate input signal when the button has not been pressed for a while
- Idea: Think of a function for CV1 Out (maybe Envelope Follower for Input Signal / VU meter)
- Idea: use led blinking to indicate mode/quality for a better insight about selection (best implemented in conjunction with VU meter because a permanent quick blinking led is annoying)
- Idea: PersistantStorage for Quality, Mode and the knobs that are not currently selected (by the toggle).

## Controls

<table><thead>
  <tr>
    <th>Control</th>
    <th colspan="2">Description</th>
    <th>Comment</th>
  </tr></thead>
<tbody>
  <tr>
    <td>Toggle Switch</td>
    <td>Switch Up</td>
    <td>Switch Down</td>
    <td>Toggling changes function of pots and button</td>
  </tr>
  <tr>
    <td>CV_1</td>
    <td>Position</td>
    <td>Texture</td>
    <td></td>
  </tr>
  <tr>
    <td>CV_2</td>
    <td>Size</td>
    <td>Dry/Wet</td>
    <td></td>
  </tr>
  <tr>
    <td>CV_3</td>
    <td>Density</td>
    <td>Feedback</td>
    <td></td>
  </tr>
  <tr>
    <td>CV_4</td>
    <td>Pitch</td>
    <td>Reverb</td>
    <td>Hold button for >3sec to adjust stereo spread (while keeping button pressed)</td>
  </tr>
  <tr>
    <td>CV_5</td>
    <td colspan="2">Position CV Input</td>
    <td></td>
  </tr>
  <tr>
    <td>CV_6</td>
    <td colspan="2">Size CV Input</td>
    <td></td>
  </tr>
  <tr>
    <td>CV_7</td>
    <td colspan="2">Density CV Input</td>
    <td></td>
  </tr>
  <tr>
    <td>CV_8</td>
    <td colspan="2">Pitch CV Input (V/Oct)</td>
    <td></td>
  </tr>
  <tr>
    <td>LED</td>
    <td>Show selected quality</td>
    <td>Show selected mode</td>
    <td>Indicated with brightness</td>
  </tr>
  <tr>
    <td>Button</td>
    <td>Cycle through qualities</td>
    <td>Cycle through modes</td>
    <td></td>
  </tr>
  <tr>
    <td>Gate In 1</td>
    <td colspan="2">Freeze Gate In</td>
    <td>Activates Freeze</td>
  </tr>
  <tr>
    <td>Gate In 2</td>
    <td colspan="2">Trigger In</td>
    <td>Triggers a Seed</td>
  </tr>
  <tr>
    <td>Gate Out x</td>
    <td colspan="2">Gate In x Through</td>
    <td>Both Gate Inputs are fed to outputs</td>
  </tr>
  <tr>
    <td>Audio In / Out 1 &amp; 2</td>
    <td colspan="2">1 = Left, 2 = Right</td>
    <td>Stereo in, stereo out</td>
  </tr>
</tbody></table>

Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) for more information on these controls.

#### Mode
You can select from Nimbus' four alternate modes here. These are:
- Granular
- Pitch Shift / Time Stretch
- Looping Delay
- Spectral Processor
  
Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) section on "The Infamous Alternate Modes".  

#### Quality
You can also select from four quality / mono stereo modes. These are:
- 16 bit Stereo
- 16 bit Mono
- 8bit u-law Stereo
- 8bit u-law Mono
  
Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) section on "Audio Quality".  
