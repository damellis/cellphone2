#ifndef __PhoneBook__
#define __PhoneBook__

#include <GSM3ShieldV1ModemCore.h>
#include <GSM3MobileSMSProvider.h>
#include <GSM3ShieldV1SMSProvider.h>

#define PHONEBOOK_UNKNOWN -1
#define PHONEBOOK_SIM 0
#define PHONEBOOK_MISSEDCALLS 1
#define PHONEBOOK_RECEIVEDCALLS 2
#define PHONEBOOK_DIALEDCALLS 3
#define PHONEBOOK_ALLCALLS 4
#define PHONEBOOK_OWNNUMBERS 5
#define PHONEBOOK_GSMMODULE 6

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
    bool gotNumber;
    char number[20];
    char name[20];
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
