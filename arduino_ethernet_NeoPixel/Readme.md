# Arduino Ethernet with 8x5 AdaFruit NeoPixel Shield and 5110/PCD8544 LCD
See the Readme.md at the top of this repo for more information about this project.

Blog articles on the hardware for these devices may be available at http://joe.blog.freemansoft.com/
* https://joe.blog.freemansoft.com/2014/06/combining-arduino-ethernet-and-adafruit.html
* https://joe.blog.freemansoft.com/2012/12/web-control-addressable-led-strip-using.html
This source code located at https://github.com/freemansoft/build-monitor-devices
This code was tested with Arduino Ethernet https://docs.arduino.cc/retired/boards/arduino-ethernet-rev3-without-poe

# Dependencies

## Install these libraries in the library manager
1. Adafruit Neopixel by Adafruit
1. Adafruit-PCD8544-Nokia-5110-LCD-library

## Install these dependencies from GitHub
1. Download this repo https://github.com/sirleech/Webduino and put it in the `~/Documents/Arduino/libraries` or wherver the libraries are on your development environment.

## JQuery
Jquery resources are fetched from the jQuery web site.  You can find all the permutations of jQuery on https://releases.jquery.com/

# Usage
Post Form Data.  The form fields are as follows

```
//  API: Form POST can post any number of form elements in a single POST with the following conventions
//    r=<value>  set all RED LEDs to this value
//    g=<value>  set all GREEN LEDs to this value
//    b=<value>  set all BLUDE LEDs to this value
//    r#=<value> set RED LED number '#' to this value
//    g#=<value> set GREEN LED number '#' to this value
//    b#=<value> set BLUE LED number '#' to this value
//    s#=<value> set Nokia 5100 LCD text on line # to this value. Rows start as 0
//    c=<value>  clear Nokia 5100 LCD value is ignored
```

## Python sample
This code extracted from test.py

```
myobj = {"r": "0", "g": "0", "b": "0"}
response = requests.post(url, data=myobj)
```

# Troubleshooting
1. The Ethernet IP address of the web server is displayed on the Nokia 5110
1. If you are using the Nokia 5110 display and you have trouble reading the text then that is often a contrast issue.  Adjust the `contrast` parameter buried in the ino file
1. The Nokia 5110 pin assignments are slightly different than standard because the NeoPixel shield uses pin 6.  This uses pins 3,4,5,7,8 instead of 3,4,5,6,7.
1. Enabling the Serial port may crash the Arduino.  The code is very memory constrained.

# Build and Deploy
Build and deploy using the Arduino IDE

# Board Under Test

1. Arduino Ethernet
1. Proto Shield.  I did a small cutout over and around the ethernet jack so it could be pressed all the way down.
1. Nokia 5110 style LCD display.
1. 8 pin header mounted on the end of the Proto Shield at 90 so the cable easily plug into the shield
1. Double ended 8 pin socket cable to connect the Proto Shield and the LCD
1. AdaFruit Neopixel 8x5 40 pixel shield
1. USB to RX/TX adapter for programming

## Arduino Ethernet
The Arduino Ethernet is a microcontroller board based on the ATmega328. It has 14 digital input/output pins, 6 analog inputs, a 16 MHz crystal oscillator, a RJ45 connection, a power jack, an ICSP header, and a reset button.
Pins 10, 11, 12 and 13 are reserved for interfacing with the Ethernet module and should not be used otherwise. This reduces the number of available pins to 9, with 4 available as PWM outputs.

The ATmega328 has 32 KB (with 0.5 KB used for the bootloader). It also has 2 KB of SRAM and 1 KB of EEPROM (which can be read and written with the EEPROM library).

An onboard microSD card reader, which can be used to store files for serving over the network, is accessible through the SD Library. Pin 10 is reserved for the Wiznet interface, SS for the SD card is on Pin 4.