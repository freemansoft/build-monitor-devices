#include "WProgram.h"
#include "Interface.h"
#include "Rainbow.h"
#include "Demos.h"
#include <Wire.h>

extern Rainbow myRainbow;

extern unsigned char demoIndex;

//for receive color data from serial port
//extern unsigned short receiveBuffer[8][8];//defined in data.h

Interface::Interface(){
}

//initialization all interfaces including serial and I2C
void Interface::init(void)
{
  //Initalize serial state
  serialState = COMMAND_MODE;

  //Initialize serial interface
  Serial.begin(BAUDRATE);
  // I put this in during 3D command testing because perl on windows gave me problems
  Serial.println("initialized");

  //Initiallize I2C interface
#ifdef I2C_MASTER
  Wire.begin();
#else 
  Wire.begin(I2C_ADDR);//slave has an address
#endif;
}

void Interface::initBluetoothBee()
{
    delay(1000);
    Serial.print("\r\n+STWMOD=0\r\n");                // slave
    Serial.print("\r\n+STNA=SeeeduinoBluetooth\r\n"); // device name
    Serial.print("\r\n+STAUTO=0\r\n");                // auto connect to last device 0=no,1=yes
    Serial.print("\r\n+STOAUT=1\r\n");                // permit paired device to connect 0=no,1=yes
    Serial.print("\r\n +STPIN=1234\r\n");             // set pin to 1234
    delay(2000); // This delay is required.
    Serial.print("\r\n+INQ=1\r\n");                   // stop inquiring
    delay(2000); // This delay is required.
}

//pocess serial and I2C interface 
void Interface::process(void)
{
  processSerial();
  processI2C();
}

//process serial interface
void Interface::processSerial(void)
{
  switch (serialState){
  case COMMAND_MODE:
    {
      processCmd();
      break;
    }//when in command mode,receive and reslove it 
  case DATA_MODE:
    {
      processData();
      break;
    }//when in data mode, receive and process it
  default:
    break;
  }
}

//process I2C interface data
//looks like someone disabled I2C for version 1.0.5?
void Interface::processI2C(void)
{
}

//receive and resolve the command
void Interface::processCmd(void)
{
  static unsigned char cmd[7]={
    0,0,0,0,0,0,0    };//'R'+type+shift+red+green+blue+ASCII/index
  unsigned char receiveOK = 0;

  //check the commmand
  receiveOK = checkCmd(cmd);

  //reslove the command
  if(receiveOK){
    resolveCmd(cmd);
  }
}

//receive and process the data
void Interface::processData(void)
{
  //when get 0xAA 0x55,means data transmitting over
  if(Serial.available()){
    unsigned char t1 = Serial.read();
    if(0xAA == t1){
      char t2;
      while(1){
        if(Serial.available()>0){
          break;
        }
      }
      t2 = Serial.read();
      if(0x55 == t2){
        serialState = COMMAND_MODE;
        myRainbow.lightAll();
      }
      else{
        myRainbow.fillColorBuffer(t1);
        myRainbow.fillColorBuffer(t2);
      }
    }
    else{
      myRainbow.fillColorBuffer(t1);
    }
  }
}

unsigned char Interface::checkCmd(unsigned char cmd[7])
{
  static unsigned char i = 1;
  static unsigned char gotFirstFlag = 0;
  unsigned char receiveOK = 0;

  //read command
  if(Serial.available()){
    if(gotFirstFlag){
      cmd[i++] = Serial.read();
      if(7 == i){
        receiveOK = 1;
        gotFirstFlag = 0;
        i = 1;
      }
    }
    else{
      cmd[0] = Serial.read();
      if(cmd[0] == 'R'){
        gotFirstFlag = 1;
      }
    }
  }
  return receiveOK;
}

//resolve all kinds of command
void Interface::resolveCmd(unsigned char cmd[7])
{
  // a little debug block that can be useful
  //Serial.print("\nresolveCmd:");
  //unsigned char asciiCmd=cmd[1]+'0';
  //Serial.println(asciiCmd);
  unsigned char shift = 0;
  unsigned char red = 0;
  unsigned char green = 0;
  unsigned char blue = 0;
  unsigned char ASCII = 0;
  unsigned char index = 0;
  unsigned short color = 0;//0x0bgr;

  demoIndex = 255;//means not run demo
  
  switch (cmd[1]){
  case DISP_PRESET_PIC:
    {
      shift = cmd[2];
      index = cmd[6];
      myRainbow.dispPresetPic(shift,index);
      break;
    }
  case DISP_CHAR:
    {

      shift = cmd[2];
      red = cmd[3];
      green = cmd[4];
      blue = cmd[5];
      ASCII = cmd[6];
      color = blue;//0x0bgr;
      color = (color<<8)|(green<<4)|red;
      myRainbow.dispChar(ASCII,color,shift);
      break;
    }
  case DISP_COLOR:
    {
      red = cmd[3];
      green = cmd[4];
      blue = cmd[5];
      ASCII = cmd[6];
      color = blue;//0x0bgr;
      color = (color<<8)|(green<<4)|red;

      myRainbow.dispColor(color);
      break;
    }
  case SET_DOT:
    {
      unsigned char line = cmd[2];
      unsigned char column = cmd[3];
      red = cmd[4];
      green = cmd[5];
      blue = cmd[6];
      color = blue;//0x0bgr;
      color = (color<<8)|(green<<4)|red;

      myRainbow.lightOneDot(line,column,color,OTHERS_ON);
      break;
    }
  case DISP_RANDOM:
    {//not implement yet
      demoIndex = cmd[2];
      break;
    }
  case DISP_3D:
  {
      unsigned char coords = cmd[2];
      unsigned char z = coords & 0x03;
      unsigned char y = (coords >> 2) & 0x03;
      unsigned char x = (coords >> 4) & 0x03;
      red = cmd[3];
      green = cmd[4];
      blue = cmd[5];
      // cmd[6] should be 0 (reserved for future use)
      color = blue;//0x0bgr;
      color = (color<<8)|(green<<4)|red;
      myRainbow.light3D( x,y, z,  color,OTHERS_ON);
      break;
  }
  case CHANGE_TO_DATA:
    {
      serialState = DATA_MODE;
      break;
    }
  default:
    break;
  }

}
