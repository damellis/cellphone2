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

#ifndef _GSM3DTMF_
#define _GSM3DTMF_

#include <GSM3GenericCommand.h>

class GSM3DTMF : public GSM3GenericCommand {
public:
  void tone(char);
  void localTone(char);
};

#endif