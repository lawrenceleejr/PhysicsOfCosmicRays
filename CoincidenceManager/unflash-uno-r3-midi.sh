#!/bin/zsh

sudo dfu-programmer atmega16u2 read
sudo dfu-programmer atmega16u2 erase
sudo dfu-programmer atmega16u2 flash /Users/rhillman/Library/Arduino15/packages/arduino/hardware/avr/1.8.6/firmwares/atmegaxxu2/arduino-usbserial/Arduino-usbserial-atmega16u2-Uno-Rev3.hex
sudo dfu-programmer atmega16u2 reset