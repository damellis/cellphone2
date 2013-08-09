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

#include <GSM3ClockService.h>
#include <Arduino.h>

#define GETCLOCK ((GSM3_commandType_e) 100)
#define SETCLOCK ((GSM3_commandType_e) 101)

int GSM3ClockService::checkTime()
{
	theGSM3ShieldV1ModemCore.openCommand(this,GETCLOCK);
	checkTimeContinue();
}

int GSM3ClockService::checkTimeContinue()
{
	switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
	case 1:
		theGSM3ShieldV1ModemCore.setCommandCounter(2);
		theGSM3ShieldV1ModemCore.genericCommand_rq(PSTR("AT+CCLK?"));
		break;
	case 2:
		if (parseCCLK()) theGSM3ShieldV1ModemCore.closeCommand(1);
		else theGSM3ShieldV1ModemCore.closeCommand(4);
		break;
	}
}

int GSM3ClockService::setTime(int year, int month, int day, int hour, int minute, int second)
{
	this->datetime.year = year; this->datetime.month = month; this->datetime.day = day;
	this->datetime.hour = hour; this->datetime.minute = minute; this->datetime.second = second;
	theGSM3ShieldV1ModemCore.openCommand(this,SETCLOCK);
	setTimeContinue();
}

int GSM3ClockService::setTimeContinue()
{
	bool resp;
	switch (theGSM3ShieldV1ModemCore.getCommandCounter()) {
		case 1:
			theGSM3ShieldV1ModemCore.setCommandCounter(2);
			theGSM3ShieldV1ModemCore.genericCommand_rq(PSTR("AT+CCLK=\""), false);
			
			if (datetime.year < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(datetime.year);
			theGSM3ShieldV1ModemCore.print("/");
			if (datetime.month < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(datetime.month);
			theGSM3ShieldV1ModemCore.print("/");
			if (datetime.day < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(datetime.day);
			
			theGSM3ShieldV1ModemCore.print(",");
			
			if (datetime.hour < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(datetime.hour);
			theGSM3ShieldV1ModemCore.print(":");
			if (datetime.minute < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(datetime.minute);
			theGSM3ShieldV1ModemCore.print(":");
			if (datetime.second < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(datetime.second);
			
			theGSM3ShieldV1ModemCore.print("+00\"\r"); // ignore timezone for now

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

// "yy/MM/dd,hh:mm:ss±zz"
// +CCLK: "12/10/23,07:30:46-16"
bool GSM3ClockService::parseCCLK()
{
	if (!(theGSM3ShieldV1ModemCore.theBuffer().chopUntil("+CCLK: ", true)))
		return false;
	
	//theGSM3ShieldV1ModemCore.theBuffer().read(); // "
	datetime.year = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil("/", false);
	datetime.month = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().read(); // skip the previous '/'
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil("/", false);
	datetime.day = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",", false);
	datetime.hour = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil(":", false);
	datetime.minute = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().read(); // skip the previous ':'
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil(":", false);
	datetime.second = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil("-", false);
}

void GSM3ClockService::manageResponse(byte from, byte to)
{
	switch(theGSM3ShieldV1ModemCore.getOngoingCommand())
	{
		case NONE:
			theGSM3ShieldV1ModemCore.gss.cb.deleteToTheEnd(from);
			break;
		case GETCLOCK:
			checkTimeContinue();
			break;
		case SETCLOCK:
			setTimeContinue();
			break;
	}
}