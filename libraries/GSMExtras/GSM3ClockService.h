#ifndef _GSM3CLOCKSERVICE_
#define _GSM3CLOCKSERVICE_

#include <GSM3ShieldV1ModemCore.h>

class GSM3ClockService : public GSM3ShieldV1BaseProvider
{
public:
	void manageResponse(byte from, byte to);
	int checkTime();
	int setTime(int year, int month, int day, int hour, int minute, int second);
	int getYear() { return year; }
	int getMonth() { return month; }
	int getDay() { return day; }
	int getHour() { return hour; }
	int getMinute() { return minute; }
	int getSecond() { return second; }
	
private:
	int year, month, day, hour, minute, second;
	bool parseCCLK();
	int checkTimeContinue();
	int setTimeContinue();
};

#endif