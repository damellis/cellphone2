#include <GSM.h>
#include <GSM3ShieldV1VoiceProvider.h>
#include <GSM3Serial1.h>

#include <PhoneBook.h>
#include <GSM3ClockService.h>
#include <GSM3VolumeService.h>
#include <GSM3DTMF.h>

#include <LedDisplay.h>

#include <Keypad.h>

GSM gsmAccess(true);
GSMVoiceCall vcs(false);
GSM_SMS sms(false);
GSM3ClockService clock;
GSM3VolumeService volume;
GSM3DTMF dtmf;
PhoneBook pb;

int brightness = 15;

unsigned long lastClockCheckTime, lastSMSCheckTime;

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

#define NAME_OR_NUMBER() (name[0] == 0 ? number : name)

int missed = 0;

GSM3_voiceCall_st prevVoiceCallStatus;

enum Mode { NOMODE, TEXTALERT, MISSEDCALLALERT, LOCKED, HOME, DIAL, PHONEBOOK, EDITENTRY, EDITTEXT, MENU, MISSEDCALLS, RECEIVEDCALLS, DIALEDCALLS, TEXTS, SETTIME };
Mode mode = HOME, prevmode, backmode = mode, interruptedmode = mode;
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
char phoneBookNames[NUMPHONEBOOKLINES][15];
char phoneBookNumbers[NUMPHONEBOOKLINES][15];
int phoneBookSize;
int phoneBookIndexStart; // inclusive
int phoneBookIndexEnd; // exclusive
int phoneBookLine;
int phoneBookPage;

long phoneBookCache[256];
int phoneBookCacheSize;

int entryIndex;
char entryName[15], entryNumber[15];
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

      initmode = (mode != prevmode) && !back;
      back = false;
      prevmode = mode;
      
      if (mode == HOME || (mode == LOCKED && unlocking)) {
//        screen.print(clock.getMonth());
//        screen.print("/");
//        screen.print(clock.getDay());
//        screen.print("/");
//        if (clock.getYear() < 10) screen.print('0');
//        screen.print(clock.getYear());
//
//        screen.print(" ");
//        if (clock.getMonth() < 10) screen.print(' ');
//        if (clock.getDay() < 10) screen.print(' ');
//        if (clock.getHour() < 10) screen.print(' ');
        
        screen.print(clock.getHour());
        screen.print(":");
        if (clock.getMinute() < 10) screen.print('0');
        screen.print(clock.getMinute());
      }
      
      if (mode == MISSEDCALLALERT) {
        screen.print(NAME_OR_NUMBER());
        if (missed > 1) screen.print(" + ");
        screen.print(missed - 1);
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
        
//        for (int i = textline * 14; i < textline * 14 + 56; i++) {
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
//          if (strlen(text) > (textline * 14 + 56)) textline++;
//        }
      } else if (mode == LOCKED) {
        if (initmode) {
          unlocking = false;
          blank = false;
        }
        
        if (unlocking) {
          softKeys("Unlock");
          if (key == 'L') { mode = HOME; screen.setBrightness(brightness); unlocking = false; }
//          if (key == 'U') { brightness += 1; screen.setBrightness(brightness / 2); lastKeyPressTime = millis(); }
//          if (key == 'D') { brightness -= 1; screen.setBrightness(brightness / 2); lastKeyPressTime = millis(); }
          if (millis() - lastKeyPressTime > 3000) unlocking = false;
          blank = false;
        } else {
          if (key) {
            screen.setBrightness(brightness / 2);
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
        
        if (key >= '0' && key <= '9') {
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
        }

        for (int i = 0; i < NUMPHONEBOOKLINES; i++) {
          if (strlen(phoneBookNames[i]) == 0) {
            screen.print(phoneBookNumbers[i]);
          } else {
            screen.print(phoneBookNames[i]);
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
          phoneBookLine++;
          if (phoneBookLine == NUMPHONEBOOKLINES) {
            phoneBookPage++;
            phoneBookIndexStart = phoneBookIndexEnd;
            phoneBookIndexEnd = loadphoneBookNamesForwards(phoneBookIndexStart, NUMPHONEBOOKLINES);
            phoneBookLine = 0;
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

      screen.print(NAME_OR_NUMBER());
      softKeys("end");
      
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
      }
      if (strlen(number) == 0) vcs.retrieveCallingNumber(number, sizeof(number));
      if (strlen(number) > 0 && name[0] == 0) {
        phoneNumberToName(number, name, sizeof(name) / sizeof(name[0]));
      }
      screen.print(NAME_OR_NUMBER());
      softKeys("end", "answer");
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
  for (; startingIndex <= phoneBookSize; startingIndex++) {
    pb.readPhoneBookEntry(startingIndex);
    while (!pb.ready());
    if (pb.gotNumber) {
      phoneBookIndices[i] = startingIndex;
      strncpy(phoneBookNames[i], pb.name, 15);
      phoneBookNames[i][14] = 0;
      strncpy(phoneBookNumbers[i], pb.number, 15);
      phoneBookNumbers[i][14] = 0;
      if (pb.getPhoneBookType() != PHONEBOOK_SIM) phoneNumberToName(phoneBookNumbers[i], phoneBookNames[i], 15);
      if (++i == n) break; // found four entries
    }
  }
  for (; i < n; i++) {
    phoneBookIndices[i] = 0;
    phoneBookNames[i][0] = 0;
    phoneBookNumbers[i][0] = 0;
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
      strncpy(phoneBookNames[i], pb.name, 15);
      phoneBookNames[i][14] = 0;
      strncpy(phoneBookNumbers[i], pb.number, 15);
      phoneBookNumbers[i][14] = 0;
      if (pb.getPhoneBookType() != PHONEBOOK_SIM) phoneNumberToName(phoneBookNumbers[i], phoneBookNames[i], 15);
      if (--i == -1) break; // found four entries
    }
  }
  for (; i >= 0; i--) {
    phoneBookIndices[i] = 0;
    phoneBookNames[i][0] = 0;
    phoneBookNumbers[i][0] = 0;
  }
  return endingIndex;
}

void numberInput(char key, char *buf, int len)
{
  scrolling = false;
  screen.showCursor();
  
  screen.print((strlen(buf) < 7) ? buf : (buf + strlen(buf) - 7));
  
  if (key >= '0' && key <= '9') {
    int i = strlen(buf);
    if (i < len - 1) { buf[i] = key; buf[i + 1] = 0; }
  }
  if (key == '*') {
    int i = strlen(buf);
    if (i > 0) { buf[i - 1] = 0; }
  }
}

void textInput(char key, char *buf, int len)
{
  scrolling = false;
  screen.showCursor();
  
  if (millis() - lastKeyPressTime > 1000) {
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
    lastKeyPressTime = 0;
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
//  screen.setCursor(0);
//  screen.print(left);
//  for (int i = 0; i < 8 - strlen(left) - strlen(right); i++) screen.print(" ");
//  screen.print(right);
}

