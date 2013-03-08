/*
  HCMS Display 
 Language: Arduino/Wiring
 
 Displays a string on an Avago HCMS-297x display
 Scrolls the current display string as well.
 Anything you send in the serial port is displayed
 when the Arduino receives a newline or carriage return.
 
 String library based on the Wiring String library:
 http://wiring.org.co/learning/reference/String.html
 
 created 12 Jun. 2008
 modified 11 March 2010
 by Tom Igoe
 
 */
#include <WString.h>
#include <LedDisplay.h>

#define maxStringLength 180  // max string length

// Define pins for the LED display. 
// You can change these, just re-wire your board:
#define dataPin 2              // connects to the display's data in
#define registerSelect 3       // the display's register select pin 
#define clockPin 4             // the display's clock pin
#define enable 5               // the display's chip enable pin
#define reset 6               // the display's reset pin

#define displayLength 8        // number of chars in the display

// create am instance of the LED display:
LedDisplay myDisplay = LedDisplay(dataPin, registerSelect, clockPin, 
enable, reset, displayLength);

int brightness = 15;        // screen brightness

int myDirection = 1;        // direction of scrolling. -1 = left, 1 = right.

String displayString;        // the string currently being displayed
String bufferString;         // the buffer for receiving incoming characters

void setup() {
  Serial.begin(9600);
  // set an initial string to display:
  displayString = "Hello World!";

  // initialize the display library:
  myDisplay.begin();
  // set the display string, speed,and brightness:
  myDisplay.setString(displayString);
  myDisplay.setBrightness(brightness);
}

void loop() {
  // get new data in from the serial port:
  while (Serial.available()>0) {
    // read in new serial:
    getSerial();
  } 
  
  // srcoll left and right:
  if ((myDisplay.getCursor() > 8) ||
    (myDisplay.getCursor() <= -(myDisplay.stringLength()))) {
    myDirection = -myDirection;
    delay(1000);
  }
  myDisplay.scroll(myDirection);
  delay(100);

}

void getSerial() {
  // get a new byte in the serial port:
  int inByte = Serial.read();
  switch (inByte) {
  case '\n':
:
    // if you get a newline,
    // copy the buffer into the displayString:
    displayString = bufferString;
    // set the display with the new string:
    myDisplay.setString(displayString);
    // clear the buffer:
    bufferString = "";
    break;
  default:
    // if you get any ASCII alphanumeric value 
    // (i.e. anything greater than a space), add it to the buffer:
    if (inByte >= ' ') {
      bufferString.append(char(inByte));
    }
    break;
  }
}
