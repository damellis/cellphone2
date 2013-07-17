/*

(C) Copyright 2012-2013 Massachusetts Institute of Technology

This file is part of the GSMExtras library.

The GSMExtras library is free software: you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 2.1 of the
License, or (at your option) any later version.

The GSMExtras library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GSMExtras library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <GSM3VolumeService.h>

#define GETVOLUME ((GSM3_commandType_e) 100)
#define SETVOLUME ((GSM3_commandType_e) 101)

void GSM3VolumeService::checkVolume()
{
  theGSM3ShieldV1ModemCore.openCommand(this,GETVOLUME);
  checkVolumeContinue();
}

int GSM3VolumeService::checkVolumeContinue()
{
  switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
    case 1:
      theGSM3ShieldV1ModemCore.setCommandCounter(2);
      theGSM3ShieldV1ModemCore.genericCommand_rq(PSTR("AT+CLVL?"));
      break;
    case 2:
      if (parseCLVL()) theGSM3ShieldV1ModemCore.closeCommand(1);
      else theGSM3ShieldV1ModemCore.closeCommand(4);
      break;
  }
}

void GSM3VolumeService::setVolume(int volume)
{
  this->volume = volume;
  theGSM3ShieldV1ModemCore.openCommand(this,SETVOLUME);
  setVolumeContinue();
}

int GSM3VolumeService::setVolumeContinue()
{
  bool resp;
  switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
    case 1:
      theGSM3ShieldV1ModemCore.setCommandCounter(2);
      theGSM3ShieldV1ModemCore.genericCommand_rq(PSTR("AT+CLVL="), false);
      theGSM3ShieldV1ModemCore.print(volume);
      theGSM3ShieldV1ModemCore.print("\r");      
      break;
    case 2:
      if(theGSM3ShieldV1ModemCore.genericParse_rsp(resp))
      {
        if (resp) theGSM3ShieldV1ModemCore.closeCommand(1);
        else theGSM3ShieldV1ModemCore.closeCommand(3);
      }
      break;
  }
}

void GSM3VolumeService::manageResponse(byte from, byte to)
{
  switch(theGSM3ShieldV1ModemCore.getOngoingCommand())
  {
    case NONE:
      theGSM3ShieldV1ModemCore.gss.cb.deleteToTheEnd(from);
      break;
    case GETVOLUME:
      checkVolumeContinue();
      break;
    case SETVOLUME:
      setVolumeContinue();
      break;
  }
}

bool GSM3VolumeService::parseCLVL()
{
  // should be "+CLVL: " but readInt() skips the first character
  // remaining in the buffer.
  if (!(theGSM3ShieldV1ModemCore.theBuffer().chopUntil("+CLVL:", true)))
    return false;
  
  volume = theGSM3ShieldV1ModemCore.theBuffer().readInt();
  return true;
}
