#include "Rainbow.h"
#include "Interface.h"
#include "Demos.h"
#include <Wire.h>
#include <avr/pgmspace.h>

//0 matrix patterns
//1 character map
//2 cube demo
//255 represents choose no demo
unsigned char demoIndex = 255;//255 represents choose no demo
unsigned char cmdArrived = 0;//is command arrived,1-yes,0-no

//color data format :0x0bgr
unsigned short color[8] = {
  WHITE,RED,GREEN,RANDOM,BLUE,YELLOW,AQUA,VIOLET};

//preinstantiate Rainbow object
Rainbow myRainbow = Rainbow();

//preinstantiate Interface object
Interface myInterface = Interface();

//Join I2C bus as master or slave
#define I2C_MASTER


//=============================================================
void setup()
{
  myInterface.init();
  myInterface.initBluetoothBee(); // comment this out if not using bluetooth bee
  myRainbow.init();
  myRainbow.lightAll();

}

void loop()
{
  //process the interface
  myInterface.process();

  //run the specific demo if we get runDemo command
  runDemo();


}

//= //Timer1 interuption service routine=========================================
ISR(TIMER1_OVF_vect)         
{
  //sweep 8 lines to make led matrix looks stable
  static unsigned char line=0,level=0;

  flash_line(line,level);

  line++;
  if(line>7)
  {
    line=0;
    level++;
    if(level>15)
    {
      level=0;
    }
  }  
 
 //check if there's data availabe from serial port, set a flag, and then process it in the loop
 if(Serial.available()){
   cmdArrived = 1;
 }
}

