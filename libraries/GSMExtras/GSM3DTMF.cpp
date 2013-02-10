#include <GSM3DTMF.h>

void GSM3DTMF::tone(char c)
{
  openGenericCommand();
  theGSM3ShieldV1ModemCore.genericCommand_rq(PSTR("AT+VTS="), false);
  theGSM3ShieldV1ModemCore.print(c);
  theGSM3ShieldV1ModemCore.print("\r");
}