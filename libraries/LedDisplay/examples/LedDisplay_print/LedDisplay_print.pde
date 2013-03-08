/*
  HCMS Display 
 Language: Arduino/Wiring
 
 Displays a string on an Avago HCMS-297x display
 
 String library based on the Wiring String library:
 http://wiring.org.co/learning/reference/String.html
 
 created 12 Jun. 2008
 modified 11 March 2010
 by Tom Igoe
 
 */
#include <LedDisplay.h>

// Define pins for the LED display. 
// You can change these, just re-wire your board:
#define dataPin 2              // connects to the display's data in
#define registerSelect 3       // the display's register select pin 
#define clockPin 4             // the display's clock pin
#define enable 5               // the display's chip enable pin
#define reset 6               // the display's reset pin

#define displayLength 8        // number of characters in the display

// create am instance of the LED display library:
LedDisplay myDisplay = LedDisplay(dataPin, registerSelect, clockPin, 
enable, reset, displayLength);

int brightness = 15;        // screen brightness

void setup() {
  // initialize the display library:
  myDisplay.begin();
  // set the brightness of the display:
  myDisplay.setBrightness(brightness);
}

void loop() {
  // set the cursor to 0:
  myDisplay.home();
  // print the millis:
  myDisplay.print("ms:");
  myDisplay.print(millis());
}
