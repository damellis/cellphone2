#include <GSM.h>
#include <GSM3ShieldV1VoiceProvider.h>
#include <GSM3Serial1.h>

#include <PhoneBook.h>
#include <GSM3ClockService.h>
#include <GSM3VolumeService.h>
#include <GSM3DTMF.h>

#include <LedDisplay.h>

#include <Keypad.h>

#include "pitches.h"

int ringTone[] =          { NOTE_E5, NOTE_D5, NOTE_F4, NOTE_G4, NOTE_C5, NOTE_B5, NOTE_D4, NOTE_E4, NOTE_B5, NOTE_A5, NOTE_C4, NOTE_E4, NOTE_A5, 0 };
int ringToneDurations[] = { 250,     250,     500,     500,     250,     250,     500,     500,     250,     250,     500,     500,     1000,    1000 };
int ringToneIndex;

GSM gsmAccess(true);
GSMVoiceCall vcs(false);
GSM_SMS sms(false);
GSMScanner scannerNetworks;
GSM3ClockService clock;
GSM3VolumeService volume;
GSM3DTMF dtmf;
PhoneBook pb;

int brightness = 15;

#define SCREEN_WIDTH 8
#define ENTRY_SIZE 20

int signalQuality;
unsigned long lastClockCheckTime, lastSMSCheckTime, lastSignalQualityCheckTime, noteStartTime;

// _dataPin, _registerSelect, _clockPin, _chipEnable, _resetPin,  _displayLength
LedDisplay screen = LedDisplay(22, 21, 20, 18, 17, 8);

const byte ROWS = 6;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'!','U','?'},
  {'L','D','R'},
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};  
byte rowPins[ROWS] = {25, 27, 28, 29, 30, 31};
byte colPins[COLS] = {26, 24, 23};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

int pwrkey = 2;

int x = 0, y = 0;

char number[20];
char name[20];

#define NAME_OR_NUMBER() (name[0] == 0 ? (number[0] == 0 ? "Unknown" : number) : name)

int missed = 0;
DateTime missedDateTime;

GSM3_voiceCall_st prevVoiceCallStatus;

enum Mode { NOMODE, TEXTALERT, MISSEDCALLALERT, LOCKED, HOME, DIAL, PHONEBOOK, EDITENTRY, EDITTEXT, MENU, MISSEDCALLS, RECEIVEDCALLS, DIALEDCALLS, TEXTS, SETTIME };
Mode mode = LOCKED, prevmode, backmode = mode, interruptedmode = mode;
boolean initmode, back, fromalert;

struct menuentry_t {
  char *name;
  Mode mode;
  void (*f)();
};

menuentry_t mainmenu[] = {
  { "Missed calls", MISSEDCALLS, 0 },
  { "Received calls", RECEIVEDCALLS, 0 },
  { "Dialed calls", DIALEDCALLS, 0 },
  { "Set date+time", SETTIME, 0 },
};

menuentry_t phoneBookEntryMenu[] = {
  { "Call", PHONEBOOK, callPhoneBookEntry },
  { "Text", EDITTEXT, initTextFromPhoneBookEntry },
  { "Add entry", EDITENTRY, initEditEntry },
  { "Edit", EDITENTRY, initEditEntryFromPhoneBookEntry },
  { "Delete", PHONEBOOK, deletePhoneBookEntry }
};

menuentry_t callLogEntryMenu[] = {
  { "Call", MISSEDCALLS, callPhoneBookEntry },
  { "Save number", EDITENTRY, initEditEntryFromCallLogEntry },
  { "Delete", MISSEDCALLS, deleteCallLogEntry }
};

menuentry_t *menu;

int menuLength;
int menuLine;

const int NUMPHONEBOOKLINES = 1;
int phoneBookIndices[NUMPHONEBOOKLINES];
char phoneBookNames[NUMPHONEBOOKLINES][ENTRY_SIZE];
char phoneBookNumbers[NUMPHONEBOOKLINES][ENTRY_SIZE];
boolean phoneBookHasDateTime[NUMPHONEBOOKLINES];
DateTime phoneBookDateTimes[NUMPHONEBOOKLINES];
int phoneBookSize;
int phoneBookIndexStart; // inclusive
int phoneBookIndexEnd; // exclusive
int phoneBookLine;
int phoneBookPage;

long phoneBookCache[256];
int phoneBookCacheSize;

int entryIndex;
char entryName[ENTRY_SIZE], entryNumber[ENTRY_SIZE];
enum EntryField { NAME, NUMBER };
EntryField entryField;

char text[161];
int textline;

char uppercase[10][10] = { 
  { '.', '?', ',', '\'', '!', '0', 0 },
  { ' ', '1', 0 },
  { 'A', 'B', 'C', '2', 0 },
  { 'D', 'E', 'F', '3', 0 },
  { 'G', 'H', 'I', '4', 0 },
  { 'J', 'K', 'L', '5', 0 },
  { 'M', 'N', 'O', '6', 0 },
  { 'P', 'Q', 'R', 'S', '7', 0 },
  { 'T', 'U', 'V', '8', 0 },
  { 'W', 'X', 'Y', 'Z', '9', 0 },
};

char lowercase[10][10] = { 
  { '.', '?', ',', '\'', '!', '0', 0 },
  { ' ', '1', 0 },
  { 'a', 'b', 'c', '2', 0 },
  { 'd', 'e', 'f', '3', 0 },
  { 'g', 'h', 'i', '4', 0 },
  { 'j', 'k', 'l', '5', 0 },
  { 'm', 'n', 'o', '6', 0 },
  { 'p', 'q', 'r', 's', '7', 0 },
  { 't', 'u', 'v', '8', 0 },
  { 'w', 'x', 'y', 'z', '9', 0 },
};

char (*letters)[10];

char lastKey;
int lastKeyIndex;
unsigned long lastKeyPressTime;
boolean shiftNextKey;

int setTimeField;
int setTimeValues[7]; // month, day, year (tens), year (ones), hour, minute (tens), minute (ones)
int setTimeMin[7] = {  1,  1, 0, 0,  1, 0, 0 };
int setTimeMax[7] = { 12, 31, 9, 9, 24, 9, 9 };
char *setTimeSeparators[7] = { "/", "/", "", " ", ":", "", "" };

unsigned long lastScrollTime = 0;
const long scrollSpeed = 200;

boolean unlocking, blank, scrolling, terminateScreen;

void setup() {
  Serial.begin(9600);

//  // turn on display  
//  pinMode(17, OUTPUT);
//  digitalWrite(17, HIGH);
  
  pinMode(4, OUTPUT);
  
  screen.begin();
  screen.flip();
  screen.clear();
  screen.setCursor(0);
  
  delay(2000);
  
  screen.print("connect");
  screen.display();
  
  delay(2000);
  
  // restart the GSM module.
  // the library will attempt to start the module using pin 7, which is SCK
  // (and not connected to anything except the ISP header)
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);
  delay(12000);
  digitalWrite(19, HIGH);
  
  while (gsmAccess.begin(0, false) != GSM_READY) {
    delay(1000);
  }
  screen.setCursor(0);
  screen.print("connected.");
  screen.display();
  
  vcs.hangCall();
  
  delay(300);
  
  screen.setCursor(0);
  screen.print("caching.");
  screen.display();
  
  cachePhoneBook();
  
  screen.setCursor(0);
  screen.print("done.");
  screen.display();
}

void loop() {
//  if (vcs.getvoiceCallStatus() == IDLE_CALL && mode == LOCKED) screen.setBrightness(0);
//  else screen.setBrightness(brightness);

  scrolling = true;
  
  if (vcs.getvoiceCallStatus() != RECEIVINGCALL) noTone(4);
  
  char key = keypad.getKey();
  //screen.clear();
  screen.setCursor(0);
  screen.hideCursor();
  terminateScreen = true;
  
  if (millis() - lastClockCheckTime > 60000) {
    clock.checkTime();
    while (!clock.ready());
    lastClockCheckTime = millis();
  }
  
  GSM3_voiceCall_st voiceCallStatus = vcs.getvoiceCallStatus();
  switch (voiceCallStatus) {
    case IDLE_CALL:
      if (mode != MISSEDCALLALERT && prevmode != MISSEDCALLALERT && mode != LOCKED && mode != TEXTALERT && missed > 0) {
        interruptedmode = mode;
        mode = MISSEDCALLALERT;
      }
    
      if (mode != TEXTALERT && prevmode != TEXTALERT && mode != LOCKED && mode != MISSEDCALLALERT && millis() - lastSMSCheckTime > 10000) {
        lastSMSCheckTime = millis();
        sms.available();
        while (!sms.ready());
        if (sms.ready() == 1) {
          interruptedmode = mode;
          mode = TEXTALERT;
        }
      }

      if ((mode == HOME || (mode == LOCKED && !unlocking)) && millis() - lastSignalQualityCheckTime > 30000) {
        signalQuality = scannerNetworks.getSignalStrength().toInt();
        lastSignalQualityCheckTime = millis();
      }
  
      initmode = (mode != prevmode) && !back;
      back = false;
      prevmode = mode;
      
      if (mode == HOME || (mode == LOCKED && unlocking)) {
        long bandGap = 0;
        for (int i = 0; i < 100; i++) bandGap += readBandGap();
        int voltage = (1023L * 110 * 10 / (bandGap / 10));
        
        screen.write((signalQuality + 4) / 6 + 26);
        screen.write(constrain((voltage - 330) / 14, 0, 6) + 19);
//        screen.print(voltage);
        screen.print(" ");
        if (clock.getHour() < 10) screen.print(" ");
        screen.print(clock.getHour());
        screen.print(":");
        if (clock.getMinute() < 10) screen.print('0');
        screen.print(clock.getMinute());
      }
      
      if (mode == LOCKED && unlocking) {
        screen.print(" ");
        screen.print(clock.getMonth());
        screen.print("/");
        screen.print(clock.getDay());
        screen.print("/");
        if (clock.getYear() < 10) screen.print('0');
        screen.print(clock.getYear());        
      }
      
      if (mode == MISSEDCALLALERT) {
        screen.print(NAME_OR_NUMBER());
        if (missed > 1) {
          screen.print(" + ");
          screen.print(missed - 1);
        }
        screen.print(" (");
        screen.print(missedDateTime);
        screen.print(")");
        softKeys("close", "call");
        
        if (key == 'L') {
          missed = 0;
          mode = interruptedmode;
        }
        if (key == 'R') {
          missed = 0;
          mode = interruptedmode;
          vcs.voiceCall(number);
          //while (!vcs.ready());
        }
      } else if (mode == TEXTALERT) {
        if (initmode) {
          sms.remoteNumber(number, sizeof(number));
          name[0] = 0;
          int i = 0;
          for (; i < sizeof(text) - 1; i++) {
            int c = sms.read();
            if (!c) break;
            text[i] = c;
          }
          text[i] = 0;
          textline = 0;
          sms.flush(); // XXX: should save to read message store, not delete
        }
        
        if (name[0] == 0) phoneNumberToName(number, name, sizeof(name) / sizeof(name[0]));
        
        screen.print(NAME_OR_NUMBER());
        screen.print(": ");
        screen.print(text);
        
//        for (int i = textline * SCREEN_WIDTH; i < textline * SCREEN_WIDTH + 56; i++) {
//          if (!text[i]) break;
//          screen.print(text[i]);
//        }
        
        softKeys("close", "reply");
        
        if (key == 'L') mode = interruptedmode;
        if (key == 'R') {
          text[0] = 0;
          mode = EDITTEXT;
          fromalert = true;
        }
//        if (key == 'U') {
//          if (textline > 0) textline--;
//        }
//        if (key == 'D') {
//          if (strlen(text) > (textline * SCREEN_WIDTH + 56)) textline++;
//        }
      } else if (mode == LOCKED) {
        if (initmode) {
          unlocking = false;
          blank = false;
        }
        
        if (unlocking) {
          softKeys("Unlock");
          if (key == 'L') { mode = HOME; unlocking = false; }
          if (key == 'U') { brightness += 1; } 
          if (key == 'D') { brightness -= 1; }
          if (key == 'U' || key == 'D') { brightness = constrain(brightness, 0, 15); screen.setBrightness(brightness); lastKeyPressTime = millis(); }
          if (millis() - lastKeyPressTime > 4000) unlocking = false;
          blank = false;
        } else {
          if (key) {
            screen.setBrightness(brightness);
            unlocking = true;
            lastKeyPressTime = millis();
          }
          
          if (!blank) {
            screen.setBrightness(0);
            blank = true;
          }
        }
      } else if (mode == HOME) {
        softKeys("lock", "menu");
        
        if ((key >= '0' && key <= '9') || key == '#') {
          lastKeyPressTime = millis();
          number[0] = key; number[1] = 0;
          mode = DIAL;
        } else if (key == 'L') {
          mode = LOCKED;
        } else if (key == 'R') {
          mode = MENU;
          menu = mainmenu;
          menuLength = sizeof(mainmenu) / sizeof(mainmenu[0]);
          backmode = HOME;
        } else if (key == 'D') {
          mode = PHONEBOOK;
        }
      } else if (mode == DIAL) {
        numberInput(key, number, sizeof(number));
        softKeys("back", "call");
        
        if (key == 'L') {
          mode = HOME;
        } else if (key == 'R') {
          if (strlen(number) > 0) {
            mode = HOME; // for after call ends
            name[0] = 0;
            vcs.voiceCall(number);
            while (!vcs.ready());
          }
        }
      } else if (mode == PHONEBOOK || mode == MISSEDCALLS || mode == RECEIVEDCALLS || mode == DIALEDCALLS) {
        if (initmode) {
          if (mode == PHONEBOOK) pb.selectPhoneBook(PHONEBOOK_SIM);
          if (mode == MISSEDCALLS) pb.selectPhoneBook(PHONEBOOK_MISSEDCALLS);
          if (mode == RECEIVEDCALLS) pb.selectPhoneBook(PHONEBOOK_RECEIVEDCALLS);
          if (mode == DIALEDCALLS) pb.selectPhoneBook(PHONEBOOK_DIALEDCALLS);
          while (!pb.ready());
          delay(300); // otherwise the module gives an error on pb.queryPhoneBook()
          pb.queryPhoneBook();
          while (!pb.ready());
          phoneBookSize = pb.getPhoneBookSize();
          phoneBookPage = 0;
          phoneBookLine = 0;
          phoneBookIndexStart = 1;
          phoneBookIndexEnd = loadphoneBookNamesForwards(phoneBookIndexStart, NUMPHONEBOOKLINES);
          lastKeyPressTime = millis();
        }

        for (int i = 0; i < NUMPHONEBOOKLINES; i++) {
          if (strlen(phoneBookNames[i]) > 0) {
            screen.print(phoneBookNames[i]);
          } else if (strlen(phoneBookNumbers[i]) > 0) {
            screen.print(phoneBookNumbers[i]);
          } else if (phoneBookIndices[i] != 0) {
            screen.print("Unknown");
          }
          if (phoneBookHasDateTime[i] && i == phoneBookLine) {
            screen.print(" ");
            screen.print(phoneBookDateTimes[i]);
          }
        }
        softKeys("back", "okay");
        
        if (key == 'L') mode = HOME;
        else if (key == 'R') {
          backmode = mode;
          if (mode == PHONEBOOK) {
            mode = MENU;
            menu = phoneBookEntryMenu;
            menuLength = sizeof(phoneBookEntryMenu) / sizeof(phoneBookEntryMenu[0]);
          } else {
            mode = MENU;
            menu = callLogEntryMenu;
            menuLength = sizeof(callLogEntryMenu) / sizeof(callLogEntryMenu[0]);
          }
        } else if (key == 'D') {
          if (phoneBookPage * NUMPHONEBOOKLINES + phoneBookLine + 1 < pb.getPhoneBookUsed()) {
            phoneBookLine++;
            if (phoneBookLine == NUMPHONEBOOKLINES) {
              phoneBookPage++;
              phoneBookIndexStart = phoneBookIndexEnd;
              phoneBookIndexEnd = loadphoneBookNamesForwards(phoneBookIndexStart, NUMPHONEBOOKLINES);
              phoneBookLine = 0;
            }
            lastKeyPressTime = millis();
          }
        } else if (key == 'U') {
          if (phoneBookLine > 0 || phoneBookPage > 0) {
            phoneBookLine--;
            if (phoneBookLine == -1) {
              phoneBookPage--;
              phoneBookIndexEnd = phoneBookIndexStart;
              phoneBookIndexStart = loadphoneBookNamesBackwards(phoneBookIndexEnd, NUMPHONEBOOKLINES);
              phoneBookLine = NUMPHONEBOOKLINES - 1;
            }
            lastKeyPressTime = millis();
          }
        }
      } else if (mode == EDITENTRY) {
        if (initmode) entryField = NAME;
        
        if (entryField == NAME) textInput(key, entryName, sizeof(entryName));
        if (entryField == NUMBER) numberInput(key, entryNumber, sizeof(entryNumber));
                
        softKeys("cancel", "save");

        if (entryField == NAME) {
          if (key == 'D') entryField = NUMBER;
        }
        
        if (entryField == NUMBER) {
          if (key == 'U') entryField = NAME;
        }
        
        if (key == 'L') {
          mode = backmode;
          back = true;
        }
        if (key == 'R') {
          savePhoneBookEntry(entryIndex, entryName, entryNumber);
          mode = PHONEBOOK;
        }
      } else if (mode == EDITTEXT) {
        textInput(key, text, sizeof(text));
        softKeys("cancel", "send");
        
        if (key == 'L') {
          mode = (fromalert ? HOME : PHONEBOOK);
          back = true;
        } 
        if (key == 'R') {
          sendText(number, text);
          mode = HOME;
          back = true;
        }
      } else if (mode == MENU) {
        if (initmode) menuLine = 0;
        
        screen.print(menu[menuLine].name);

        softKeys("back", "okay");
        
        if (key == 'U') {
          if (menuLine > 0) menuLine--;
        } else if (key == 'D') {
          if (menuLine < menuLength - 1) menuLine++;
        } else if (key == 'R') {
          mode = menu[menuLine].mode;
          if (menu[menuLine].f) menu[menuLine].f();
        } else if (key == 'L') {
          mode = backmode;
          back = true;
        }
      } else if (mode == TEXTS) {
        softKeys("back");
        
        if (key == 'L') mode = HOME;
      } else if (mode == SETTIME) {
        if (initmode) {
          setTimeValues[0] = clock.getMonth();
          setTimeValues[1] = clock.getDay();
          setTimeValues[2] = clock.getYear() / 10;
          setTimeValues[3] = clock.getYear() % 10;
          setTimeValues[4] = clock.getHour();
          setTimeValues[5] = clock.getMinute() / 10;
          setTimeValues[6] = clock.getMinute() % 10;
          setTimeField = 0;
        }
        
        for (int i = (setTimeField < 4 ? 0 : 4); i < (setTimeField < 4 ? 4 : 7); i++) {
          if (i == setTimeField && millis() % 500 < 250) {
            screen.print(" ");
            if (setTimeValues[i] >= 10) screen.print(" ");
          } else screen.print(setTimeValues[i]);
          screen.print(setTimeSeparators[i]);
        } 
        
        if (setTimeField == 6) softKeys("cancel", "set");
        else softKeys("cancel", "next");
        
        if (key == 'L') mode = HOME;
        
        if (key == 'U') {
          setTimeValues[setTimeField]++;
          if (setTimeValues[setTimeField] > setTimeMax[setTimeField]) setTimeValues[setTimeField] = setTimeMin[setTimeField];
        }
        
        if (key == 'D') {
          setTimeValues[setTimeField]--;
          if (setTimeValues[setTimeField] < setTimeMin[setTimeField]) setTimeValues[setTimeField] = setTimeMax[setTimeField];
        }
        
        if (key == 'R') {
          setTimeField++;
          if (setTimeField == 7) {
            clock.setTime(setTimeValues[2] * 10 + setTimeValues[3], setTimeValues[0], setTimeValues[1],
                          setTimeValues[4], setTimeValues[5] * 10 + setTimeValues[6], 0);
            while (!clock.ready());
            delay(300);
            clock.checkTime();
            while (!clock.ready());
            lastClockCheckTime = millis();
            mode = HOME;
          }
        }
      }
      break;
      
    case CALLING:
      //if (name[0] == 0) phoneNumberToName(number, name, sizeof(name) / sizeof(name[0]));
      screen.print("Calling: ");
      screen.print(NAME_OR_NUMBER());
      softKeys("end");
      
      if (key == 'U' || key == 'D') {
        volume.checkVolume();
        if (checkForCommandReady(volume, 500) && volume.ready() == 1) {
          volume.setVolume(constrain(volume.getVolume() + (key == 'U' ? 5 : -5), 0, 100));
        }
      }
      
      if (key == 'L') {
        vcs.hangCall();
        while (!vcs.ready());
      }
      break;
      
    case RECEIVINGCALL:
      if (prevVoiceCallStatus != RECEIVINGCALL) {
        blank = false;
        screen.setBrightness(brightness);
        missed++;
        name[0] = 0;
        number[0] = 0;
        noteStartTime = 0;
        ringToneIndex = sizeof(ringTone) / sizeof(ringTone[0]) - 1; // index of last note, so we'll continue to the first one
        
      }
      missedDateTime.year = clock.getYear(); missedDateTime.month = clock.getMonth(); missedDateTime.day = clock.getDay();
      missedDateTime.hour = clock.getHour(); missedDateTime.minute = clock.getMinute(); missedDateTime.second = clock.getSecond();
      if (strlen(number) == 0) vcs.retrieveCallingNumber(number, sizeof(number));
      if (strlen(number) > 0 && name[0] == 0) {
        phoneNumberToName(number, name, sizeof(name) / sizeof(name[0]));
      }
      screen.print(NAME_OR_NUMBER());
      softKeys("end", "answer");
      if (millis() - noteStartTime > ringToneDurations[ringToneIndex]) {
        ringToneIndex = (ringToneIndex + 1) % (sizeof(ringTone) / sizeof(ringTone[0]));
        noteStartTime = millis();
        if (ringTone[ringToneIndex] == 0) noTone(4);
        else tone(4, ringTone[ringToneIndex]);
      }
      if (key == 'L') {
        missed--;
        vcs.hangCall();
        while (!vcs.ready());
      }
      if (key == 'R') {
        missed--;
        vcs.answerCall();
        while (!vcs.ready());
      }
      break;
      
    case TALKING:
      screen.print("Connected: ");
      screen.print(NAME_OR_NUMBER());
      softKeys("end");
      
      if ((key >= '0' && key <= '9') || key == '#' || key == '*') dtmf.tone(key);
      
      if (key == 'U' || key == 'D') {
        volume.checkVolume();
        if (checkForCommandReady(volume, 500) && volume.ready() == 1) {
          volume.setVolume(constrain(volume.getVolume() + (key == 'U' ? 5 : -5), 0, 100));
        }
      }
      
      if (key == 'L') {
        vcs.hangCall();
        while (!vcs.ready());
      }
      break;
  }
  
  if (scrolling && millis() - lastScrollTime > scrollSpeed) {
    screen.scroll();
    lastScrollTime = millis();
  }
  
  if (terminateScreen) screen.terminate();
  screen.display(); // blank the rest of the line
  prevVoiceCallStatus = voiceCallStatus;
}

boolean checkForCommandReady(GSM3ShieldV1BaseProvider &provider, int timeout)
{
  unsigned long commandStartTime = millis();
  
  while (millis() - commandStartTime < timeout) {
    if (provider.ready()) return true;
  }
  
  return false;
}

void initEditEntry()
{
  entryIndex = 0;
  entryName[0] = 0;
  entryNumber[0] = 0;
}

void initEditEntryFromCallLogEntry()
{
  entryIndex = 0;
  strcpy(entryName, phoneBookNames[phoneBookLine]);
  strcpy(entryNumber, phoneBookNumbers[phoneBookLine]);
}

void initEditEntryFromPhoneBookEntry()
{
  entryIndex = phoneBookIndices[phoneBookLine];
  strcpy(entryName, phoneBookNames[phoneBookLine]);
  strcpy(entryNumber, phoneBookNumbers[phoneBookLine]);
}

void initTextFromPhoneBookEntry() {
  strcpy(number, phoneBookNumbers[phoneBookLine]);
  text[0] = 0;
  fromalert = false;
}

void sendText(char *number, char *text)
{
  sms.beginSMS(number);
  for (; *text; text++) sms.write(*text);
  sms.endSMS();
  
  while (!sms.ready());
}

void callPhoneBookEntry() {
  vcs.voiceCall(phoneBookNumbers[phoneBookLine]);
  while (!vcs.ready());
  strcpy(number, phoneBookNumbers[phoneBookLine]);
  strcpy(name, phoneBookNames[phoneBookLine]);
}

void deletePhoneBookEntry() {
  pb.deletePhoneBookEntry(phoneBookIndices[phoneBookLine]);
  phoneBookCache[phoneBookIndices[phoneBookLine]] = 0;
}

void deleteCallLogEntry() {
  pb.deletePhoneBookEntry(phoneBookIndices[phoneBookLine]);
}

boolean savePhoneBookEntry(int index, char *name, char *number) {
  pb.selectPhoneBook(PHONEBOOK_SIM);
  while (!pb.ready());
  delay(300); // otherwise the module may give an error when accessing the phonebook.
  if (index == 0) {
    // search for an possible empty phone book entry by looking for a cached hash of 0
    for (int i = 1; i < phoneBookCacheSize; i++) {
      if (!phoneBookCache[i]) {
        pb.readPhoneBookEntry(i);
        while (!pb.ready());
        
        // if the entry is really empty, save the new entry there
        if (!pb.gotNumber) {
          pb.writePhoneBookEntry(i, number, name);
          while (!pb.ready());
          phoneBookCache[i] = hashPhoneNumber(number);
          break;
        }
      }
    }
  } else {
    pb.writePhoneBookEntry(index, number, name);
    while (!pb.ready());
    if (pb.ready() == 1) phoneBookCache[index] = hashPhoneNumber(number);
  }
  
  return true;
}

void cachePhoneBook()
{
  int type;
  
  pb.queryPhoneBook();
  while (!pb.ready());
  type = pb.getPhoneBookType();
  
  if (type != PHONEBOOK_SIM) {
    pb.selectPhoneBook(PHONEBOOK_SIM);
    while (!pb.ready());
    delay(300);
    pb.queryPhoneBook();
    while (!pb.ready());
  }
  
  // the phone book entries start at 1, so the size of the cache is one more than the size of the phone book.  
  phoneBookCacheSize = min(pb.getPhoneBookSize() + 1, sizeof(phoneBookCache) / sizeof(phoneBookCache[0]));
  for (int i = 1; i < phoneBookCacheSize; i++) {
    pb.readPhoneBookEntry(i);
    while (!pb.ready());
    if (pb.gotNumber) {
      phoneBookCache[i] = hashPhoneNumber(pb.number);
    }
  }
  
  if (type != PHONEBOOK_SIM) {
    pb.selectPhoneBook(type);
    while (!pb.ready());
  }
}

long hashPhoneNumber(char *s)
{
  long l = 0;
  boolean firstDigit = true;
  for (; *s; s++) {
    if ((*s) >= '0' && (*s) <= '9') {
      if (firstDigit) {
        firstDigit = false;
        if (*s == '1') continue; // skip U.S. country code
      }
      l = l * 10 + (*s) - '0';
    }
  }
  return l;
}

// return true on success, false on failure.
boolean phoneNumberToName(char *number, char *name, int namelen)
{
  if (number[0] == 0) return false;
  
  long l = hashPhoneNumber(number);
  
  for (int i = 1; i < 256; i++) {
    if (l == phoneBookCache[i]) {
      boolean success = false;
      int type;
      
      pb.queryPhoneBook();
      while (!pb.ready());
      type = pb.getPhoneBookType();
      
      if (type != PHONEBOOK_SIM) {
        pb.selectPhoneBook(PHONEBOOK_SIM);
        while (!pb.ready());
        delay(300);
      }
      
      pb.readPhoneBookEntry(i);
      if (checkForCommandReady(pb, 500) && pb.gotNumber) {
        strncpy(name, pb.name, namelen);
        name[namelen - 1] = 0;
        success = true;
      }
      
      if (type != PHONEBOOK_SIM) {
        pb.selectPhoneBook(type);
        while (!pb.ready());
        delay(300);
      }
      
      return success;
    }
  }
  
  return false;
}

int loadphoneBookNamesForwards(int startingIndex, int n)
{
  int i = 0;
  
  if (pb.getPhoneBookUsed() > 0) {
    for (; startingIndex <= phoneBookSize; startingIndex++) {
      pb.readPhoneBookEntry(startingIndex);
      while (!pb.ready());
      if (pb.gotNumber) {
        phoneBookIndices[i] = startingIndex;
        strncpy(phoneBookNames[i], pb.name, ENTRY_SIZE);
        phoneBookNames[i][ENTRY_SIZE - 1] = 0;
        strncpy(phoneBookNumbers[i], pb.number, ENTRY_SIZE);
        phoneBookNumbers[i][ENTRY_SIZE - 1] = 0;
        phoneBookHasDateTime[i] = pb.gotTime;
        if (pb.gotTime) memcpy(&phoneBookDateTimes[i], &pb.datetime, sizeof(DateTime));
        if (pb.getPhoneBookType() != PHONEBOOK_SIM) {
          phoneBookNames[i][0] = 0; // the names in the call logs are never accurate (but sometimes present)
          phoneNumberToName(phoneBookNumbers[i], phoneBookNames[i], ENTRY_SIZE); // reuses / overwrites pb
        }
        if (++i == n) break; // found a full page of entries
        if (phoneBookPage * NUMPHONEBOOKLINES + i == pb.getPhoneBookUsed()) break; // hit end of the phonebook
      }
    }
  }
  
  for (; i < n; i++) {
    phoneBookIndices[i] = 0;
    phoneBookNames[i][0] = 0;
    phoneBookNumbers[i][0] = 0;
    phoneBookHasDateTime[i] = false;
  }
  
  return startingIndex + 1;
}

int loadphoneBookNamesBackwards(int endingIndex, int n)
{
  int i = n - 1;
  for (endingIndex--; endingIndex >= 0; endingIndex--) {
    pb.readPhoneBookEntry(endingIndex);
    while (!pb.ready());
    if (pb.gotNumber) {
      phoneBookIndices[i] = endingIndex;
      strncpy(phoneBookNames[i], pb.name, ENTRY_SIZE);
      phoneBookNames[i][ENTRY_SIZE - 1] = 0;
      strncpy(phoneBookNumbers[i], pb.number, ENTRY_SIZE);
      phoneBookNumbers[i][ENTRY_SIZE - 1] = 0;
      phoneBookHasDateTime[i] = pb.gotTime;
      if (pb.gotTime) memcpy(&phoneBookDateTimes[i], &pb.datetime, sizeof(DateTime));
      if (pb.getPhoneBookType() != PHONEBOOK_SIM) {
        phoneBookNames[i][0] = 0; // the names in the call logs are never accurate (but sometimes present)
        phoneNumberToName(phoneBookNumbers[i], phoneBookNames[i], ENTRY_SIZE); // reuses / overwrites pb
      }
      if (--i == -1) break; // found four entries
    }
  }
  for (; i >= 0; i--) {
    phoneBookIndices[i] = 0;
    phoneBookNames[i][0] = 0;
    phoneBookNumbers[i][0] = 0;
    phoneBookHasDateTime[i] = false;
  }
  return endingIndex;
}

void numberInput(char key, char *buf, int len)
{
  scrolling = false;
  screen.showCursor();
  
  int i = strlen(buf);
  
  if (i > 0 && (buf[i - 1] == '*' || buf[i - 1] == '#' || buf[i - 1] == '+') && millis() - lastKeyPressTime <= 1000) {
    for (int i = (strlen(buf) < 8) ? 0 : (strlen(buf) - 8); i < strlen(buf); i++) screen.print(buf[i]);
    terminateScreen = false;
    screen.terminate();
    screen.setCursor(screen.getCursor() - 1);
  } else if (millis() - lastKeyPressTime < 5000) {
    screen.print((strlen(buf) < 7) ? buf : (buf + strlen(buf) - 7));
  } else {
    scrolling = true;
    screen.print(buf);
  }
  
  if (key >= '0' && key <= '9') {
    if (i < len - 1) { buf[i] = key; buf[i + 1] = 0; }
    lastKeyPressTime = millis();
  }
  if (key == '*') {
    if (i > 0) { buf[i - 1] = 0; }
    lastKeyPressTime = millis();
  }
  if (key == '#') {
    if (i > 0 && (buf[i - 1] == '*' || buf[i - 1] == '#' || buf[i - 1] == '+') && millis() - lastKeyPressTime <= 1000) {
      lastKeyPressTime = millis();
      if (buf[i - 1] == '#') buf[i - 1] = '*';
      else if (buf[i - 1] == '*') buf[i - 1] = '+';
      else if (buf[i - 1] == '+') buf[i - 1] = '#';
    } else {
      if (i < len - 1) { buf[i] = '#'; buf[i + 1] = 0; lastKeyPressTime = millis(); }
    }
  }
}

void textInput(char key, char *buf, int len)
{
  scrolling = false;
  screen.showCursor();
  
  if (millis() - lastKeyPressTime > 5000) {
    scrolling = true;
    screen.print(buf);
  } else if (millis() - lastKeyPressTime > 1000) {
    screen.print((strlen(buf) < 7) ? buf : (buf + strlen(buf) - 7));
  } else {
    for (int i = (strlen(buf) < 8) ? 0 : (strlen(buf) - 8); i < strlen(buf); i++) screen.print(buf[i]);
    terminateScreen = false;
    screen.terminate();
    screen.setCursor(screen.getCursor() - 1);
  }
  
  if (key >= '0' && key <= '9') {
    if (millis() - lastKeyPressTime > 1000 || key - '0' != lastKey) {
      // append new letter
      lastKeyIndex = 0;
      lastKey = key - '0';
      int i = strlen(buf);
      
      if (i == 0) letters = uppercase;
      else {
        letters = lowercase;
        for (int j = i - 1; j >= 0; j--) {
          if (buf[j] == '.' || buf[j] == '?' || buf[j] == '!') {
            letters = uppercase;
            break;
          } else if (buf[j] != ' ') break;
        }
      }
      
      if (shiftNextKey) {
        if (letters == uppercase) letters = lowercase;
        else letters = uppercase;
        
        shiftNextKey = false;
      }
      
      if (i < len - 1) { buf[i] = letters[lastKey][lastKeyIndex]; buf[i + 1] = 0; }
    } else {
      // cycle previously entered letter
      lastKeyIndex++;
      if (letters[lastKey][lastKeyIndex] == 0) lastKeyIndex = 0; // wrap around
      int i = strlen(buf);
      if (i > 0) { buf[i - 1] = letters[lastKey][lastKeyIndex]; }
    }
    lastKeyPressTime = millis();
  }
  if (key == '*') {
    int i = strlen(buf);
    if (i > 0) { buf[i - 1] = 0; }
    lastKeyPressTime = millis() - 1000;
    shiftNextKey = false;
  }
  if (key == '#') shiftNextKey = true;
}

void softKeys(char *left)
{
  softKeys(left, "");
}

void softKeys(char *left, char *right)
{
//  screen.setCursor(0, 40);
//  screen.setTextColor(WHITE, BLACK);
//  screen.print(left);
//  screen.setTextColor(BLACK);
//  for (int i = 0; i < SCREEN_WIDTH - strlen(left) - strlen(right); i++) screen.print(" ");
//  screen.setTextColor(WHITE, BLACK);
//  screen.print(right);
//  screen.display();
}

int readBandGap()
{
  uint8_t low, high;
  
  ADMUX = B01011110; // AVCC reference, right-adjust result, 1.1V input
  
  ADCSRA |= (1 << ADSC);
  
  // ADSC is cleared when the conversion finishes
  while (bit_is_set(ADCSRA, ADSC));
  
  // we have to read ADCL first; doing so locks both ADCL
  // and ADCH until ADCH is read.  reading ADCL second would
  // cause the results of each conversion to be discarded,
  // as ADCL and ADCH would be locked when it completed.
  low  = ADCL;
  high = ADCH;
  
  // combine the two bytes
  return (high << 8) | low;
}
