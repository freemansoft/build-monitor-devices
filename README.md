This directory contains firmware for various embedded devices, mostly LED lights used for build or status lights. Most of the devices accept LED control commands over the COM port emulated over the USB connection.  All have been tested with bluetooth dongles on the RX/TX pins

 - **rainboduino_v1_0_7** has firmware to support sending lighting commands to a SeeedStudio (Seeed Studio) 4x4x4 LED cube whose master controller is the Rainbowduino Arduino clone.  This firmware was submitted to seed studio but never published.  
 
 - **rgb_pwm_serial** is Arduino firmware that supports several different lighting configurations originally intended to act as build notification lights. This could actually be used for anything.  It supports two RGB lights using Arduino Uno integrated PWM, 4 RGB lights using the JeeNodes port expander and 32 lights using a Sparkfun addressable led strip. Different commands speak to different devices so no software changes are required.
 
 - **Freemometer** is firmware for an Arduino based device built into an Ikea Dekad retro-style alarm clock. The hands are replaced by a servo.  An LED is added to the front and the mechanical ringer is also controlled by the Arduino.  Commands are sent to the Arduino over USB serial port or via bluetooth depending on the hardware. The firmware supports various patterns for the LED and mechanical bell.

 - **ti_launchpad_rgb** Energia created firmware for an MSP430 launchpad with a single RGB LED.  It allows full PWM control and provides the same LED blink patterns that the Freemoemter provides for it's LED and mecahnical bell. It is the simplest device to make and costs around $10 including the TI MSP430 Launchpad in 2012.
 
  - This code also contains the source to TwoMsTimer.h and TwoMsTimer.cpp which can be used general purpose timer callback timer for the MSP430 at 2msec resolution..

 - **ti_launchpad_rgb_2** A second version of ti_launchpad_rgb. Energia created firmware for an MSP430 Launchpad and the Stellaris / Tiva Cortex M3 Launchpad  with a single onboard RGB LED.  It allows full PWM control and provides the same LED blink patterns that the Freemoemter provides for it's LED and mecahnical bell. It is the simplest device to make and costs around $10 including the TI MSP430 Launchpad in 2012.

  - This code also contains the source to AnyMsTimer which can be used as general purpose timer callback timer for both the MSP430 series and the Tiva series processors.  The MSP430 has 2msec resolution and the Stellaris / Tiva 1msec resolutin. This is an upgraded version of TwoMsTimer.

 - **arduino_ethernet_LED** is Arduino firmware that runs on the Arduino Ethernet board and communicates with a 3 wire Addressable LED strip,
   avaialble from Pololu 12/2012.  The arduino registers its address
   with Bonjour and runs a small web server that accepts POST commands
   to individually control the lamps. There is a sample PERL script that
   shows the structure of the POST commands

 - **arduino_ethernet_NeoPixel** is Arduino firmware that runs on the Arduino Ethernet board and communicates with an Adafruit Neopixel panel.  The arduino registers its address
   with Bonjour and runs a small web server that accepts POST commands
   to individually control the lamps. There is a sample PERL script that
   shows the structure of the POST commands

 - **neo_pixel_panel** is an Adafruit NeoPixel Arduino RGB 5x8 shield on top of an Arudino Uno. This Arduino firmware provides color and blink control via ASCII terminal commands.

Writeups on the hardware for these devices may be available at http://joe.blog.freemansoft.com/