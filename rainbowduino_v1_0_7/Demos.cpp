#include "Demos.h"
#include "Rainbow.h"
#include "WProgram.h"

extern Rainbow myRainbow;
extern unsigned short color[8];
extern unsigned char cmdArrived;
extern unsigned char demoIndex ;//255 represents choose no demo

//run the specific demo 
void runDemo()
{
    switch (demoIndex){
  case 0:
    {
      flashMatrixDemo();//flash led matrix in several patterns, 
      break;
    }
  case 1:
    {
      lightLedStripNumberDemo();//light 12v led strip in 0~9 number way
      break;
    }
  case 2:
    {
      lightCubeDemo(); // flash the 3D cube in several patterns
      break;
    }
  default: 
    break;
  }
}


/****************************************************************/

 //some demos to test rainbow member funtions, you can add more

/****************************************************************/

void flashMatrixDemo(void)
{
 int i = 0;
 int j = 0;

 //all with the same color
 for(i = 0; i < 8; i++)
 {
   checkReturn(cmdArrived);
   delay(1000);
   myRainbow.lightAll(color[i]);
 }

 //sweep one line while others off
 for(i = 0; i < 8; i++)
 {
   checkReturn(cmdArrived);
   delay(100);
   //myRainbow.lightOneLine(i,color[i],OTHERS_OFF);
   myRainbow.lightOneLine(i,myRainbow.receiveBuffer[i],OTHERS_OFF);
 }

 //sweep one line while others on
 for(i = 7; i >= 0; i--)
 {
   checkReturn(cmdArrived);
   delay(100);
   //myRainbow.lightOneLine(i,color[i],OTHERS_ON);
   myRainbow.lightOneLine(i,myRainbow.receiveBuffer[i],OTHERS_ON);
 }
 
 //sweep one column while others off
 for(i = 0; i < 8; i++)
 {
   checkReturn(cmdArrived);
   delay(100);
   //myRainbow.lightOneColumn(i,color[i],OTHERS_OFF);
   myRainbow.lightOneColumn(i,myRainbow.receiveBuffer,OTHERS_OFF);
 }
 
 //sweep one colum while others on
 for(i = 7; i >= 0; i--)
 {
   checkReturn(cmdArrived);
   delay(100);
   //myRainbow.lightOneColumn(i,color[i],OTHERS_ON);
   myRainbow.lightOneColumn(i,myRainbow.receiveBuffer,OTHERS_ON);
 }
 
 //sweep each dot while others off
 for(i = 0; i < 8; i++)
   for(j = 0;j < 8; j++)
   {
     checkReturn(cmdArrived);
     delay(50);
     //myRainbow.lightOneDot(i,j,color[j],OTHERS_OFF);
     myRainbow.lightOneDot(i,j,myRainbow.receiveBuffer[i][j],OTHERS_OFF);
   }
 
 //sweep each dot while others on
 for(i = 7; i >= 0; i--)
   for(j = 7;j >= 0; j--)
   {
     checkReturn(cmdArrived);
     delay(50);
     //myRainbow.lightOneDot(i,j,color[j],OTHERS_ON);
     myRainbow.lightOneDot(i,j,myRainbow.receiveBuffer[i][j],OTHERS_ON);
   }
 
  //sweep the diagonal line from top left to riht bottom while others off
  for(i = 0; i < 16; i++)
  {
    checkReturn(cmdArrived);
    delay(100);
    //myRainbow.lightOneDiagonal(i,LEFT_BOTTOM_TO_RIGHT_TOP,color[i&0x07],OTHERS_OFF);
    myRainbow.lightOneDiagonal(i,LEFT_BOTTOM_TO_RIGHT_TOP,myRainbow.receiveBuffer,OTHERS_OFF);
  }
  
  //sweep the diagonal line from riht bottom to top left while others on
  for(i = 15; i >= 0; i--)
  {
    checkReturn(cmdArrived);
    delay(100);
    //myRainbow.lightOneDiagonal(i,LEFT_BOTTOM_TO_RIGHT_TOP,color[i&0x07],OTHERS_ON);
    myRainbow.lightOneDiagonal(i,LEFT_BOTTOM_TO_RIGHT_TOP,myRainbow.receiveBuffer,OTHERS_ON);
  }

  //sweep the diagonal line from bottom left to riht top while others off
  for(i = 0; i < 16; i++)
  {
    checkReturn(cmdArrived);
    delay(100);
    //myRainbow.lightOneDiagonal(i,LEFT_TOP_TO_RIGHT_BOTTOM,color[i&0x07],OTHERS_OFF);
    myRainbow.lightOneDiagonal(i,LEFT_TOP_TO_RIGHT_BOTTOM,myRainbow.receiveBuffer,OTHERS_OFF);
  }

  //sweep the diagonal line from riht top to bottom left while others on
  for(i = 15; i >= 0; i--)
  {
    checkReturn(cmdArrived);
    delay(100);
    //myRainbow.lightOneDiagonal(i,LEFT_TOP_TO_RIGHT_BOTTOM,color[i&0x07],OTHERS_ON);
    myRainbow.lightOneDiagonal(i,LEFT_TOP_TO_RIGHT_BOTTOM,myRainbow.receiveBuffer,OTHERS_ON);
  }
  
  //sweep 1/4 part of matrix
  checkReturn(cmdArrived);
  delay(200);
  static int k = 0;
  
  for(int m = 0; m < 4; m++)
  {
    for(i = 0; i < 4; i++)
    {
      for(j = 0; j < (i+1)*2; j++)
      {
        myRainbow.lightOneDot(3-i,3-i+j,color[k],OTHERS_ON);
      }
    }
    if(8 == ++k) k = 0;
    
    checkReturn(cmdArrived);
    delay(500);
    for(i = 0; i < 4; i++)
    {
      for(j = 0; j < (i+1)*2; j++)
      {
        myRainbow.lightOneDot(3-i+j,4+i,color[k],OTHERS_ON);
      }
    }
    if(8 == ++k) k = 0;
    
    checkReturn(cmdArrived);
    delay(500);
    for(i = 0; i < 4; i++)
    {
      for(j = 0; j < (i+1)*2; j++)
      {
        myRainbow.lightOneDot(4+i,3-i+j,color[k],OTHERS_ON);
      }
    }
    if(8 == ++k) k = 0;
    
    checkReturn(cmdArrived);
    delay(500);
    for(i = 0; i < 4; i++)
    {
      for(j = 0; j < (i+1)*2; j++)
      {
        myRainbow.lightOneDot(3-i+j,3-i,color[k],OTHERS_ON);
      }
    }
    if(8 == ++k) k = 0;
  }

}

//light 12v led strip in 0~9 number way
void lightLedStripNumberDemo(void)
{
      
    static int colorNum = 0;
    for(int i = 0; i < 10; i++)
    {
      checkReturn(cmdArrived);
      delay(1000);
      //display 0~9 with specific color
      lightLedStripNumber(i,color[colorNum]);
    }
    if(++colorNum == 8) colorNum = 0;
}

/***********************************************************
function: light 12v led strip in the way of number 0~9 
(http://www.seeedstudio.com/depot/3w-rgb-led-strip-common-anode-12v-p-351.html?cPath=32_21)
hardware connection way:
7 picese of led trips, in the following pattern:

          ---   ->strip0     
strip5<- |   |  ->strip1
          ---   ->strip6
strip4<- |   |  ->strip2
          ---   ->strip3
          
1. connect +12v of all 7strips to VCC1 of rainbowduino
2. connect RGB of strip0 to R8 G8 B8 of rainbowduino.(note: it is in inverse way)
3. connect RGB of strip1 to R7 G7 B7 of rainbowduino.
4. ...
...
8 connect RGB of strip6,to R1 G1 B1 of rainbowduino.

In this way, 7 strips is just like 7 dots of the firt line of led matrix. So you can 
control them like the following.

PS: you can add another 7 led strips and connect +12v to VCC2, just like the second line of led matrix,
in which way you can control two numbers. Add more group, getting more numbers, and max is 8 of cource.
************************************************/

void lightLedStripNumber(int num,unsigned short color)
{
  //light the first line leds
  myRainbow.lightOneLine(0,color,OTHERS_OFF);
  
  //close the specific strip to get the number effect
  switch (num)
  {
    case 0: myRainbow.closeOneDot(0,6);break;
    case 1: myRainbow.closeOneLine(0);myRainbow.lightOneDot(0,1,color,OTHERS_ON);myRainbow.lightOneDot(0,2,color,OTHERS_ON);break;
    case 2: myRainbow.closeOneDot(0,2);myRainbow.closeOneDot(0,5);break;
    case 3: myRainbow.closeOneDot(0,4);myRainbow.closeOneDot(0,5);break;
    case 4: myRainbow.closeOneDot(0,0);myRainbow.closeOneDot(0,3);myRainbow.closeOneDot(0,4);break;
    case 5: myRainbow.closeOneDot(0,1);myRainbow.closeOneDot(0,4);break;
    case 6: myRainbow.closeOneDot(0,1);break;
    case 7: myRainbow.closeOneDot(0,3);myRainbow.closeOneDot(0,4);myRainbow.closeOneDot(0,5);myRainbow.closeOneDot(0,6);break;
    case 8: break;
    case 9: myRainbow.closeOneDot(0,4);break;
    default:break;
  }
}


/**********************************************************
  demo the 3D API for the 4x4x4 LED cube
 **********************************************************/
void lightCubeDemo(void)
{
  myRainbow.closeAll();
  
  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 4; j++){
      for (int k = 0; k < 4; k++){
        checkReturn(cmdArrived);
        delay(10);
        myRainbow.lightOneDot(i,j,k,WHITE,OTHERS_OFF);
      }
    }
  }
  delay(50);
  myRainbow.closeAll();
  
  delay(1000);
  
  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 4; j++){
      for (int k = 0; k < 4; k++){
        checkReturn(cmdArrived);
        delay(30);
        myRainbow.lightOneDot(i,j,k,GREEN,OTHERS_ON);
      }
    }
  }
  
  delay(1000);
  
   for (int i = 3; i >= 0; i--){
    for (int j = 3; j >= 0; j--){
      for (int k = 3; k >= 0; k--){
        checkReturn(cmdArrived);
        delay(80);
        myRainbow.lightOneDot(i,j,k,RED,OTHERS_ON);
      }
    }
  }
  
  delay(2000);
  
  // do some plane walking
  // first walk across the X axis
  for(int i = 0 ; i <=3; i++){
    myRainbow.light3D(i,0xff,0xff,BLUE,OTHERS_OFF);
    delay(500);
  }
  // now walk up the y axis
  for(int i = 0 ; i <=3; i++){
    myRainbow.light3D(0xff,i,0xff,VIOLET,OTHERS_OFF);
    delay(500);
  }
  // run back the z axis
  for(int i = 0 ; i <=3; i++){
    myRainbow.light3D(0xff,0xff,i,YELLOW,OTHERS_OFF);
    delay(500);
  }
 
}
