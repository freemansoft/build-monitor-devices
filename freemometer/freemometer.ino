/**
 * download the MsTimer2 library and put it in your arduino workspace's libraries directory.
 * The workspace is where all your arduino projects are.  You may have to create the libraries directory.
 * 
 * Written by Joe Freeman joe+arduino@freemansoft.com
 * http://joe.blog.freemansoft.com/2012/07/aruino-servo-ikea-dekad-freemometer.html
 * 
 * This is the command firmware fro the Freemometer USB controlled gauge, light and alarm built
 * on top of the Ikea Dekad retro clock with external hammer bells. Details are on my blog
 * 
 */

#include <Servo.h> 
#include <MsTimer2.h>

// these two may have to be changed if you swap the connector on the LED
// assumes NTE30112 2-wire bi-color LED that can be purchased at Microcenter in casemod section
// would also work with 3-wire LED
const int PIN_LED_GREEN =  12;    // bias green LED connected to digital pin 
const int PIN_LED_RED =  13;    // bias red LED connected to digital pin 
// servo control pin
const int PIN_BELL = 7;
// tune to your servo.  Mine was 138 degrees with this offset from center.
const int SERVO_START_9 = 38;
const int SERVO_START_10 = 38;
const int SERVO_END_9 = 176;
const int SERVO_END_10 = 176;
const int SERVO_FULL_RANGE = 100;

// Servo library uses Timer1 which means no PWM on pins 9 & 10.
// might as well configure both for servos then.
// Timer 0 used for millis() and micros() and PWM
// Timer 2 available for PWM 8 bit timer or MyTimer2
Servo servo10;
Servo servo9;
// current position
volatile int servoPosition9 = 0;
volatile int servoPosition10 = 0;
// where we want to be
volatile int servoTarget9 = 0;
volatile int servoTarget10 = 0;

// adjust these two for noise and speed
// how many steps per interrupt
int servoSpeedPerStep = 4;  // in degrees
// time between steps
int servoStepInterval = 30; // in milliseconds 

// when to turn off the bell 0==bell is already off
volatile unsigned long bellEndTime = 0;

// The setup() method runs once, when the sketch starts

void setup()   {                
  // initialize the digital pin as an output:
  Serial.begin(19200);
  pinMode(PIN_BELL,OUTPUT);
  bell_silence();
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  led_off();
  // stat the servo code
  servo9.attach(9);  // attaches the servo on pin 9 to the servo object 
  servo10.attach(10);  // attaches the servo on pin 9 to the servo object 
  // put them in the star tposition
  servoPosition9 = SERVO_START_9;
  servoPosition10 = SERVO_START_10;
  // will be on start so set end to be same as start
  servoTarget9 = servoPosition9;
  servoTarget10 = servoPosition10;
  // actually position the servos to their initial position - without delay
  servo9.write(servoPosition9);
  servo10.write(servoPosition10);
  // register function for timer handler at 15ms
  MsTimer2::set(servoStepInterval, process_servos_and_bell);
  MsTimer2::start();

  delay(200);
  servo_10_demo(1);
}

// main loop that runs as long as Arduino has power
void loop()                     
{
  const int READ_BUFFER_SIZE = 20;
  char readBuffer[READ_BUFFER_SIZE];
  int readCount = 0;
  char newCharacter = '\0';
  while((readCount < READ_BUFFER_SIZE) && newCharacter !='\r'){
    if (Serial.available()){
      newCharacter = Serial.read();
      if (newCharacter != '\r'){
        readBuffer[readCount] = newCharacter;
        readCount++;
      }
    }
  }
  if (newCharacter == '\r'){
    readBuffer[readCount] = '\0';
    // got a command so parse it
    //    Serial.print("received ");
    //    Serial.print(readCount);
    //    Serial.print(" characters, command: ");
    //    Serial.println(readBuffer);
    process_command(readBuffer,readCount);
  } 
  else {
    // too many characters so start over
  }
}

/*=============================================
 * make stuff happen
 *=============================================*/

// this should move to the interrupt handler like the servo code
void bell_ring(int bellTimeMSec){
  unsigned long now = millis();
  bellEndTime = now+(long)bellTimeMSec;
  if (bellEndTime < now){
    // timer rollover -- will just bing once and stop immediately
  }
  digitalWrite(PIN_BELL,HIGH);          // ring bell
}

// convenience for command
void bell_silence(){
  bellEndTime = 0;
  digitalWrite(PIN_BELL,LOW);          // silence bell
}

void led_off(){
  digitalWrite(PIN_LED_GREEN, LOW);    // set the LED on
  digitalWrite(PIN_LED_RED, LOW);       // set the LED off
}

void led_green(){
  digitalWrite(PIN_LED_GREEN, HIGH);    // set the LED on
  digitalWrite(PIN_LED_RED, LOW);       // set the LED off
}

void led_red(){
  digitalWrite(PIN_LED_GREEN, LOW);     // set the LED off
  digitalWrite(PIN_LED_RED, HIGH);  // set the LED on
}

void servo_set(int numericParameter){
  servo_set_10(numericParameter);
}

// we just set the target because the intterupt handler actually moves the servo
void servo_set_10(int numericParameter){
  // do bounds checking?
  if (numericParameter <= 0){
    servoTarget10 = SERVO_START_10;
  } 
  else if (numericParameter >= SERVO_FULL_RANGE){
    servoTarget10 = SERVO_END_10;
  } 
  else {
    // calculate percentage of full range
    int range = SERVO_END_10 - SERVO_START_10;
    long multiplicand = range * numericParameter;
    int scaledValue = multiplicand / SERVO_FULL_RANGE;
    int offsetValue = SERVO_START_10 + scaledValue;
    servoTarget10 = offsetValue;
  }
}

/*=============================================
 * interrupt handler section
 *=============================================*/

// Interrupt handler that smoothly moves the servo
void process_servos_and_bell(){
  move_servo(servo10, "S10", servoPosition10, servoTarget10, SERVO_START_10, SERVO_END_10);
  move_servo(servo9,  "S9", servoPosition9, servoTarget9, SERVO_START_9, SERVO_END_9);
  check_bell();
}

void check_bell(){
  if (bellEndTime != 0){
    if (bellEndTime < millis()){
      bell_silence();
    }
  } 
  else {
    // really only need to do this one time
    bell_silence();
  }
}

// supports servo movement for multile servos even though prototype only has one
void move_servo(Servo servo, String label, volatile int &currentPosition, volatile int &targetPosition ,int servoMinimum, int servoMaximum){
  // only move the servo or process if we're not yet at the target
  if (currentPosition != targetPosition){
    if (currentPosition < targetPosition){
      currentPosition+=servoSpeedPerStep;
      if (currentPosition>targetPosition){
        currentPosition = targetPosition;
      }
      if (currentPosition>servoMaximum){
        currentPosition = servoMaximum;
      }
    }
    if (currentPosition > targetPosition){
      currentPosition-=servoSpeedPerStep;
      if (currentPosition<targetPosition ){
        currentPosition = targetPosition;
      }
      if (currentPosition<servoMinimum){
        currentPosition=servoMinimum;
      }
    }
    servo.write(currentPosition);
    //    Serial.print(label);
    //    Serial.print(":");
    //    Serial.println(currentPosition);
  }
}

/*=============================================
 * demo code
 *=============================================*/

// runs a short demo on servo 10
void servo_10_demo(int numberOfDemoCycles){
  for (int i = 0 ; i < numberOfDemoCycles; i++){
    led_green();
    servo_set(SERVO_FULL_RANGE);
    delay(1000);        // wait
    led_off();
    bell_ring(50);
    delay(1000);                  // wait

    led_red();
    servo_set(0);
    delay(1000);                  // wait
    led_off();
    bell_ring(50);
    delay(1000);                  // wait
  }
}

/*================================================================================
 *
 * command handler area 
 *
 *================================================================================*/

// first look for commands without parameters then with parametes 
boolean  process_command(char *readBuffer,int readCount){
  // use string tokenizer to simplify parameters -- could be faster by only running if needed
  char *command;
  char *command2;
  char *parameter;
  char *parsePointer;
  //  First strtok iteration
  command = strtok_r(readBuffer," ",&parsePointer);
  command2 = strtok_r(NULL," ",&parsePointer);
  parameter = strtok_r(NULL," ",&parsePointer); // optional parameter

    boolean processedCommand = false;
  if (readCount == 1){
    help();
  } 
  else if (strcmp(command,"led") == 0){
    if (strcmp(command2,"green")==0){
      led_green();
      processedCommand = true;
    } 
    else if (strcmp(command2,"red")==0){
      led_red();
      processedCommand = true;
    } 
    else if (strcmp(command2,"off")==0){
      led_off();
      processedCommand = true;
    }
  } 
  else if (strcmp(command,"bell") == 0){
    if (strcmp(command2,"silence")==0){
      bell_silence();
      processedCommand = true;
    } 
    else if (strcmp(command2,"ring")==0){
      if (parameter != NULL){
        int numericParameter = atoi(parameter);
        if (numericParameter < 0){ 
          numericParameter = 0; 
        }
        else if (numericParameter > 30000) { 
          numericParameter = 30000; 
        }
        bell_ring(numericParameter);
        processedCommand = true;
      }
    }
  } 
  else if (strcmp(command,"servo")==0){
    if (strcmp(command2,"set") == 0){
      if (parameter != NULL){
        int numericParameter = atoi(parameter);
        if (numericParameter < 0){ 
          numericParameter = 0; 
        }
        else if (numericParameter > SERVO_FULL_RANGE) { 
          numericParameter = SERVO_FULL_RANGE; 
        }
        servo_set(numericParameter);
        processedCommand = true;
      }
    }
  } 
  else {
    // completely unrecognized
  }
  if (!processedCommand){
    Serial.print(command);
    Serial.println(" not recognized");
  }
  return processedCommand;
}

void help(){
  Serial.println("h: help");
  Serial.println("?: help");
  Serial.println("bell ring <time_msec>: ring bell max 30 seconds");
  Serial.println("bell silence: turn off bell");
  Serial.println("led red: turn on red LED");
  Serial.println("led green: turn on green LED");
  Serial.println("led off: turn off LED");
  Serial.print  ("servo set <0-");Serial.print(SERVO_FULL_RANGE);Serial.println(">: move servo maps to maximum range");
  Serial.flush();
}

// end of file

