#ifndef _GSM3GENERICCOMMAND_
#define _GSM3GENERICCOMMAND_

#include <GSM3ShieldV1ModemCore.h>

class GSM3GenericCommand : public GSM3ShieldV1BaseProvider {
public:
  void manageResponse(byte from, byte to);
protected:
  void openGenericCommand();
private:
  int genericCommandContinue();
};

#endif