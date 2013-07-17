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

#ifndef __PhoneBook__
#define __PhoneBook__

#include <GSM3ShieldV1ModemCore.h>
#include <GSM3MobileSMSProvider.h>
#include <GSM3ShieldV1SMSProvider.h>

#include <DateTime.h>

#define PHONEBOOK_UNKNOWN -1
#define PHONEBOOK_SIM 0
#define PHONEBOOK_MISSEDCALLS 1
#define PHONEBOOK_RECEIVEDCALLS 2
#define PHONEBOOK_DIALEDCALLS 3
#define PHONEBOOK_ALLCALLS 4
#define PHONEBOOK_OWNNUMBERS 5
#define PHONEBOOK_GSMMODULE 6

#define PHONEBOOK_BUFLEN 20

class PhoneBook : public GSM3ShieldV1BaseProvider
{		
  public:
    PhoneBook();
    		
    void manageResponse(byte from, byte to);
    		
    // Returns 0 if last command is still executing
    // 1 if success
    // >1 if error 
    //int ready(){return GSM3ShieldV1BaseProvider::ready();};
    
    void queryPhoneBook();
    int getPhoneBookType();
    int getPhoneBookSize();
    int getPhoneBookUsed();
    void selectPhoneBook(int type);
  
    void addPhoneBookEntry(char number[], char name[]);
    void writePhoneBookEntry(int index, char number[], char name[]);
    void deletePhoneBookEntry(int index);
    void readPhoneBookEntry(int index);
  
    int phoneBookIndex;
    bool gotNumber, gotTime;
    char number[PHONEBOOK_BUFLEN];
    char name[PHONEBOOK_BUFLEN];
    DateTime datetime;
  private:
    void queryPhoneBookContinue();
    void selectPhoneBookContinue();
    void writePhoneBookContinue();
    void readPhoneBookContinue();
    bool parseCPBR();
    bool parseCPBS();

    int phoneBookType;
    int phoneBookSize;
    int phoneBookUsed;
};
#endif
