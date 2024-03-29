//  This file created by joe at freemansoft inc. http://joe.blog.freemansoft.com
//  This program is written for the Arduino Ethernet http://arduino.cc/en/Main/ArduinoBoardEthernet an Arduino with a built in ethernet shield
//  Note:
//  This board uses the following pins
//      0 RX when programming/debugging
//      1 TX when programming/debugging
//      2 W5100 interrupt when bridged
//      4 SD Card SS (if not using Nokia 5110)
//      3 optional Nokia 5110 CLK
//      4 optional Nokia 5110 DIn
//      5 optional Nokia 5110 D/C
//      6 NeopPixel Sheild (default pin)
//      7 optional Nokia 5110 Rst Reset (6 and 7 are twisted on LCD breakout board)
//      8 optional Nokia 5110 SCE CE Chip Enable (6 and 7 are twisted on LCD breakout board)
//     10 SPI Wiznet SS
//     11 SPI
//     12 SPI
//     13 SPI
//  It demonstrates a web server that controls an Adafruit Neopixel shield and lcdDisplay IP on a Nokia 5110
//
//  Serial.println() crashes the server
//  Web Server functionality originated from the Webduino project https://github.com/sirleech/Webduino
//  Nokia 5110 style LCD control code migrated to Adafruit https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
//  Adafruit NeoPixel shield libraries came from https://github.com/adafruit/Adafruit_NeoPixel
//  Bonjour and MDNS seems to crash the server or at least really break the Neopixel panel. It could be a memory issue 2022/12
//
//  See http://arduino.cc/en/Guide/Libraries on how to import the necessary libraries
//
//  The web user interface demonstrates the POST API for this service.
//
//  I had code in that logged via serial port but burned too much memory. (428 free vs 248 free ~ 180 bytes )
//
//  API: Form POST can post any number of form elements in a single ost with the following conventions
//    r=<value>  set all RED LEDs to this value
//    g=<value>  set all GREEN LEDs to this value
//    b=<value>  set all BLUDE LEDs to this value
//    r#=<value> set RED LED number '#' to this value
//    g#=<value> set GREEN LED number '#' to this value
//    b#=<value> set BLUE LED number '#' to this value
//    s#=<value> set Nokia 5100 LCD text on line # to this value. Rows start as 0
//    c=<value>  clear Nokia 5100 LCD value is ignored

#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PCD8544.h>

// #include <MDNS_Generic.h>

EthernetUDP udp;
// MDNS mdns(udp);

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
// Adafruit_PCD8544 lcdDisplay = Adafruit_PCD8544(7, 6, 5, 4, 3);

// We're using this because of pin limitations - Ethernet on SPI and Neopixel on Pin 6
// pin 3 - Serial Clock (CLK) (SCLK)
// pin 4 - Serial Data out (DIN) (SDIN)
// pin 5 - Data/Command (D/C)
// pin 8 - LCD (CS) (SCE)
// pin 7 - LCD (RST)
Adafruit_PCD8544 lcdDisplay = Adafruit_PCD8544(3, 4, 5, 8, 7);
// Every lcdDisplay seems to be different :-()
#define PCD8544_CONTRAST 60
// 8 pixel high plus blank line
#define PCD8544_LINE_HEIGHT 9
// 5 + a blank line of pixels
#define PCD8544_LINE_CHAR_WIDTH 6
// Adafruit_PCD8544.LCDWIDTH / PCD8544_LINE_CHAR_WIDTH --> 84/6 = 14 characters
// Adafruit_PCD8544.LCDHEIGHT / PCD8544_LINE_height --> 48/9 = 5 lines

#define PCD8544_BLANK_LINE F("              ")

// mac address make something up. odds of collision on your network are low
byte mac[] = {0x46, 0x52, 0x45, 0x8B, 0x4B, 0xED};
// shortened the name to 12 characters so it fits on the 5110 LCD.
// This is the same value in dhcp.h because it is hardcoded there
#define HOST_NAME F("arduino")
// This should be the host name in theory but it doesn't seem to work on my local network
// #define DNS_NAME "arduino_8b4bed"
// #define MDNS_SERVICE "arduino_8b4bed._http"

// webduino on top of web server on port 80
// this prefix MUST match the URL in the embedded javascript
#define PREFIX "/"
WebServer webserver(PREFIX, 80);

// pin used to talk to NeoPixel
#define NEO_PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
// using the arduino shield which is 5x8
const int NUM_PIXELS = 40;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

typedef struct
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} pixelDefinition;
// should these be 8x5 intead of linear 40 ?
volatile pixelDefinition lightStatus[NUM_PIXELS];

/*
 * this command is set as the default command for the server.  It
 * handles both GET and POST requests.  For a GET, it returns a simple
 * page with some buttons.  For a POST, it saves the value posted to
 * the red/green/blue variable, and then sends array to the NeoPixel
 *
 * this has to go before startup because it is referenced
 */
void stripCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[5];   // shortened all our post variable names to 3 digits including the LED #
    char value[33]; // maximum LCD string is 32 characters plus \0
    char *nameNumber;
    bool hadError = false;
    do
    {
      /* readPOSTparam returns false when there are no more parameters
       * to read from the input.  We pass in buffers for it to store
       * the name and value strings along with the length of those
       * buffers. */
      repeat = server.readPOSTparam(name, 5, value, 33);

      /* We're looking for a parameter named "r", "g" or "b" here.
       * The character comparison saved 40 bytes
       *
       * These must exactly match the javascript post variable names
       */
      /* use the Ascii to Int function to turn the string
       *  color strength value into our unsigned char red/green/blue variable
       *
       * Non integer value behavior will cause problems
       */
      if (name[0] == 'c')
      {
        // clear lcdDisplay
        lcdDisplay.clearDisplay();
        lcdDisplay.display();
      }
      else if (name[0] == 's' && name[1] != '\0')
      {
        // figure out the row the text goes on
        nameNumber = &name[1];
        int row = atoi(nameNumber);
        // blank the row
        lcdDisplay.setCursor(0, row * PCD8544_LINE_HEIGHT);
        lcdDisplay.print(PCD8544_BLANK_LINE);
        // then draw the row
        lcdDisplay.setCursor(0, row * PCD8544_LINE_HEIGHT);
        lcdDisplay.print(value);
        lcdDisplay.display();
      }
      else
      {
        // RGB control section
        // We setup buffer could process them as they are received and then lcdDisplay at end

        // support the sliders in the web interface
        // support the 'r' 'g' 'b' comands without LED numbers
        // modify all LEDs at once
        // assumes using utf-8 or ASCII
        uint8_t valueMasked = atoi(value) & 0xff;
        ;
        if (name[1] == '\0')
        {
          for (int i = 0; i < NUM_PIXELS; i++)
          {
            if (name[0] == 'r')
            {
              lightStatus[i].red = valueMasked;
            }
            else if (name[0] == 'g')
            {
              lightStatus[i].green = valueMasked;
            }
            else if (name[0] == 'b')
            {
              lightStatus[i].blue = valueMasked;
            }
          }
        }
        else
        {
          // this is the preferred interface if you want to set lots of lights.
          // r0,r1,r2,...,g0,g1,g2,...,b0,g1,g2,...
          // r0=xx&r1=yy&g0=zz
          nameNumber = &name[1];
          int webTarget = atoi(nameNumber);
          if (webTarget < NUM_PIXELS)
          {
            if (name[0] == 'r')
            {
              lightStatus[webTarget].red = valueMasked;
            }
            else if (name[0] == 'g')
            {
              lightStatus[webTarget].green = valueMasked;
            }
            else if (name[0] == 'b')
            {
              lightStatus[webTarget].blue = valueMasked;
            }
          }
          else
          {
            hadError = true;
          }
        }
      }

    } while (repeat);
    // lcdDisplay colors after all post params processed

    showColors();

    // after procesing the POST data, tell the web browser to reload
    // the page using a GET method.
    // server.httpSeeOther(PREFIX);
    // or we could just return success
    if (hadError)
    {
      server.httpFail();
    }
    else
    {
      server.httpSuccess();
    }

    return;
  }

  /* for a GET or HEAD, send the standard "it's all OK headers" */
  server.httpSuccess();

  /* we don't output the body for a HEAD request */
  if (type == WebServer::GET)
  {
    /* store the HTML in program memory using the P macro */
    P(message) =
        "<!DOCTYPE html><html><head>\n"
        "<title>Arduino LED</title>\n"
        "<link  href='https://code.jquery.com/ui/1.13.2/themes/base/jquery-ui.css' rel=stylesheet />"
        "<script src='https://code.jquery.com/jquery-2.2.4.min.js' integrity='sha256-BbhdlvQf/xTY9gja0Dq3HiwQF8LaCRTXxZKRutelT44=' crossorigin='anonymous'></script>"
        "<script src='https://code.jquery.com/ui/1.13.2/jquery-ui.min.js' integrity='sha256-lSjKY0/srUM9BE3dPm+c4fBo1dky2v27Gdjm2uoZaL0=' crossorigin='anonymous'></script>"

        "<style> body { background: black; } #r, #g, #b { margin: 10px; } #r { background: #f00; } #g { background: #0f0; } #b { background: #00f; } #text { color: white;}</style>\n"
        "<script>"

        // the post path must match the URL above  '/' becomes '/' or whatever

        // change color on mouse up, not while sliding (causes much less traffic to the Arduino):
        "function changeRGB(event, ui) { var id = $(this).attr('id'); if (id == 'r') $.post('/', { r: ui.value } ); if (id == 'g') $.post('/', { g: ui.value } ); if (id == 'b') $.post('/', { b: ui.value } ); } "
        "$(document).ready(function(){ $('#r, #g, #b').slider({min: 0, max:255, change:changeRGB}); });"
        // TODO: This may be broken after jquery upgrades
        // change color on slide and mouse up (causes more traffic to the Arduino)
        // wait 100msec before reading to reduce traffic Arduino, you might have to change the timeout to some threshold value that fits your setup
        // "function changeRGB(event, ui) { jQuery.ajaxSetup({timeout: 100});  var id = $(this).attr('id'); if (id == 'r') $.post('/', { r: ui.value } ); if (id == 'g') $.post('/', { g: ui.value } ); if (id == 'b') $.post('/', { b: ui.value } ); } "
        // "$(document).ready(function(){ $('#r, #g, #b').slider({min: 0, max:255, change:changeRGB, slide:changeRGB}); });"

        "</script>\n"
        "</head>\n"
        "<body>"
        "<div id=text>Color changes on mouse-up</div>"
        "<div id=r></div>"
        "<div id=g></div>"
        "<div id=b></div>"
        "</body>\n"
        "</html>\n";

    server.printP(message);
  }
}

void setup()
{
  //Serial.begin(9600);

  strip.begin();
  initializeStartupColors();

  lcdDisplay.begin();
  lcdDisplay.setContrast(PCD8544_CONTRAST);
  lcdDisplay.setTextSize(1);
  lcdDisplay.display();

  // start the ethernet loop
  Ethernet.begin(mac);
  // mdns.begin(Ethernet.localIP(), DNS_NAME);
  // mdns.addServiceRecord(MDNS_SERVICE, 80, MDNSServiceTCP);

  // register default command on the discoverable url http://a.b.c.d/strip_cmd
  webserver.setDefaultCommand(&stripCmd);

  // start the webduino processing
  webserver.begin();

  lcdDisplay.clearDisplay();
  lcdDisplay.setCursor(0, 0);
  lcdDisplay.println(F("Host Name"));
  lcdDisplay.setCursor(0, 1 * PCD8544_LINE_HEIGHT);
  lcdDisplay.println(HOST_NAME);
  lcdDisplay.setCursor(0, 3 * PCD8544_LINE_HEIGHT);
  lcdDisplay.println(F("Listening on"));
  lcdDisplay.setCursor(0, 4 * PCD8544_LINE_HEIGHT);

  char buf[4];
  for (byte thisByte = 0; thisByte < 4; thisByte++)
  {
    if (thisByte != 0)
    {
      lcdDisplay.print(F("."));
      // Serial.print(".");
    }
    // print the value of each byte of the IP address:
    itoa(Ethernet.localIP()[thisByte], buf, 10);
    lcdDisplay.print(buf);
    // Serial.print(buf);
  }
  // Serial.print("\n");
  lcdDisplay.display();

  // strip.begin();
  initializeStartupColors();

  ;
}

void loop()
{
  // This actually runs the mDNS module. YOU HAVE TO CALL THIS PERIODICALLY,
  // OR NOTHING WILL WORK! Preferably, call it once per loop().
  // mdns.run();  // start the webduino component.
  // YOU HAVE TO CALL THIS PERIODICALLY, OR NOTHING WILL WORK!
  // Preferably, call it once per loop().
  webserver.processConnection();
}

/*
 * blank out the LEDs and buffer
 */
void initializeStartupColors()
{
  strip.rainbow();
  strip.show();
  // should sleep here
  strip.clear();
  strip.show();
}

/*
 * lcdDisplay the color in the light status buffer
 */
void showColors()
{
  uint16_t i;
  for (i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, lightStatus[i].red, lightStatus[i].green, lightStatus[i].blue);
  }
  strip.show();
}
