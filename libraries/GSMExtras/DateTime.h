#ifndef DateTime_h
#define DateTime_h

#include <Printable.h>

struct DateTime : public Printable {
  int second;         /* seconds */
  int minute;         /* minutes */
  int hour;        /* hours */
  int day;        /* day of the month */
  int month;         /* month */
  int year;        /* year */
  
  virtual size_t printTo(Print& p) const {
    int n = 0;
    
    n += p.print(hour);
    n += p.print(":");
    if (minute < 10) n += p.print("0");
    n += p.print(minute);
    n += p.print(" ");
    n += p.print(month);
    n += p.print("/");
    n += p.print(day);
    n += p.print("/");
    if (year < 10) n += p.print("0");
    n += p.print(year);
    
    return n;
  }
};

#endif