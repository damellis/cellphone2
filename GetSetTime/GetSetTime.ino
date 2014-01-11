#include <GSM.h>
#include <GSM3ClockService.h>

GSM gsm(true); // Main gsm object. Pass true to the constructor to enable serial debugging output.
GSM3ClockService clock;

void setup()
{
  Serial.begin(9600);
  
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);
  delay(12000);
  digitalWrite(19, HIGH);
  
  while(gsm.begin(0, false) != GSM_READY);
  
  // set the time to 10 am on January 11, 2014
  clock.setTime(14, 1, 11, 10, 0, 0);
}

void loop()
{
  clock.checkTime();
  while (!clock.ready());
  Serial.print(clock.getMonth());
  Serial.print("/");
  Serial.print(clock.getDay());
  Serial.print("/20");
  Serial.print(clock.getYear());
  Serial.print(" ");
  Serial.print(clock.getHour());
  Serial.print(":");
  if (clock.getMinute() < 10) Serial.print("0");
  Serial.print(clock.getMinute());
  Serial.print(":");
  if (clock.getSecond() < 10) Serial.print("0");
  Serial.print(clock.getSecond());
  Serial.println();
  
  delay(1000);
}
