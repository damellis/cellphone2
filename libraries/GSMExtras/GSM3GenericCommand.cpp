#include <GSM3GenericCommand.h>

#define GENERICCOMMAND ((GSM3_commandType_e) 100)

void GSM3GenericCommand::openGenericCommand()
{
  theGSM3ShieldV1ModemCore.openCommand(this,GENERICCOMMAND);
  genericCommandContinue();
}

int GSM3GenericCommand::genericCommandContinue()
{
  bool resp;
  switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
    case 1:
      if(theGSM3ShieldV1ModemCore.genericParse_rsp(resp))
      {
        if (resp) theGSM3ShieldV1ModemCore.closeCommand(1);
        else theGSM3ShieldV1ModemCore.closeCommand(3);
      }
      break;
  }
}

void GSM3GenericCommand::manageResponse(byte from, byte to)
{
  switch(theGSM3ShieldV1ModemCore.getOngoingCommand())
  {
    case NONE:
      theGSM3ShieldV1ModemCore.gss.cb.deleteToTheEnd(from);
      break;
    case GENERICCOMMAND:
      genericCommandContinue();
      break;
  }
}