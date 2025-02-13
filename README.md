# Adafruit Macropad Keyboard Controller 

### This is a WIP, very early stages, with many issues left to be addressed.  
  
A basic script for turning the [Adafruit Macropad](https://www.adafruit.com/product/5128) into a keyboard.

Supports keyboard shortcuts (up to 3 keys currently), so for example Copy (Left Ctrl + c) can be bound to a single key.   
Rather than using CircuitPython, this script is written in C++ as an Arduino sketch and can be uploaded to the Adafruit Macropad using the [Arduino IDE](https://www.arduino.cc/en/software).

## Setup

To be able to run Arduino sketches on the Adafruit Macropad, you have to follow the [setup guide on Adafruit's website](https://learn.adafruit.com/adafruit-macropad-rp2040/arduino-ide-setup).  
This will guide you through how to set up the Philhower Board Manager and Support Package, and how to install the 3 required libraries that offer support for the OLED display, the LEDs and the Rotary Encoder.  
No additional libraries are used other than these default ones.

## Features

A basic menu displayed on the OLED, which can be navigated using the rotary encoder, offers the following features:
1. Changing each key's functionality.
2. Changing each key's LED color, including the option to turn them off.
3. Changing the overall brightness of the LED's.
  
These options allow you to adjust the Adafruit Macropad's settings on the go, without having to edit the sketch and reupload it, making it possible to use it freely on any computer.
