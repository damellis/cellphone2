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
	this->year = year; this->month = month; this->day = day;
	this->hour = hour; this->minute = minute; this->second = second;
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
			
			if (year < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(year);
			theGSM3ShieldV1ModemCore.print("/");
			if (month < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(month);
			theGSM3ShieldV1ModemCore.print("/");
			if (day < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(day);
			
			theGSM3ShieldV1ModemCore.print(",");
			
			if (hour < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(hour);
			theGSM3ShieldV1ModemCore.print(":");
			if (minute < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(minute);
			theGSM3ShieldV1ModemCore.print(":");
			if (second < 10) theGSM3ShieldV1ModemCore.print('0');
			theGSM3ShieldV1ModemCore.print(second);
			
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
	year = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil("/", false);
	month = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().read(); // skip the previous '/'
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil("/", false);
	day = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil(",", false);
	hour = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil(":", false);
	minute = theGSM3ShieldV1ModemCore.theBuffer().readInt();
	theGSM3ShieldV1ModemCore.theBuffer().read(); // skip the previous ':'
	theGSM3ShieldV1ModemCore.theBuffer().chopUntil(":", false);
	second = theGSM3ShieldV1ModemCore.theBuffer().readInt();
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