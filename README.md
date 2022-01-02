# pico-infones

This software is a port of InfoNES, a NES emulator, for the Raspberry Pi Pico, and supports video and audio output over HDMI.
The code for HDMI output is based on [PicoDVI](https://github.com/Wren6991/PicoDVI).

## Wiring
The default pinout of the HDMI connector follows the [Pico-DVI-Sock](https://github.com/Wren6991/Pico-DVI-Sock).

The controller should be connected to the micro USB port of the Raspberry Pi Pico via an OTG adapter.

Supply +5V to VBUS (pin 40) as power source. Be careful not to connect the power supply at the same time as the PC connection, for example when writing programs or ROMs.

## ROM
The ROM should be placed in some way from 0x10080000, and can be easily transferred using [picotool](https://github.com/raspberrypi/picotool).
```
picotool load foo.nes -t bin -o 0x10080000
```

You can either place the .nes files directly or place a tar file containing multiple .nes files. The maximum file size that can be used is 1.5 MiB for the standard Raspberry Pi Pico.

## Controller
The following controllers are supported.

- BUFFALO BGC-FC801
- SONY DUALSHOCK 4
- SONY DualSense

There are several special functions assigned to button combinations.

| Buttton               | Function               |
| --                    | --                     |
| SELECT + START        | Reset the emulator     |
| SELECT + LEFT / RIGHT | Select the next ROM    |
| SELECT + UP / DOWN    | Switch the screen mode |
| SELECT + A / B        | Toggle rapid-fire      |

## Battery backed SRAM
If there is a game with battery-backed memory, 8K bytes per title will be allocated from address 0x10080000 in the reverse direction.
Writing to Flash ROM is done at the timing when reset or ROM selection is made.


