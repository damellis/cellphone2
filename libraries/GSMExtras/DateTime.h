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
	
  bool operator==(DateTime rhs) const {
    return second == rhs.second && minute == rhs.minute && hour == rhs.hour &&
           day == rhs.day && month == rhs.month && year == rhs.year;
  }
  
  bool operator!=(DateTime rhs) const {
    return !(*this == rhs);
  }
  
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