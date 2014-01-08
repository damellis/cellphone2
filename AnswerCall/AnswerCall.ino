#include <GSM.h> // Use the Arduino GSM file.

GSM gsm(true); // Main gsm object. Pass true to the constructor to enable serial debugging output.
GSMVoiceCall vcs; // Object used to control voice calls.

void setup()
{
  Serial.begin(9600);
  
  pinMode(31, INPUT_PULLUP); // Pin used to answer the call.
  pinMode(13, OUTPUT); // Built-in LED used to indicate call status.
  
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
  // If a call is coming in, rapidly blink the pin 13 LED. If we're on a call, turn the LED on.
  // Otherwise, turn the LED off.
  if (vcs.getvoiceCallStatus() == RECEIVINGCALL) digitalWrite(13, (millis() % 200 < 100) ? HIGH : LOW);
  else if (vcs.getvoiceCallStatus() == TALKING) digitalWrite(13, HIGH);
  else digitalWrite(13, LOW);
  
  // If we're receiving a call and you ground pin 31, answer the call.
  if (vcs.getvoiceCallStatus() == RECEIVINGCALL && digitalRead(31) == LOW) { vcs.answerCall(); delay(500); }
  
  // If we're on a call and you ground pin 31, hang up the call.
  if (vcs.getvoiceCallStatus() == TALKING && digitalRead(31) == LOW) { vcs.hangCall(); delay(500); }
}
