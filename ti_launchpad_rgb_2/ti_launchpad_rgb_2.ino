/*
  written by joe+energia@freemansoft.com http://joe.blog.freemansoft.com/
  This is a simple monitoring program that lets you use a TI Launchpad + an RGB LED as a status device.
  The program will flash the red LED2 and the RGB LED will be off on reset
  
  PC and other programs communicate over the USB emulated serial port @ 9600 baud
  All commands must end with a carraige return.
  Send "h\cr" to get a list of commands
  
  This understands commands in the format
  "rgb red green blue blink" to set light color and blink pattern
    red, green, blue are 0-255
    blink is 0-9
  "?" means help
 */
 
#include "AnyMsTimer.h"

// RGB LEDs can be common Anode   (high) and the RGB (pwm) pins can sink (pull low) to enable
// RGB LEDs can be common Cathode (low)  and the RGB (pwm) pins drive to enable.  
// LED_ON_HIGH=true means common cathode, the pins drive
// LED_ON_HIGH=false means common anode, the pins sink
// Stellaris launchpad onboard LED is LED_ON_HIGH
//
const boolean LED_ON_HIGH = true;             // are LEDs on with a high or a low?
#if defined(_MSP430_CONFIG_)
const int PWM_PINS[] = { P2_2, P1_6, P2_5 };  // red green blue for joe's demo board
// used as a scope test pin
const int HEARTBEAT_PIN = P1_0;               // toggles with heartbeat - used for debugging
// step size per step slower cpu do bigger steps
const int MAX_PWM_CHANGE_SIZE = 32 ;     
#elif defined(_CORTEX_CONFIG_)
const int PWM_PINS[] = { RED_LED, GREEN_LED, BLUE_LED }; // red green blue
// used as a scope test pin
const int HEARTBEAT_PIN = PB_2;    
// step size per step faster CPU with better pwm can do smaller steps 
const int MAX_PWM_CHANGE_SIZE = 16 ;     
#endif

// mark variables used in interrupt handlers as volatile
volatile int rgbActiveValues[] = { 255, 255, 255 };
volatile int rgbLastWrittenValues[] = {0, 0, 0};
volatile int ledPattern = 0;              // which pattern in the ACTION_PATTERNS
volatile int ledCurrentState = 0;         // which location in the state list for the current pattern
volatile unsigned long ledLastChangeTime;

// time between steps
const int STATE_STEP_INTERVAL = 10; // in milliseconds - all our table times are even multiples of this

boolean heartbeatStepOn = false;
long heartbeatStepCount = 0;

// the number of pattern steps in every bell pattern 
const int MAX_NUM_STATES = 4;
typedef struct
{
  boolean  isActive;          // digital value for this state to be active (Ring/Silent)
  unsigned long activeTime;    // time to stay active in this state stay in milliseconds 
} stateDefinition;
typedef struct
{
  stateDefinition state[MAX_NUM_STATES];    // can pick other numbers of slots
} stateTemplate;

// the length of this array should == NUM_PATTERNS
const int NUM_PATTERNS = 10;
// blink patterns
const stateTemplate ACTION_PATTERNS[] =
{
    //  state0 state1 state2 state3 
  { /* no variable before stateDefinition*/ {{false, 10000}, {false, 10000}, {false, 10000}, {false, 10000}} /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true,  10000},  {true, 10000},  {true, 10000},  {true, 10000}} /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 300}, {false, 300}, {false, 300}, {false, 300}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 300}, {true , 300}, {true , 300}, {true , 300}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 200}, {false, 100}, {true , 200}, {false, 800}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 200}, {true , 100}, {false, 200}, {true , 800}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 300}, {false, 400}, {true , 150}, {false, 400}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 300}, {true , 400}, {false, 150}, {true , 400}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 100}, {false, 100}, {true , 100}, {false, 800}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 100}, {true , 100}, {false, 100}, {true , 800}}  /* no variable after stateDefinition*/ },
};

void setup()  { 
  // open the hardware serial port
  Serial.begin(9600);
  pinMode(HEARTBEAT_PIN, OUTPUT);  // for future
  ledLastChangeTime = millis();
  AnyMsTimer::set(STATE_STEP_INTERVAL, process_step);
  AnyMsTimer::start();
  // should this happen before starting timer?
  showPowerupSequence();  
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
    // terminate c style string
    readBuffer[readCount] = '\0';
    // got a command so parse it
    process_command(readBuffer);
  } 
  else {
    // too many characters so start over
  }
}

// cheat and run the command processor to show colors
void showPowerupSequence(){
  char showRed[] = "rgb 250 0 0 1";
  process_command(showRed);
  delay(250);
  char showGreen[] = "rgb 0 250 0 1";
  process_command(showGreen);
  delay(250);
  char showBlue[] = "rgb 0 0 250 1";
  process_command(showBlue);
  delay(250);
  char showOff[] = "rgb 0 0 0 0";
  process_command(showOff);
}

/*=============================================
 * interrupt handler section
 *=============================================*/

// Interrupt handler that smoothly moves the servo
void process_step(){
  check_led();
  update_heartbeat_led();
}

// led portion of the interrupt handler
void check_led(){
  if (ledPattern >= 0 && ledPattern < NUM_PATTERNS){ // quick range check
    if (millis() >= ledLastChangeTime + ACTION_PATTERNS[ledPattern].state[ledCurrentState].activeTime){
      // calculate next state with rollover/repeat
      ledCurrentState = (ledCurrentState+1) % MAX_NUM_STATES;
      ledLastChangeTime = millis();
    }
    // hopefully this won't cause slight flicker if already showing led
    if (ACTION_PATTERNS[ledPattern].state[ledCurrentState].isActive){
      for ( int i = 0 ; i < 3; i++){
        set_led_value(i,rgbActiveValues[i]);
      }
    } else {
      for ( int i = 0 ; i < 3; i++){
        set_led_value(i,0);
      }
    }
  }
}

// tracks all values sent to LEDs so can do smoother transition
void set_led_value(int ledIndex, int value){
  // do some brightness smoothing
  int valueToBeWritten = value;
  int diffSinceLast = value-rgbLastWrittenValues[ledIndex];
  if (diffSinceLast > MAX_PWM_CHANGE_SIZE){
    valueToBeWritten =rgbLastWrittenValues[ledIndex]+MAX_PWM_CHANGE_SIZE;
  } else if (diffSinceLast < -MAX_PWM_CHANGE_SIZE){
    valueToBeWritten = rgbLastWrittenValues[ledIndex]-MAX_PWM_CHANGE_SIZE;
  }
  if (LED_ON_HIGH){
    analogWrite(PWM_PINS[ledIndex],valueToBeWritten);
  } else {
    analogWrite(PWM_PINS[ledIndex],255-valueToBeWritten);
  }
  rgbLastWrittenValues[ledIndex] = valueToBeWritten;
}

// flash the onboard LED to show the app is running
void update_heartbeat_led(){
  heartbeatStepCount++;
  heartbeatStepCount = heartbeatStepCount % (1000/STATE_STEP_INTERVAL);
  if (heartbeatStepCount == 0){
    heartbeatStepOn = !heartbeatStepOn;
    digitalWrite(HEARTBEAT_PIN,heartbeatStepOn);
  }
}
/*================================================================================
 *
 * command handler area 
 *
 *================================================================================*/

// first look for commands without parameters then with parametes 
boolean  process_command(char *readBuffer){
  // use string tokenizer to simplify parameters -- could be faster by only running if needed
  char *command;
  char *parsePointer;
  //  First strtok iteration
  command = strtok_r(readBuffer," ",&parsePointer);

   boolean processedCommand = false;
  if (strcmp(command,"h") == 0 || strcmp(command,"?") == 0){
    help();
    processedCommand = true;
  } 
  else if (strcmp(command,"rgb") == 0){
    char * red = strtok_r(NULL," ",&parsePointer);
    char * green = strtok_r(NULL," ",&parsePointer);
    char * blue = strtok_r(NULL," ",&parsePointer);
    char * pattern = strtok_r(NULL," ",&parsePointer);
    if (red == NULL || green == NULL || blue == NULL || pattern == NULL){
      help();
    } else {
      // this code shows how lazy I am.
      int numericValue;
      numericValue = atoi(red);
      if (numericValue < 0) { numericValue = 0; };
      if (numericValue > 255) { numericValue = 255; };
      rgbActiveValues[0] = numericValue;
      numericValue = atoi(green);
      if (numericValue < 0) { numericValue = 0; };
      if (numericValue > 255) { numericValue = 255; };
      rgbActiveValues[1] = numericValue;
      numericValue = atoi(blue);
      if (numericValue < 0) { numericValue = 0; };
      if (numericValue > 255) { numericValue = 255; };
      rgbActiveValues[2] = numericValue;
      numericValue = atoi(pattern);
      if (numericValue < 0) { numericValue = 0; };
      if (numericValue > NUM_PATTERNS) { numericValue = NUM_PATTERNS-1; };
      // don't affect the flashing timing if requested current pattern number
      if (ledPattern != numericValue){
        ledPattern = numericValue;   
        ledLastChangeTime = millis();
        check_led();
      }
      processedCommand = true;   
    }
  } else {
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
  Serial.println("rgb <red 0..255> <green 0..255> <blue 0..255> <pattern 0..9 0:off, 1:continuous> set color and blink pattern");
  Serial.flush();
}



