See the Readme.md at the top of this repo for more information about this project.

Blog articles on the hardware for these devices may be available at http://joe.blog.freemansoft.com/
* https://joe.blog.freemansoft.com/2014/06/combining-arduino-ethernet-and-adafruit.html
This source code located at https://github.com/freemansoft/build-monitor-devices

# Install Dependencies

Install these libraries in the library manager
1. Adafruit Neopixel by Adafruit
1. Adafruit-PCD8544-Nokia-5110-LCD-library

Install these dependencies from GitHub
1. Download this repo https://github.com/sirleech/Webduino and put it in the `~/Documents/Arduino/libraries` or wherver the libraries are on your development environment.

# Troubleshooting
1. The Ethernet IP address of the web server is displayed on the Nokia 5110
1. If you are uing the Nokia 5110 display to show your IP address and it is too light or too dark then adjust the `contrast` parameter in the code
1. The 5110 pins are slightly different than standard because the NeoPixel shield uses pin 6

# Build and Deploy
Build and deploy using the Arduino IDE

