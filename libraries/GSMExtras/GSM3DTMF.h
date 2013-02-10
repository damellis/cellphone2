#ifndef _GSM3DTMF_
#define _GSM3DTMF_

#include <GSM3GenericCommand.h>

class GSM3DTMF : public GSM3GenericCommand {
public:
  void tone(char);
};

#endif