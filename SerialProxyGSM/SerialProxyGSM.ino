#include <GSM3ShieldV1AccessProvider.h>
#include <GSM3ShieldV1ModemCore.h>

GSM3ShieldV1AccessProvider gsm;

void setup()
{
  Serial.begin(9600);
  Serial.print("Initializing GSM module...");
  
  // restart the GSM module.
  // the library will attempt to start the module using pin 7, which is SCK
  // (and not connected to anything except the ISP header)
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);
  delay(12000);
  digitalWrite(19, HIGH);

  // try to initialize the GSM module, retrying on failure
  while (gsm.begin() != GSM_READY) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("done!");
}

void loop()
{
  char c;
  boolean recv = false; // whether we got data from the GSM module
  
  // relay characters from the computer to the GSM module
  while ((c = Serial.read()) != -1)
    theGSM3ShieldV1ModemCore.write(c);
    
  // relay characters from the module to the computer
  while ((c = theGSM3ShieldV1ModemCore.theBuffer().read()) != 0) {
    Serial.write(c);
    recv = true;
  }
  
  // if the receive buffer filled up, the GSM library tells the module
  // to stop sending us data. this tells it to start again.
  if (recv) theGSM3ShieldV1ModemCore.gss.spaceAvailable();
}
