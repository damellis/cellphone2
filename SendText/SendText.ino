#include <GSM.h> // Use the Arduino GSM file.

GSM gsm(true); // Main gsm object. Pass true to the constructor to enable serial debugging output.
GSM_SMS sms;

char recipient[] = "XXXXXXXX"; // fill-in phone number of recipient here.

void setup()
{
  Serial.begin(9600);
  
  pinMode(31, INPUT_PULLUP); // Pin used to send the text message.
  pinMode(13, OUTPUT); // Built-in LED used to indicate status.
  
  digitalWrite(13, HIGH); // Turn on the LED while trying to connect.
  
  Serial.print("connecting...");
  
  // These lines start the GSM module, which is connected to pin 19 on the board.
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);
  delay(12000);
  digitalWrite(19, HIGH);  

  // Initialize the GSM library. Passing false as the second argument to the constructor
  // tells the library not to start up the GSM module, since we've done it already.  
  while (gsm.begin(0, false) != GSM_READY); // For more information, enable debugging above.
  
  Serial.println("done.");
  
  digitalWrite(13, LOW); // Turn off the LED once we've connected.
}

void loop()
{
  if (digitalRead(31) == LOW) {
    sms.beginSMS(recipient);
    sms.print("Hello.");
    sms.endSMS();
    
    // blink the pin 13 LED
    for (int i = 0; i < 5; i++) {
      digitalWrite(13, HIGH);
      delay(50);
      digitalWrite(13, LOW);
      delay(50);
    }
  }
}
