//  This file created by joe at freemansoft inc. http://joe.blog.freemansoft.com
//  This program is written for the Arduino Ethernet http://arduino.cc/en/Main/ArduinoBoardEthernet available at microcenter
//  It demonstrates how to combine bonjour and the webduino project to create a web 
//  server that controls a TM1803 3-wire LED strip that I acquired from  http://www.pololu.com/catalog/product/2541
//  The strip used +5V, GND and signal. This program assumes the signal is on 8.   
//  Pololu has written a library available here https://github.com/pololu/pololu-led-strip-arduino
//  Tie the three grounds together, power supply, strip, arduino ethernet.
//  
//  Portions of this file originated from Arduino EthernetBonjour.
//  The updated code is available here https://github.com/neophob/EthernetBonjour
//  The original code is available here http://gkaindl.com
//
//  Portions of this file origintated from the Webduino project https://github.com/sirleech/Webduino
//
//  The web user interface demonstrates the POST API for this service.
// 
//  I had code in that logged via serial port but burned too much memory.

#if defined(ARDUINO) && ARDUINO > 18
#include <SPI.h>
#endif
// raw arduino ethernet
#include <Ethernet.h>
// simpleconfig support using bonjour
#include <EthernetBonjour.h>
#include <WebServer.h>
// TM1803 67us/136us strip
#include <PololuLedStrip.h>

// mac address make something up. odds of collision on your network are low
byte mac[] = { 0x46, 0x52, 0x45, 0x45, 0x4D, 0x4E };

// webduino on top of web server on port 80
// can pick a prefix but I'm not sure how you mention that prefix in the Bonjour library with the text area
// this prefix MUST match the URL in the embedded javascript
#define PREFIX "/"
WebServer webserver(PREFIX, 80);

// Create an ledStrip object on pin 8.
PololuLedStrip<8> ledStrip;
// Create a buffer for holding 60 colors.  Takes 90 bytes.
#define LED_COUNT 30
rgb_color colors[LED_COUNT];

// colors submitted via POST
rgb_color webColor = {0,0,0};
// target led set via POST, -1 means all
#define NO_TARGET -2
#define ALL_TARGET -1
int webTarget = ALL_TARGET;

/* 
 * his command is set as the default command for the server.  It
 * handles both GET and POST requests.  For a GET, it returns a simple
 * page with some buttons.  For a POST, it saves the value posted to
 * the red/green/blue variable, affecting the output of the speaker 
 *
 * this has to go before startup because it is referenced
 */
void stripCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    char * nameNumber;
    do
    {
      /* readPOSTparam returns false when there are no more parameters
       * to read from the input.  We pass in buffers for it to store
       * the name and value strings along with the length of those
       * buffers. */
      repeat = server.readPOSTparam(name, 16, value, 16);


      /* this is a standard string comparison function.  It returns 0
       * when there's an exact match.  We're looking for a parameter
       * named red/green/blue/led here. 
       *
       * These must exactly match the javascript post variable names
       */
      /* use the Ascii to Int function to turn the string
       * version of the color strength value into our unsigned char red/green/blue
       * variable 
       */
      // this exists to support the sliders in the web interface
      if (strcmp(name, "r") == 0) {
        webColor.red = atoi(value);
      } else if (strcmp(name, "g") == 0) {
        webColor.green = atoi(value);
      } else if (strcmp(name, "b") == 0) {
        webColor.blue = atoi(value);
      } else if (strcmp(name, "l") == 0) {
        // note that a LED value of -1 means do all lights
        // can't use NO_TARGET in this type of post
        webTarget = atoi(value);
        if (webTarget < ALL_TARGET || webTarget >= LED_COUNT){
          webTarget = ALL_TARGET;
        } 
      } else 
      // this is the preferred interface if you want to set lots of lights.
      // r0,r1,r2,...,g0,g1,g2,...,b0,g1,g2,...
      if (name[0] == 'r'){
        nameNumber = &name[1];
        webTarget = atoi(nameNumber);
        colors[webTarget].red = atoi(value);
        webTarget = NO_TARGET;
      } else if (name[0] == 'g'){
        nameNumber = &name[1];
        webTarget = atoi(nameNumber);
        colors[webTarget].green = atoi(value);
        webTarget = NO_TARGET;
      } else if (name[0] == 'b'){
        nameNumber = &name[1];
        webTarget = atoi(nameNumber);
        colors[webTarget].blue = atoi(value);
        webTarget = NO_TARGET;
      }
      
    } while (repeat);
    displayColors();
    
    // after procesing the POST data, tell the web browser to reload
    // the page using a GET method. 
    server.httpSeeOther(PREFIX);

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
        "<title>LED Strip Controller</title>\n"
        "<link href='http://ajax.googleapis.com/ajax/libs/jqueryui/1.8.16/themes/base/jquery-ui.css' rel=stylesheet />\n"
        "<script src='http://ajax.googleapis.com/ajax/libs/jquery/1.6.4/jquery.min.js'></script>\n"
        "<script src='http://ajax.googleapis.com/ajax/libs/jqueryui/1.8.16/jquery-ui.min.js'></script>\n"
        "<style> body { background: black; } #r, #g, #b, #l { margin: 10px; } #r { background: #f00; } #g { background: #0f0; } #b { background: #00f; } #l { background: #0ff; }  </style>\n"
        "<script>\n"
      
      // the post path must match the URL above  '/' becomes '/' or whatever
      // change color on mouse up, not while sliding (causes much less traffic to the Arduino):
      //    "function changeRGB(event, ui) { var id = $(this).attr('id'); if (id == 'r') $.post('/', { r: ui.value } ); if (id == 'g') $.post('/', { g: ui.value } ); if (id == 'b') $.post('/', { b: ui.value } );  if (id == 'l') $.post('/', { l: ui.value } );} "
      //    "$(document).ready(function(){ $('#r, #g, #b').slider({min: 0, max:255, change:changeRGB});  $('#l').slider({min: -1, max:29, value: -1, change:changeRGB, slide:changeRGB});});"
      
      // the post path must match the URL above  '/' becomes '/' or whatever
      // change color on slide and mouse up (causes more traffic to the Arduino) but wait 110 before reading:
      // not to DDoS the Arduino, you might have to change the timeout to some threshold value that fits your setup
          "function changeRGB(event, ui) { jQuery.ajaxSetup({timeout: 110});  var id = $(this).attr('id'); if (id == 'r') $.post('/', { r: ui.value } ); if (id == 'g') $.post('/', { g: ui.value } ); if (id == 'b') $.post('/', { b: ui.value } ); if (id == 'l') $.post('/', { l: ui.value } ); } "
          "$(document).ready(function(){ $('#r, #g, #b').slider({min: 0, max:255, change:changeRGB, slide:changeRGB}); $('#l').slider({min: -1, max:29, value: -1, change:changeRGB, slide:changeRGB}); });"
      
        "\n</script>\n"
      "</head>\n"
      "<body style='font-size:62.5%;'>\n"
        "<div id=r></div>"
        "<div id=g></div>"
        "<div id=b></div>"
        "<div id=l></div>"
      "</body>\n"
      "</html>\n";

    server.printP(message);
  }
}

void setup()
{
  // start the ethernet loop
  Ethernet.begin(mac);
  // Initialize the Bonjour/MDNS library. You can now reach or ping this
  // Arduino via the host name "arduino_LED_strip.local", provided that your operating
  // system is Bonjour-enabled (such as MacOS X).
  // Always call this before any other method!
  // esentially the server name in bonjour
  EthernetBonjour.begin("arduino_LED_strip");

  // Now let's register the service we're offering (a web service) via Bonjour!
  // To do so, we call the addServiceRecord() method. The first argument is the
  // name of our service instance and its type, separated by a dot. In this
  // case, the service type is _http. There are many other service types, use
  // google to look up some common ones, but you can also invent your own
  // service type, like _mycoolservice - As long as your clients know what to
  // look for, you're good to go.
  // The second argument is the port on which the service is running. This is
  // port 80 here, the standard HTTP port.
  // The last argument is the protocol type of the service, either TCP or UDP.
  // Of course, our service is a TCP service.
  // With the service registered, it will show up in a Bonjour-enabled web
  // browser. As an example, if you are using Apple's Safari, you will now see
  // the service under Bookmarks -> Bonjour (Provided that you have enabled
  // Bonjour in the "Bookmarks" preferences in Safari).
  EthernetBonjour.addServiceRecord("Arduino Adressable LED Strip._http",
                                   80,
                                   MDNSServiceTCP);
                     
  // register default command on the discoverable url http://a.b.c.d/strip_cmd
  webserver.setDefaultCommand(&stripCmd);
  // start the webduino processing
  webserver.begin();

  // display the initial set of colors
  displayColors();
}

void loop()
{ 
  // This actually runs the Bonjour module. 
  // YOU HAVE TO CALL THIS PERIODICALLY, OR NOTHING WILL WORK! 
  // Preferably, call it once per loop().
  EthernetBonjour.run();
  // start the webduino component.  
  // YOU HAVE TO CALL THIS PERIODICALLY, OR NOTHING WILL WORK! 
  // Preferably, call it once per loop().
  webserver.processConnection();
  // show colors based the current values
  // no need to do this because the handler does it now
  //displayColors();
  
}

void displayColors(){
  if (webTarget == ALL_TARGET){
    // copy the current color into every LED
    for(byte i = 0; i < LED_COUNT; i++)
    {
      colors[i] = webColor;
    }
  } else if ( webTarget == NO_TARGET ){
    // do nothing for assignment
  } else {
    colors[webTarget] = webColor;
  }
  // Write the colors to the LED strip.
  ledStrip.write(colors, LED_COUNT);  
}

