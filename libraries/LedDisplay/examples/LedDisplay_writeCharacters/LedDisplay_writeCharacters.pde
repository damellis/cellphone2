/*
  HCMS Display 
 Language: Arduino/Wiring
 
 Writes characters on an Avago HCMS-297x display
 
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

#define displayLength 8        // number of bytes needed to pad the string

// create am instance of the LED display:
LedDisplay myDisplay = LedDisplay(dataPin, registerSelect, clockPin, 
enable, reset, displayLength);

int brightness = 15;        // screen brightness
char myString[] = {
  'p','r','i','n','t','i','n','g'};
void setup() {
  Serial.begin(9600);

  // initialize the display library:
  myDisplay.begin();
  myDisplay.setString("Printing");
  myDisplay.home();
  myDisplay.setBrightness(brightness);
}

void loop() {

  for (int thisPosition = 0; thisPosition < 8; thisPosition++) {
    for (int thisChar = ' '; thisChar < 'z'; thisChar++) {
      myDisplay.write(thisChar);
      myDisplay.setCursor(thisPosition);
      delay(3);
    }
    myDisplay.write(myString[thisPosition]);
    delay(10);
  }
  delay(500);
  myDisplay.clear();
  myDisplay.home();
}
