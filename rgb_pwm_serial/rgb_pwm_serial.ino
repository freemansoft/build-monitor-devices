#include <Wire.h>   // include Wire library for I2C
/*
  rgb_pwm_serial v11.06
  This program written by Joe Freeman, joe@freemansoft.com
  Give me some credit if you this code in our project :-)
 
  This program automates color and blink rates based on passed in settings
  It ramps up and down the colors so we don't get sudden LED style transitons
  
  The firmware supports two RGB lights on the Arduino PWM ports.
  There can be up to do lights in this configuration
 
  It also supports 5 lights
  on a PCA9635 I2C PWM chip, specifically the JeeLabs Dimmer plug.
  The firmware auto-detects if there is a PCA9635 on the I2C bus at 0x40
  and falls back to the internal PWM if there is no PCA9635 found
  
  Additional functionality has been hacked in to allow control of the 32 LED
  Sparkfun Addressable LED strip. The control code is straight from the
  Sparkfun demo program.  You can connect a button from A5 to ground and
  the strip will rotate colors every time you press the button
  
  Note: Verify the value of the  INVERTED_PWM flag in this code.
  
  Accepts the followng commands:
  Color set max min color at 0
  ~c#RGB;
    # the led triplet (ascii starts at ascii '0')
    RGB  colors for each LED '0'-'F' (hex as characters) 16 values per pixel
    Ex: ~c1888; means set the color of all 3 RGB LEDs to half bright.
    echos  "+<command>" on success
  Blink
  ~b#ooofff;
    # the led triplet (ascii starts at ascii '0')
    ooo "led on" time for each in 1/2 seconds ('0'-'F' ascii)
    fff "led off" time for each in 1/2 seconds ('0'-'F' ascii)
    Ex: ~b14414443 means blink light 1.  Set the red and green to 4 1/4 sec or 2 sec 
    echos "+<command>" on success
  Query
  ~q#;
    # of the led triplet (ascii starts at ascii '0')
    returns data to caller
    echos "+<command>" on success
  Color for single strip element on sparkfun addressable LED
  ~s#RGB;
    # of the led triplet (ascii starts at ascii '0'->'0'+31) not HEX
    RGB  colors for each LED '0'-'F' (hex as characters) 16 values per pixel
    echos  "+<command>" on success
  Color for all strip elements on sparkfun addressable LED individually in single command
  ~Scccccccccccccccccccccccccccccccc;
    32 colors where each color is a CSS1/HTML3-4 color from the 16 color palette
    Colors range from '0'-'F' (hex as characters) where the number is the CGA number
    See http://en.wikipedia.org/wiki/Web_colors
    black,navy,green,teal,maroon,purple,olive,silver,gray,blue,lime,aqua,red,fuschia,yellow,white
    echos  "+<command>" on success
  Help
  ~h;
    list out the commands
    
    REVISION HISTORY
    11.05 Initial version supports two lights on PWM ports
    11.06 Add support for PCA9635 I2C PWM chip as used in Jee Labs Dimmper Plug
          Support up to 5 lights.  Autodetect if PCA9635 present or fall back to dual
    11.07 Add support for Sparkfun Addressable LED strip
          Add analog button code for Addressable strip. Could be used for something else
 */

/* 
  number of LED triplets. Set this to 1 if you only have one RGB cluster. 
  set INVERTED_PWM to true if you have to sink current to turn LEDs.
*/
const boolean INVERTED_PWM = true;
/** 6 pwm pins means two PWM RGGB */
const int NUM_TRIPLETS_PWM = 2;
const int NUM_TRIPLETS_DIMMER_PLUG = 5;
/* the initialize method will change the number of triplets to match the hardware */
int numTriplets= 0;

/* 
  2 RGB LEDs using 6 of the PWM pins
  pwm pins R/G/B - change the order to suite your layout 
 */
int pwmPins[NUM_TRIPLETS_PWM][3]={{9,10,11}, {3,5,6}};
/* 
  jee devices has a breakout header.  Pins selected to make that (mostly) simpler wiring
  pwm pins R/G/B - change the order to suite your layout 
  These are layed out so that 1 light is in row 1, 2 in in row 2 and 4 in row 3.  
  The last LED is just the leftover pins so that one's wiring is a bit messy 
 */
int i2cPins[NUM_TRIPLETS_DIMMER_PLUG][3]={{0,1,2},{5,6,7},{8,9,10},{11,12,13},{3,4,14}};

// these are all sized as NUM_TRIPLETS_DIMMER_PLUG because it is the larger device supported
// The two LED (no I2C device) just use the first two slots

/* max brightness with defaults */
int brightnessPrimary[NUM_TRIPLETS_DIMMER_PLUG][3];
/*  min brightness with default */
int brightnessSecondary[NUM_TRIPLETS_DIMMER_PLUG][3];
/* current brightness settings */
int brightnessCurrent[NUM_TRIPLETS_DIMMER_PLUG][3];
/* blink interval  0 = no blink */
int blinkMsecPrimary[NUM_TRIPLETS_DIMMER_PLUG][3];
/* porton of blink interval where we are on must b e less than blinkMsec */
int blinkMsecSecondary[NUM_TRIPLETS_DIMMER_PLUG][3];
/* the last time we did something */
int lastMsecCycle = 0;
/*
 reset lastUpdateMsec when we reach this so we don't worry about timer rollover
 max is 2^15 but want to avoid interger math problems. 
*/
int rolloverMsec = 12000;
/* 
  change interval -- only make change every this many msec 
  too big a number here means you won't have enough time so will see step
 */
int changeIntervalMsec = 2;


/* PWM is fast so we can do smooth transitions */
const int TRANSITION_INCREMENT_PWM = 1;
/** I2C is slower so bigger increment means fewer cycles */
const int TRANSITION_INCREMENT_I2C = 3;
// even multiple of the transition increment or you get weird flickering at full brightness as it tries to close on it
int absoluteMaximumBrightness = (255/TRANSITION_INCREMENT_I2C)*TRANSITION_INCREMENT_I2C;

int transitionIncrement = 0;

/* 
  lookup table maps values to actual brightness to try and
  create linear brightness scale taking into account human eye
  from http://harald.studiokubota.com/wordpress/index.php/2010/09/05/linear-led-fading/
  exp($i/46)
  */
int correctedBrightnessTable[] = {
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,
  3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,
  5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,8,8,8,8,8,8,9,9,
  9,9,10,10,10,10,10,11,11,11,11,12,12,12,13,13,13,13,
  14,14,14,15,15,15,16,16,16,17,17,18,18,18,19,19,20,
  20,20,21,21,22,22,23,23,24,24,25,26,26,27,27,28,29,
  29,30,31,31,32,33,33,34,35,36,36,37,38,39,40,41,42,
  42,43,44,45,46,47,48,50,51,52,53,54,55,57,58,59,60,
  62,63,64,66,67,69,70,72,74,75,77,79,80,82,84,86,88,
  90,91,94,96,98,100,102,104,107,109,111,114,116,119,
  122,124,127,130,133,136,139,142,145,148,151,155,158,
  161,165,169,172,176,180,184,188,192,196,201,205,210,
  214,219,224,229,234,239,244,250,255
  };

/* a buffer of read charactes sized to the maximum command length*/
char readBuffer[37];
/* maximum command length is 2+32+1 or 35 characters (the ~s command) */
int readBufferSize = 36;
/* number of characters in the buffer */
int readBufferCount = 0;

/** the last analog button read used for tracking transitions*/
int lastAnalogButtonRead = 0;

// standard arduino setup method
void setup() {
  Serial.begin(19200);
  initializeBuffers();
  // first try the I2C
  Wire.begin();         // make ourselves the I2C master
  initializePCA9635();  // but we may switch to I2C if we find device
  // then initialize the PWM if needed. 
  initializePWM();  // start off as if we are PWM
  lastMsecCycle = millis() % rolloverMsec;
  readBufferCount = 0;
  // configure pins for possible Sparkfun Addressable LED strip
  setupLEDStrip();
  // setup pin A5 as a possible input
  // this turns on the pull up so we can detect shorts to ground without external
  pinMode(A5,INPUT);
  takeButtonAction();
  // added so PCs could detect when the device has come out of the boot loader
  Serial.print("initialized");
}

// standard arduino run loop
void loop()
{ 
  // command part of loop
  // read some characters from serial port
  int readCount = acquireAvailableSerialData(readBufferCount, readBuffer);
  // is new buffer size different than the one from before call?
  if (readCount != readBufferCount){
    // got new data so update counter
    readBufferCount = readCount;
  }
  if (readCount > 0){
    // add terminator so string prints work
    readBuffer[readCount] = '\0';
    // see if we have a completed command
    // string always starts with ~ character
    if ( isInvalidCommand(readBufferCount, readBuffer)){
      // out of synch or  buffer overrun possibly multiple strings sent too quickly
      handleInvalidBuffer();
    } else if ( readBuffer[0] == '~' && readBuffer[readCount-1] == ';'){
      // received complete command. may be valid or invalid
      readBufferCount = processCommandBuffer(readBufferCount, readBuffer);
    } else {
      // command not complete so ignore
    }
  }
  // possibly take button action
  takeButtonAction();
  
  // display part of loop
  handleLEDs();
}

/**
 * initialize all of the buffer variables
 * create a really bad color pattern to show unit alive
 */
void initializeBuffers(){
  // do the full buffer even if only running hardware pwm
  for (int triplet = 0; triplet < NUM_TRIPLETS_DIMMER_PLUG; triplet++){
    // sort of random (not really) blinking on startup
    blinkMsecPrimary[triplet][0]  = 1000;
    blinkMsecPrimary[triplet][1]  = 2000;
    blinkMsecPrimary[triplet][2]  = 1000;
    blinkMsecSecondary[triplet][0]  = 1000;
    blinkMsecSecondary[triplet][1]  = 2000;
    blinkMsecSecondary[triplet][2]  = 2000;
    for ( int rgb = 0; rgb < 3; rgb++){
      brightnessPrimary[triplet][rgb] = absoluteMaximumBrightness; 
      brightnessSecondary[triplet][rgb] = 0;
      brightnessCurrent[triplet][rgb] = 0;
    }
  }
}

/*----------------------------------------------------------------
 * start of serial command processing
 *----------------------------------------------------------------
 */

/*
 * non blocking read into allocated possibly partially populated buffer
 * reads until no data or reach buffer limit
 */
int acquireAvailableSerialData(int currentBufferCount, char *currentBuffer){
  int newBufferCount = currentBufferCount;
  char newCharacter = '\0';
  // keep reading while data available until we reach end of buffer or end of command marker
  while ((newBufferCount < readBufferSize) && newCharacter != ';' && Serial.available()) {
       newCharacter = Serial.read();
      currentBuffer[newBufferCount] = newCharacter;
      newBufferCount++;
  }
  return newBufferCount;
}

/*
 * used as part of validation 
 */
boolean isInvalidTriplet(char tripletNum){
  for ( int i = 0 ; i < numTriplets; i++){
    char equiv = '0'+i;
    if (tripletNum == equiv){
      return false; 
    }
  }
  return true;
}

/**
 * '0' through '0'+32 
 * totally lazy way to get more than 16 values in a single byte while supporting ascii entry
 */
int convertStripIndexToInt(char char1){
  int calculatedNumber = char1 - '0';
  return calculatedNumber;
}

/*
 * used as part of validation. kind of broken because converts unknowns to 0;
 */
boolean isInvalidStripNumber(char char1){
  int stripNumber = convertStripIndexToInt(char1);
  if (stripNumber < 0 && stripNumber > 31){
    return true;
  } else {
    return false;
  }
    
}

/*
 * !isInvlalidComand != valid command
 * @return true if this is an invalid command.  false if valid or not yet complete
 */
boolean isInvalidCommand(int currentBufferCount , char *currentBuffer){
  // we know array is already allocated so can always index even if data is garbage
  char firstCharacter = currentBuffer[0];
  char commandCharacter = currentBuffer[1];
  if (currentBufferCount == 0){
    // nothing in the buffer should never get here
    return false;
  } else if ((firstCharacter != '~' )|| (firstCharacter == '~' && currentBufferCount == readBufferSize)){
    // bad start character or overrun
    return true;
  } else if ((firstCharacter == '~') && currentBuffer[currentBufferCount-1] == ';'){
    // nested to save some processing
    // we think it's a command so check the length
    if (commandCharacter == 'c'){
      if (currentBufferCount != 7 || isInvalidTriplet(currentBuffer[2])){
        // invalid color command
        return true;
      }
    } else if (commandCharacter == 'b'){
      if (currentBufferCount != 10 || isInvalidTriplet(currentBuffer[2])){
        // invalid blink command
        return true;
      }
    } else if (commandCharacter == 'q'){
      if (currentBufferCount !=4  || isInvalidTriplet(currentBuffer[2])){
        // status command has no parameters
        return true;
      }
    } else if (commandCharacter == 's'){
      if (currentBufferCount != 7 || isInvalidStripNumber(currentBuffer[2])){
        // invalid strip command
        return true;
      }
    } else if (commandCharacter == 'S'){
      if (currentBufferCount != 35){
        // invalid full strip command
        return true;
      }
    } else if (commandCharacter == 'h' && currentBufferCount != 3){
      // list of commands
      return true;
    }
  } 
  return false;
}

/* 
  reply to any failed command with '-'<failed command>
  pulled into method because we use the code from several places
 */
void handleInvalidBuffer(){
  // tell caller is invalid and clear buffer
  Serial.print('-');
  Serial.print(readBuffer);
  readBuffer[0] = '\0';
  readBufferCount = 0;
}

void echoValidCommand(const char *currentBuffer){
   Serial.print  ("+");Serial.print(currentBuffer);
}

/*
 * run possibly valid command
 * echo '+'<command> fior any valid command
 * @return number of character left in buffer
 */
int processCommandBuffer(int currentBufferCount , char *currentBuffer){
  char command = currentBuffer[1];
  if (command == 'c'){
    echoValidCommand(currentBuffer);
    processColorCommand(currentBuffer);
  } else if (command == 'b'){
    echoValidCommand(currentBuffer);
    processBlinkCommand(currentBuffer);
  } else if (command == 'q'){
    echoValidCommand(currentBuffer);
    processStatusQuery(currentBuffer);
  } else if (command == 's'){
    echoValidCommand(currentBuffer);
    processStripCommand(currentBuffer);
  } else if (command == 'S'){
    echoValidCommand(currentBuffer);
    processFullStripCommand(currentBuffer);
  } else if (command == 'h') {
    echoValidCommand(currentBuffer);
    processHelpRequest();
  } else {
    // unrecognized ; should never get here
    Serial.print("?");Serial.print(currentBuffer);
  }
  currentBuffer[0] = '\0';
  return 0;
}

/*
 * convert '0','1'...'F' to 0,1,...,15
 * support 'a'..'f' also
 */
int convertAsciiHexToInt(char character){
  int numeric = character - '0';
  if (numeric > 9){
    // 'A' is 7 up from 9
    numeric = numeric - 7;
  }
  // 'a' is 26+6 up from 'A'
  if (numeric > 15){
    numeric = numeric -32;
  }
  return numeric;
}

/**
 * list of valid status commands
 * there is some weird sensitity to the strings in here
 */
void processHelpRequest(){
  Serial.println("");
  Serial.println("~c#RGB;    set color    #=LED, RGB is 0-F");
  Serial.println("~b#lllddd; set blink    #=LED, lll RGB is 1/2 secs lit and ddd RGB is 1/2 secs dark");
  Serial.println("~q#;       query status #=LED, 3 LED x4 attrib on/off/blink-on/blink-off");
  Serial.println("~s#RGB;    set strip color #=LED '0'-'0'+31, RGB is 0-F");
  Serial.println("~Scc...;   whole strip  32 colors '0'-'F' from CSS1/HTML3-4 color palette");
  Serial.println("~h;        this help string");
  Serial.println("");
  
}


/*----------------------------------------------------------------
 * start of pwm commands
 *----------------------------------------------------------------
 */

/*
 * calculates the base index of the requested triplet
 * assumes command format is ~c# where # is the RGB triplet number
 */
int getTripletNumber(char *currentBuffer){
  // single digits are easy to convert
  int triplet = currentBuffer[2] - '0'; 
  // the digit was the triplet number calculate array offset
  return triplet;  
}

/* update color settings based on text buffer */
void processColorCommand(char *currentBuffer){
  // no validation!  this is probably bad!
  int triplet = getTripletNumber(currentBuffer);
  for ( int i = 0 ; i < 3; i++){
    // calculate the brightness should be '0'-'F'
    int requestedColor = convertAsciiHexToInt(currentBuffer[3+i]);
    // '1' --> 0x11, '2' --> 0x22...
    int color = (requestedColor << 4)+requestedColor;
    brightnessPrimary[triplet][i] = min(color,absoluteMaximumBrightness);
  }
}

/**
  * update blink settings based on text buffer 
  * format is lllddd where each digit is a time in 1/2 secs
  * in the sequence RGBRGB
  */
void processBlinkCommand(char *currentBuffer){
  int triplet = getTripletNumber(currentBuffer);
  for (int i = 0; i < 3; i++){
    // single digits are easy to convert
    int onTimeHalfSeconds = convertAsciiHexToInt(currentBuffer[3+i]);
    int onTimeMsec = onTimeHalfSeconds*500;
    int offTimeHalfSeconds = convertAsciiHexToInt(currentBuffer[6+i]);
    int offTimeMsec = offTimeHalfSeconds *500;
    // convert to msec
    blinkMsecPrimary[triplet][i] = onTimeMsec;
    blinkMsecSecondary[triplet][i] = offTimeMsec;
  }
}

/**
  * return status of all registers
  */
void processStatusQuery(char *currentBuffer){
  int triplet = getTripletNumber(currentBuffer);
  for (int i = 0; i < 3; i++){
    if (i !=0){ Serial.print(":"); }
    int maxBright = brightnessPrimary[triplet][i];
    Serial.print(maxBright);
    
    Serial.print(",");
    int minBright = brightnessSecondary[triplet][i];
    Serial.print(minBright);
    
    Serial.print(",");
    int on =  blinkMsecPrimary[triplet][i];
    Serial.print(on);

    Serial.print(",");
    int off = blinkMsecSecondary[triplet][i];
    Serial.print(off);
  }
  Serial.print("");
}

/*----------------------------------------------------------------
 * start pwm utilites
 *----------------------------------------------------------------
 */
 
boolean usePWM(){
  if (numTriplets == NUM_TRIPLETS_PWM){
    return true;
  } else {
    return false;
  }
}

void initializePWM(){ 
  if (numTriplets == 0){
    // assume we are using PWM
    numTriplets = NUM_TRIPLETS_PWM;
    transitionIncrement = TRANSITION_INCREMENT_PWM;
    int aPin;
    // roll across all pins
    for (int triplet = 0; triplet < NUM_TRIPLETS_PWM; triplet++){
      for (int aPin = 0; aPin < 3; aPin++)  {
        // set the mode
        pinMode(pwmPins[triplet][aPin], OUTPUT);  
        // default to min brightness
        brightnessCurrent[triplet][aPin] = brightnessSecondary[triplet][aPin];
        // start them in the initial brightness
          writePwmLEDViaLookup(triplet,aPin);
      }
    }
  }
}
  
/*
 * write out the PWM value based on a lookup in the log table
 * another evil function that knows about global variables
 */
void writePwmLEDViaLookup(int triplet,int pin){
    int brightnessIndex = brightnessCurrent[triplet][pin];
    if (INVERTED_PWM){
      // inverted pin control when sink to light up
      analogWrite(pwmPins[triplet][pin],255-correctedBrightnessTable[brightnessIndex]);  
    } else {
      // standard pin control wource to light up
      analogWrite(pwmPins[triplet][pin],correctedBrightnessTable[brightnessIndex]);  
    }
}

/*====================================================================================
 *common code
 *====================================================================================
 */
 
/*
 * process all the LEDs checking to see if they need to change values
 */
void handleLEDs(){
  // milliseconds are a long
  unsigned long time = millis();
  // convert to int to stay in int math - modulo timer rollover period
  int msec = time % rolloverMsec;
  // the rollover means we have a boundary condition where lastMsecCycle 
  // would be bigger than msec on rollover so "modulo" lastMsecCycle
  if (msec < lastMsecCycle){
    // rely on signed math below
    lastMsecCycle = lastMsecCycle - rolloverMsec;
  }
  // evaluate each pin once per millisecond 
  if (msec > lastMsecCycle){
    // if we're in a different msec then 
    // start calculating new values for each pin
    for (int triplet = 0; triplet < numTriplets; triplet++){
      for (int aPin = 0; aPin < 3; aPin++){
        handleLEDWithBlink(msec,triplet,aPin);
      }
    }
    lastMsecCycle = msec;
  }
}

/*
 * process a single LED
 * @param msec the current time (modulo) to figure out our position
 * @parm aPin the pin we are operating on
 */
void handleLEDWithBlink(int msec,int triplet,int aPin){
    boolean didChange = false;
    // see if we are in the odd or even half of a bink cycle
    // integer math, calculate the msec into the current cycle
    int msecCycle = blinkMsecPrimary[triplet][aPin] + blinkMsecSecondary[triplet][aPin];
    // i hope a%x == a where a < x
    int msecOffset = msec % msecCycle;
    // larger changeIntervalMsec means less CPU and do smooth on/off
    if (msecOffset % changeIntervalMsec == 0){
      if ( msecOffset < blinkMsecPrimary[triplet][aPin]){
        // top half off cycle is on
        didChange = fadeToColorPrimary(triplet,aPin,msecOffset);
      } else { 
        // dark 2nd part of cycle
        didChange = fadeToColorSecondary(triplet,aPin,msecOffset);
      }
      if (didChange){
        if (usePWM()){
          writePwmLEDViaLookup(triplet,aPin);
        } else if (usePCA9635()){
          writePCA9635LEDViaLookup(triplet,aPin);
        } else {
          // we're confused
        }
      }
      
    }

}

/*
 * transition to the on value
 * take into account any color changes
 */
boolean fadeToColorPrimary(int triplet,int aPin, int msecOffset){
    boolean didChange = false;
    // lit 1st part of cycle && slow up ramp by changing every other msec
    if ((brightnessCurrent[triplet][aPin] != brightnessPrimary[triplet][aPin]) && ((msecOffset % changeIntervalMsec) == 0)) {
      if (brightnessCurrent[triplet][aPin] < brightnessPrimary[triplet][aPin]){
          brightnessCurrent[triplet][aPin] += transitionIncrement;
          didChange = true;
      } else if (brightnessCurrent[triplet][aPin] > brightnessPrimary[triplet][aPin]){
        brightnessCurrent[triplet][aPin] -= transitionIncrement;
        didChange = true;
      }
    }
    return didChange;
}

/*
 * transiton to the secondary value
 */
boolean fadeToColorSecondary(int triplet,int aPin, int msecOffset){
    boolean didChange = false;
    // slow down ramp by changing every other msec
    if ((brightnessCurrent[triplet][aPin] != brightnessSecondary[triplet][aPin])&& ((msecOffset % changeIntervalMsec) == 0)) {
      if (brightnessCurrent[triplet][aPin] < brightnessSecondary[triplet][aPin]){
          brightnessCurrent[triplet][aPin] += transitionIncrement;
          didChange = true;
      } else if (brightnessCurrent[triplet][aPin] > brightnessSecondary[triplet][aPin]){
        brightnessCurrent[triplet][aPin] -= transitionIncrement;
        didChange = true;
      }
    }
    return didChange;
}

/*==================================================================================
 * I2C dimmer plug support
 *==================================================================================
 */
 
const int PIN_SDA = 4;
const int PIN_SCL = 5;
const int PIN_DIO = 4;   // jee labs naming convention
const int PIN_AIO = 5;   // jee labs naming convention

const  int DIMMER_PLUG_ADDRESS = 0x40;
const  int MODE1 =    0;
const  int MODE2 =    1;
const  int PWM_BASE = 2;  // PWM 0 at base and then add one up to PWM_15
const  int GRPPWM =   18;
const  int GRPFREQ =  19;
const  int LEDOUT0 =  20;
const  int LEDOUT1 =  21;
const  int LEDOUT2 =  22;
const  int LEDOUT3 =  23;

boolean foundPCA9635(){
  int result1 =     writePCA9635ViaI2c(MODE1, 0x00);     // normal
  int result2 =     writePCA9635ViaI2c(MODE1, 0x00);     // normal
  if (result1 == 0 && result2 == 0){
    return true;
  } else {
    return false;
  }
}

boolean usePCA9635(){
  if (numTriplets == NUM_TRIPLETS_DIMMER_PLUG){
    return true;
  } else {
    return false;
  }
}

/**
 * set up i2c and initialize plug if it should be
 */
void initializePCA9635(){
  if (foundPCA9635()){
    numTriplets = NUM_TRIPLETS_DIMMER_PLUG;
    // I2C is slower than direct PWM access so lets not get all fancy on slow transitions
    transitionIncrement = TRANSITION_INCREMENT_I2C;
    // initialize the wire library as master
    writePCA9635ViaI2c(MODE1, 0x00);     // normal
    writePCA9635ViaI2c(MODE2, 0x14);     // 1=on: "inverted" because using sparkfun drivers - totem-pole
    // handle blinking in software
    //writePA9635ViaI2c(GRPPWM, 0x40);  // 25%
    //writePA9635ViaI2c(GRPFREQ, 5);    // 4 blinks per second
    writePCA9635ViaI2c(LEDOUT0, 0xAA);   // LEDs on with individual brightness control
    writePCA9635ViaI2c(LEDOUT1, 0xAA);   // LEDs on with individual brightness control
    writePCA9635ViaI2c(LEDOUT2, 0xAA);   // LEDs on with individual brightness control
    writePCA9635ViaI2c(LEDOUT3, 0xAA);   // LEDs on with individual brightness control
        
    for (int triplet = 0; triplet < numTriplets; triplet++){
      for (int aPin = 0; aPin < 3; aPin++)  {
        // default to min brightness
        brightnessCurrent[triplet][aPin] = brightnessSecondary[triplet][aPin];
        // start them in the initial brightness
        writePCA9635LEDViaLookup(triplet,aPin);
      }
    }
    
  }
}

/**

 * write out the PWM value to an I2C dimmer (Jee Labs dimmer plug)
 * write out the PWM vlaue based on lookup in log table
 */
void writePCA9635LEDViaLookup(int triplet,int pin){
    int brightnessIndex = brightnessCurrent[triplet][pin];    
    byte brightness = (byte)correctedBrightnessTable[brightnessIndex];
    writePCA9635LEDBrightness(i2cPins[triplet][pin],brightness);  
}

/** write a pwm value to an actual pin on the dimmer */
void writePCA9635LEDBrightness( int dimmerPin,  byte value){
  int calculatedAddress = PWM_BASE+dimmerPin;
  writePCA9635ViaI2c(calculatedAddress, value);
}

/**
 * send a byte to the I2C device
 */
byte writePCA9635ViaI2c( int address,  byte i2cData){
  Wire.beginTransmission(DIMMER_PLUG_ADDRESS);
  byte targetRegister = (byte)address;
  Wire.write(targetRegister);
  Wire.write(i2cData);
  // 0 success 
  // 1 data too long to fit in transmit buffer 
  // 2 received NACK on transmit of address 
  // 3 received NACK on transmit of data 
  // 4 other error 
  byte result = Wire.endTransmission();
  return result;
}

/*===========================================================================
 * Sparkfun Addressable LED strip 
 *===========================================================================*/
/*
  Nathan Seidle
  SparkFun Electronics 2011
  
  This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
  
  Controlling an LED strip with individually controllable RGB LEDs. This stuff is awesome.
  
  The SparkFun (individually controllable) RGB strip contains a bunch of WS2801 ICs. These
  are controlled over a simple data and clock setup. The WS2801 is really cool! Each IC has its
  own internal clock so that it can do all the PWM for that specific LED for you. Each IC
  requires 24 bits of 'greyscale' data. This means you can have 256 levels of red, 256 of blue,
  and 256 levels of green for each RGB LED. REALLY granular.
 
  To control the strip, you clock in data continually. Each IC automatically passes the data onto
  the next IC. Once you pause for more than 500us, each IC 'posts' or begins to output the color data
  you just clocked in. So, clock in (24bits * 32LEDs = ) 768 bits, then pause for 500us. Then
  repeat if you wish to display something new.
  
  This example code will display bright red, green, and blue, then 'trickle' random colors down 
  the LED strip.
  
  You will need to connect 5V/Gnd from the Arduino (USB power seems to be sufficient).
  
  For the data pins, please pay attention to the arrow printed on the strip. You will need to connect to
  the end that is the begining of the arrows (data connection)--->
  
  If you have a 4-pin connection:
  Blue = 5V
  Red = SDI
  Green = CKI
  Black = GND
  
  If you have a split 5-pin connection:
  2-pin Red+Black = 5V/GND
  Green = CKI
  Red = SDI
  
  The code was modified to use 32x3 int array instead of long packing
  Could be array of shorts/bytes
 */

// use 2 and 4 because they are not PWM and not the I2C
int SDI = 2; //Red wire (not the red 5V wire!)
int CKI = 4; //Green wire
int ledPin = 13; //On board LED

#define STRIP_LENGTH 32 //32 LEDs on this strip
int stripColors[STRIP_LENGTH][3];

//------------------
// 16  CSS1/HTML3-4/VGA color palette http://en.wikipedia.org/wiki/Web_colors
// black,navy,green,teal,maroon,purple,olive,silver,gray,blue,lime,aqua,red,fuschia,yellow,white
//------------------
int stripColorPalette[16][3] = {
  {0,0,0},{0,0,128},{0,128,0},{0,128,128},
  {128,0,0},{128,0,128},{128,128,0},{192,192,192},
  {128,128,128},{0,0,255},{0,255,0},{0,255,255},
  {255,0,0},{255,0,255},{255,255,0},{255,255,255}
};

void setupLEDStrip() {
  pinMode(SDI, OUTPUT);
  pinMode(CKI, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  //Set default values in array
  for(int x = 0 ; x < STRIP_LENGTH ; x++){
    for (int rgb=0; rgb < 3; rgb++){
      int paletteIndex = x%16;
      stripColors[x][rgb] = stripColorPalette[paletteIndex][rgb];
    }
  }
  postFrame();
}

//Takes the current strip color array and pushes it out
void postFrame (void) {
  //Each LED requires 24 bits of data
  //MSB: R7, R6, R5..., G7, G6..., B7, B6... B0 
  //Once the 24 bits have been delivered, the IC immediately relays next bits to its neighbor
  //Pulling the clock low for 500us or more causes the IC to post the data.

  // do each LED
  for(int ledNumber = 0 ; ledNumber < STRIP_LENGTH ; ledNumber++) {
    for ( int rgbElement = 0; rgbElement < 3; rgbElement++){
      int thisLedColor = stripColors[ledNumber][rgbElement]; //8 bits of r,g or b
  
      // do all 8 bits for each LED color MSB to LSB
      for(byte color_bit = 7 ; color_bit != 255 ; color_bit--) {
        //Feed color bit 7 first (red data MSB)
        
        // move the mask across the color picking off one bit at a time
        int mask = 1 << color_bit;
        
        // it should alrady be low when we get here but make sure
        digitalWrite(CKI, LOW); //Only change data when clock is low
        
        // masks off an individual color bit and write HIGH if not 0
        if(thisLedColor & mask) 
          digitalWrite(SDI, HIGH);
        else
          digitalWrite(SDI, LOW);
    
        digitalWrite(CKI, HIGH); //Data is latched when clock goes high
      }
    }
  }

  //Pull clock low to put strip into reset/post mode
  digitalWrite(CKI, LOW);
  // Lets see if we really need the 500uSec sleep or can we just
  // rely on the fact that we can't get back into this routine in time
  // 19200bps = about 1900cps.  or about 500usec per character
  // Each command is 8 characters for a total of 4msec or 8x500usec
  // So:  No we do not need to pause
  // At 115200bps we're looking at 11,520 cps, 100usec / character
  // Each command is 8 characters so that would be 800usec before
  // we could get back to this spot so still no need to pause
  //delayMicroseconds(500); //Wait for 500us to go into reset
}

/*----------------------------------------------------------------
 * start of strip commands
 *----------------------------------------------------------------
 */

/**
 * accept the command, parse it and update the buffer
 * then resend the whole buffer to the strip
 */
void processStripCommand(char *currentBuffer){
  int stripNumber = convertStripIndexToInt(currentBuffer[2]);
  for ( int i = 0 ; i < 3; i++){
    // calculate the brightness should be '0'-'F'
    int requestedColor = convertAsciiHexToInt(currentBuffer[3+i]);
    // '1' --> 0x11, '2' --> 0x22...
    int color = (requestedColor << 4)+requestedColor;
    stripColors[stripNumber][i] = color;
  }
  postFrame();
}


/**
 * accept the command, parse it and update the buffer
 * then resend the whole buffer to the strip
 */
void processFullStripCommand(char *currentBuffer){
  for (int stripNumber = 0; stripNumber < STRIP_LENGTH; stripNumber++){
      // calculate the brightness should be '0'-'F'
      int colorIndex = convertAsciiHexToInt(currentBuffer[2+stripNumber]);
      for (int rgb =0; rgb < 3; rgb++){
        stripColors[stripNumber][rgb] = stripColorPalette[colorIndex][rgb];
      }
  }
  postFrame();
}

/*==================================================================
 *
 * Example Analog button handler code
 *  rotates cbuffer on sparkfun analog strip on every push
 *
 *==================================================================
 */
 /**
 * returns 1 if button not pressed and 0 if button pressed to short
 */
int binaryAnalogRead(){
  int readValue = analogRead(A5);
  if (readValue<100){
    return 0;
  } else {
    return 1;
  }
}

/**
 * example button action
 * rotates the buffer to look like a chaser
 */
void takeButtonAction(){
    int newButtonValue = binaryAnalogRead();
    if (newButtonValue != lastAnalogButtonRead){
      if (newButtonValue == 0 && lastAnalogButtonRead ==1){
        int lastSlot = STRIP_LENGTH-1;
        int zeroBufferR = stripColors[0][0];
        int zeroBufferG = stripColors[0][1];
        int zeroBufferB = stripColors[0][2];
        for (int stripNumber = 0; stripNumber <lastSlot; stripNumber++){
            for (int rgb =0; rgb < 3; rgb++){
              stripColors[stripNumber][rgb] = stripColors[stripNumber+1][rgb];
            }
        }
        stripColors[lastSlot][0] = zeroBufferR;
        stripColors[lastSlot][1] = zeroBufferG;
        stripColors[lastSlot][2] = zeroBufferB;
        // show it
        postFrame();
      }
      
      lastAnalogButtonRead = newButtonValue;
    }
}


