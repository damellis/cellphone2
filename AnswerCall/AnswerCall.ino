#include <GSM.h>

GSM gsm(true);
GSMVoiceCall vcs;

void setup()
{
  Serial.begin(9600);
  
  pinMode(31, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  
  digitalWrite(13, HIGH);
  
  Serial.print("connecting...");
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);
  delay(12000);
  digitalWrite(19, HIGH);  
  while (gsm.begin(0, false) != GSM_READY);
  Serial.println("done.");
  
  digitalWrite(13, LOW);  
}

void loop()
{
  if (vcs.getvoiceCallStatus() == RECEIVINGCALL) digitalWrite(13, (millis() % 200 < 100) ? HIGH : LOW);
  else if (vcs.getvoiceCallStatus() == TALKING) digitalWrite(13, HIGH);
  else digitalWrite(13, LOW);
  
  if (vcs.getvoiceCallStatus() == RECEIVINGCALL && digitalRead(31) == LOW) { vcs.answerCall(); delay(500); }
  if (vcs.getvoiceCallStatus() == TALKING && digitalRead(31) == LOW) { vcs.hangCall(); delay(500); }
}
