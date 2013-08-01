/*
  Written by Freemansoft Inc.
  Exercise Neopixel shield using the adafruit NeoPixel library
 */
#include <Adafruit_NeoPixel.h>
#include <MsTimer2.h>

boolean logDebug = false;
// pin used to talk to NeoPixel
#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
// using the arduino shield which is 5x8
const int NUM_PIXELS = 40;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);


typedef struct
{
  uint32_t activeValues;         // packed 32 bit created by Strip.Color
  uint32_t lastWrittenValues;    //for fade in the future
  byte currentState;             // used for blink
  byte pattern;
  unsigned long lastChangeTime;  // where are we timewise in the pattern
} pixelDefinition;
// should these be 8x5 intead of linear 40 ?
volatile pixelDefinition lightStatus[NUM_PIXELS];



///////////////////////////////////////////////////////////////////////////

// time between steps
const int STATE_STEP_INTERVAL = 10; // in milliseconds - all our table times are even multiples of this
const int MAX_PWM_CHANGE_SIZE = 32; // used for fading at some later date

/*================================================================================
 *
 * bell pattern buffer programming pattern lifted from http://arduino.cc/playground/Code/MultiBlink
 *
 *================================================================================*/

typedef struct
{
  boolean  isActive;          // digital value for this state to be active (on off)
  unsigned long activeTime;    // time to stay active in this state stay in milliseconds 
} stateDefinition;

// the number of pattern steps in every blink  pattern 
const int MAX_STATES = 4;
typedef struct
{
  stateDefinition state[MAX_STATES];    // can pick other numbers of slots
} ringerTemplate;

const int NUM_PATTERNS = 10;
const ringerTemplate ringPatterns[] =
{
    //  state0 state1 state2 state3 
  { /* no variable before stateDefinition*/ {{false, 10000}, {false, 10000}, {false, 10000}, {false, 10000}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true,  10000},  {true, 10000},  {true, 10000},  {true, 10000}}   /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 300}, {false, 300}, {false, 300}, {false, 300}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 300}, {true , 300}, {true , 300}, {true , 300}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 200}, {false, 100}, {true , 200}, {false, 800}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 200}, {true , 100}, {false, 200}, {true , 800}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 300}, {false, 400}, {true , 150}, {false, 400}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 300}, {true , 400}, {false, 150}, {true , 400}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{true , 100}, {false, 100}, {true , 100}, {false, 800}}  /* no variable after stateDefinition*/ },
  { /* no variable before stateDefinition*/ {{false, 100}, {true , 100}, {false, 100}, {true , 800}}  /* no variable after stateDefinition*/ },
};


///////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(19200);
  // don't want to lose characters if interrupt handler too long
  // serial interrupt handler can't run so arduino input buffer length is no help
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // initialize our buffer as all LEDS off
  unsigned long ledLastChangeTime = millis();
  for ( int index = 0 ; index < NUM_PIXELS; index++){
    lightStatus[index].currentState = 0;
    lightStatus[index].pattern = 0;
    lightStatus[index].activeValues = strip.Color(0,0,0);
    lightStatus[index].lastChangeTime = ledLastChangeTime;
  }
  
  //configureForDemo();
  
  MsTimer2::set(STATE_STEP_INTERVAL, process_blink);
  MsTimer2::start();
}

void loop(){
  const int READ_BUFFER_SIZE = 4*6; // rgb_lmp_red_grn_blu_rng where ringer is only 1 digit but need place for \0
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
    // this has to be before process_Command because buffer is wiped
    if (logDebug){
        Serial.print("received ");
        Serial.print(readCount);
        Serial.print(" characters, command: ");
        Serial.println(readBuffer);
    }
    // got a command so parse it
    process_command(readBuffer,readCount);
  } 
  else {
    // too many characters so start over
  }
}

//////////////////////////// handler  //////////////////////////////
//
/*
  Interrupt handler that handles all blink operations
 */
void process_blink(){
  unsigned long now = millis();
  for ( int index = 0 ; index < NUM_PIXELS; index++){
    byte currentPattern = lightStatus[index].pattern; 
    if (currentPattern >= 0){ // quick range check for valid pattern?
      if (now >= lightStatus[index].lastChangeTime + ringPatterns[currentPattern].state[lightStatus[index].currentState].activeTime){
        // calculate next state with rollover/repeat
        int currentState = (lightStatus[index].currentState+1) % MAX_STATES;
        lightStatus[index].currentState = currentState;
        lightStatus[index].lastChangeTime = now;

        // will this cause slight flicker if already showing led?
        if (ringPatterns[currentPattern].state[currentState].isActive){
          strip.setPixelColor(index, lightStatus[index].activeValues);
        } else {
          strip.setPixelColor(index,strip.Color(0,0,0));
        }
      }
    }
  }
  strip.show();
}

// first look for commands without parameters then with parametes 
boolean  process_command(char *readBuffer, int readCount){
  int indexValue;
  byte redValue;
  byte greenValue;
  byte blueValue;
  byte patternValue;
  
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
  else if (strcmp(command,"debug") == 0){
    char * shouldLog   = strtok_r(NULL," ",&parsePointer);
    if (strcmp(shouldLog,"true") == 0){
      logDebug = true;
    } else {
      logDebug = false;
    }
    processedCommand = true;
  }
  else if (strcmp(command,"rgb") == 0){
    char * index   = strtok_r(NULL," ",&parsePointer);
    char * red     = strtok_r(NULL," ",&parsePointer);
    char * green   = strtok_r(NULL," ",&parsePointer);
    char * blue    = strtok_r(NULL," ",&parsePointer);
    char * pattern = strtok_r(NULL," ",&parsePointer);
    if (index == NULL || red == NULL || green == NULL || blue == NULL || pattern == NULL){
      help();
    } else {
      // this code shows how lazy I am.
      int numericValue;
      numericValue = atoi(index);
      if (numericValue < 0) { numericValue = -1; }
      else if (numericValue >= NUM_PIXELS) { numericValue = NUM_PIXELS-1; };
      indexValue = numericValue;
      
      numericValue = atoi(red);
      if (numericValue < 0) { numericValue = 0; }
      else if (numericValue > 255) { numericValue = 255; };
      redValue = numericValue;
      numericValue = atoi(green);
      if (numericValue < 0) { numericValue = 0; }
      else if (numericValue > 255) { numericValue = 255; };
      greenValue = numericValue;
      numericValue = atoi(blue);
      if (numericValue < 0) { numericValue = 0; }
      else if (numericValue > 255) { numericValue = 255; };
      blueValue = numericValue;
      numericValue = atoi(pattern);
      if (numericValue < 0) { numericValue = 0; }
      else if (numericValue > NUM_PATTERNS) { numericValue = NUM_PATTERNS-1; };
      patternValue = numericValue;
      /*
      Serial.println(indexValue);
      Serial.println(redValue);
      Serial.println(greenValue);
      Serial.println(blueValue);
      Serial.println(patternValue);
      */
      if (indexValue >= 0){
        lightStatus[indexValue].activeValues = strip.Color(redValue,greenValue,blueValue);
        lightStatus[indexValue].pattern = patternValue;
      } else {
        for (int i = 0; i < NUM_PIXELS; i++){
          lightStatus[i].activeValues = strip.Color(redValue,greenValue,blueValue);
          lightStatus[i].pattern = patternValue;
          
        }
      }
      processedCommand = true;   
    }
  } else {
    // completely unrecognized
  }
  if (!processedCommand){
    Serial.print(command);
    Serial.println(" not recognized ");
  }
  return processedCommand;
}

void help(){
  Serial.println("h: help");
  Serial.println("?: help");
  Serial.println("rgb <led 0..39> <red 0..255> <green 0..255> <blue 0..255> <pattern 0..9>: set RGB pattern to  pattern <0:off, 1:continuous>");
  Serial.println("rgb <all -1>    <red 0..255> <green 0..255> <blue 0..255> <pattern 0..9>: set RGB pattern to  pattern <0:off, 1:continuous>");
  Serial.println("debug <true|false> log all input to serial");
  Serial.flush();
}

//////////////////////////// demo  //////////////////////////////

/*
 * show the various blink patterns
 */
void configureForDemo(){
  unsigned long ledLastChangeTime = millis();
  for ( int index = 0 ; index < NUM_PIXELS; index++){
    lightStatus[index].currentState = 0;
    lightStatus[index].pattern = (index%8)+1; // the shield is 8x5 so we do 8 patterns and we know pattern 0 is off
    lightStatus[index].activeValues = Wheel(index*index & 255);
    lightStatus[index].lastChangeTime = ledLastChangeTime;
  }
  uint16_t i;  
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i,lightStatus[i].activeValues);
  }     
  strip.show();
}

//////////////////////////// old stuff  //////////////////////////////


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

