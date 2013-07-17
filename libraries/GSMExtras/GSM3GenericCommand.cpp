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