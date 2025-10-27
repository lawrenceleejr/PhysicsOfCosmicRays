#!/bin/zsh

sudo dfu-programmer atmega16u2 read
sudo dfu-programmer atmega16u2 erase --force
sudo dfu-programmer atmega16u2 flash /Users/rhillman/Documents/creative/02_dev/arduino/hiduino/src/arduino_midi.hex --debug=51
sudo dfu-programmer atmega16u2 reset