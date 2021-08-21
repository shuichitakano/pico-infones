# pico-infones

This software is a port of InfoNES, a NES emulator, for the Raspberry Pi Pico, and supports video and audio output over HDMI.

## How to use.
The ROM should be placed in some way from 0x10080000, and can be easily transferred using picotool.
```
picotool load foo.nes -t bin -o 0x10080000
```

You can either place the .nes files directly or place a tar file containing multiple .nes files. The maximum file size that can be used is 1.5 MiB for the standard Raspberry Pi Pico.

## Wiring
The code for HDMI output is based on [PicoDVI](https://github.com/Wren6991/PicoDVI), please refer to the circuit of the HDMI part of PicoDVI for wiring.

