#include <GSM.h>
#include <GSM3ShieldV1VoiceProvider.h>
#include <GSM3Serial1.h>

#include <PhoneBook.h>
#include <GSM3ClockService.h>

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#include <Keypad.h>

GSM gsmAccess(true);
GSMVoiceCall vcs(false);
GSM_SMS sms(false);
GSM3ClockService clock;
PhoneBook pb;

unsigned long lastClockCheckTime, lastSMSCheckTime;

Adafruit_PCD8544 screen = Adafruit_PCD8544(16, 15, 14, 12, 13); // SCLK, DIN, D/C, CS, RST 

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

boolean missed = false;

enum Mode { NOMODE, TEXTALERT, LOCKED, HOME, DIAL, PHONEBOOK, EDITENTRY, EDITTEXT, MENU, MISSEDCALLS, RECEIVEDCALLS, DIALEDCALLS, TEXTS, SETTIME };
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
  { "Set time", SETTIME, 0 },
};

menuentry_t phoneBookEntryMenu[] = {
  { "Call", PHONEBOOK, callPhoneBookEntry },
  { "Text", EDITTEXT, initTextFromPhoneBookEntry },
  { "Edit", EDITENTRY, initEditEntryFromPhoneBookEntry },
  { "Delete", PHONEBOOK, deletePhoneBookEntry }
};

menuentry_t callLogEntryMenu[] = {
  { "Call", MISSEDCALLS, callPhoneBookEntry },
  { "Delete", MISSEDCALLS, deletePhoneBookEntry }
};

menuentry_t *menu;

int menuLength;
int menuLine;

const int NUMPHONEBOOKLINES = 5;
int phoneBookIndices[NUMPHONEBOOKLINES];
char phoneBookNames[NUMPHONEBOOKLINES][15];
char phoneBookNumbers[NUMPHONEBOOKLINES][15];
int phoneBookSize;
int phoneBookIndexStart; // inclusive
int phoneBookIndexEnd; // exclusive
int phoneBookLine;
int phoneBookPage;
int phoneBookFirstPageOffset;

#define PHONEBOOKENTRY() ((phoneBookPage == 0) ? phoneBookLine - phoneBookFirstPageOffset : phoneBookLine)

int entryIndex;
char entryName[15], entryNumber[15];
enum EntryField { NAME, NUMBER };
EntryField entryField;

char text[161];
int textline;

char letters[10][10] = { 
  { '.', '?', ',', '0', 0 },
  { ' ', '1', 0 },
  { 'A', 'B', 'C', 'a', 'b', 'c', '2', 0 },
  { 'D', 'E', 'F', 'd', 'e', 'f', '3', 0 },
  { 'G', 'H', 'I', 'g', 'h', 'i', '4', 0 },
  { 'J', 'K', 'L', 'j', 'k', 'l', '5', 0 },
  { 'M', 'N', 'O', 'm', 'n', 'o', '6', 0 },
  { 'P', 'Q', 'R', 'S', 'p', 'q', 'r', 's', '7', 0 },
  { 'T', 'U', 'V', 't', 'u', 'v', '8', 0 },
  { 'W', 'X', 'Y', 'Z', 'w', 'x', 'y', 'z', '9', 0 },
};

char lastKey;
int lastKeyIndex;
unsigned long lastKeyPressTime;

int hour, minute;
enum SetTimeField { HOUR, TENS, ONES };
SetTimeField setTimeField;

boolean unlocking, blank;

void setup() {
  Serial.begin(9600);

  // turn on display  
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
  
  screen.begin();
  screen.setContrast(35);
  screen.clearDisplay();
  screen.setCursor(0,0);
  screen.display();
  
  delay(2000);
  
  screen.println("connecting...");
  screen.display();
  while (gsmAccess.begin() != GSM_READY) {
    delay(1000);
  }
  screen.println("connected.");
  screen.display();
  
  vcs.hangCall();
  
  delay(300);  
}

void loop() {
  if (vcs.getvoiceCallStatus() == IDLE_CALL && mode == LOCKED) digitalWrite(17, LOW);
  else digitalWrite(17, HIGH);
  
  char key = keypad.getKey();
  screen.clearDisplay();
  screen.setCursor(0, 0);

  if (millis() - lastClockCheckTime > 60000) {
    clock.checkTime();
    while (!clock.ready());
    lastClockCheckTime = millis();
  }
  
  screen.setTextColor(BLACK);
  
  switch (vcs.getvoiceCallStatus()) {
    case IDLE_CALL:
      if (mode != TEXTALERT && prevmode != TEXTALERT && mode != LOCKED && millis() - lastSMSCheckTime > 10000) {
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
        screen.print("    ");
        if (clock.getHour() < 10) screen.print(' ');
        
        screen.setTextColor(WHITE, BLACK);
        screen.print(clock.getHour());
        screen.print(":");
        if (clock.getMinute() < 10) screen.print('0');
        screen.print(clock.getMinute());
        
        screen.setTextColor(BLACK);
        screen.print("     ");
      }
      
      if (mode == TEXTALERT) {
        if (initmode) {
          sms.remoteNumber(number, sizeof(number));
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
        
        screen.print(number);
        screen.println(":");
        
        for (int i = textline * 14; i < textline * 14 + 56; i++) {
          if (!text[i]) break;
          screen.print(text[i]);
        }
        
        softKeys("close", "reply");
        
        if (key == 'L') mode = interruptedmode;
        if (key == 'R') {
          text[0] = 0;
          mode = EDITTEXT;
          fromalert = true;
        }
        if (key == 'U') {
          if (textline > 0) textline--;
        }
        if (key == 'D') {
          if (strlen(text) > (textline * 14 + 56)) textline++;
        }
      } else if (mode == LOCKED) {
        if (initmode) {
          unlocking = false;
          blank = false;
        }
        
        if (unlocking) {
          softKeys("Unlock");
          if (key == 'L') { mode = HOME; unlocking = false; }
          if (millis() - lastKeyPressTime > 3000) unlocking = false;
          blank = false;
        } else {
          if (key) {
            unlocking = true;
            lastKeyPressTime = millis();
          }
          
          if (!blank) {
            screen.display(); // since there's no call to softKeys()
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
          delay(300);
          phoneBookSize = pb.getPhoneBookSize();
          phoneBookPage = 0;
          phoneBookLine = 0;
          phoneBookIndexStart = 1;
          if (mode == PHONEBOOK) phoneBookFirstPageOffset = 1;
          else phoneBookFirstPageOffset = 0;
          phoneBookIndexEnd = loadphoneBookNamesForwards(phoneBookIndexStart, NUMPHONEBOOKLINES - phoneBookFirstPageOffset);
        }

        if (mode == PHONEBOOK && phoneBookPage == 0) {
          if (phoneBookLine == 0) screen.setTextColor(WHITE, BLACK);
          else screen.setTextColor(BLACK);
          screen.println("Add entry.");
        }
        for (int i = 0; i < NUMPHONEBOOKLINES - (phoneBookPage == 0 ? phoneBookFirstPageOffset : 0); i++) {
          if (i == phoneBookLine - (phoneBookPage == 0 ? phoneBookFirstPageOffset : 0)) screen.setTextColor(WHITE, BLACK);
          else screen.setTextColor(BLACK);
          if (strlen(phoneBookNames[i]) == 0) {
            screen.print(phoneBookNumbers[i]);
            if (strlen(phoneBookNumbers[i]) < 14) screen.println();
          } else {
            screen.print(phoneBookNames[i]);
            if (strlen(phoneBookNames[i]) < 14) screen.println();
          }
        }
        softKeys("back", "okay");
        
        if (key == 'L') mode = HOME;
        else if (key == 'R') {
          if (mode == PHONEBOOK && phoneBookPage == 0 && phoneBookLine == 0) {
            entryIndex = 0; entryName[0] = 0; entryNumber[0] = 0;
            mode = EDITENTRY;
          } else {
            if (mode == PHONEBOOK) {
              mode = MENU;
              menu = phoneBookEntryMenu;
              menuLength = sizeof(phoneBookEntryMenu) / sizeof(phoneBookEntryMenu[0]);
              backmode = PHONEBOOK;
            } else {
              mode = MENU;
              menu = callLogEntryMenu;
              menuLength = sizeof(callLogEntryMenu) / sizeof(callLogEntryMenu[0]);
              backmode = mode;
            }
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
              phoneBookIndexStart = loadphoneBookNamesBackwards(phoneBookIndexEnd, NUMPHONEBOOKLINES - (phoneBookPage == 0 ? phoneBookFirstPageOffset : 0));
              phoneBookLine = NUMPHONEBOOKLINES - 1;
            }
          }
        }
      } else if (mode == EDITENTRY) {
        if (initmode) entryField = NAME;
        
        screen.println("Name:");
        if (entryField != NAME) screen.println(entryName);
        else textInput(key, entryName, sizeof(entryName));
        
        screen.println("Number:");
        if (entryField != NUMBER) screen.println(entryNumber);
        else numberInput(key, entryNumber, sizeof(entryNumber));
                
        softKeys("cancel", "save");

        if (entryField == NAME) {
          if (key == 'D') entryField = NUMBER;
        }
        
        if (entryField == NUMBER) {
          if (key == 'U') entryField = NAME;
        }
        
        if (key == 'L') {
          mode = PHONEBOOK;
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

        for (int i = 0; i < menuLength; i++) {
          if (menuLine == i) screen.setTextColor(WHITE, BLACK);
          else screen.setTextColor(BLACK);
          screen.print(menu[i].name);
          if (strlen(menu[i].name) % 14 != 0) screen.println();
        }
        
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
          hour = clock.getHour();
          minute = clock.getMinute();
          setTimeField = HOUR;
        }
        
        screen.println("Time:");
        
        if (setTimeField == HOUR) screen.setTextColor(WHITE, BLACK);
        if (hour < 10) screen.print(" ");
        screen.print(hour);        
        screen.setTextColor(BLACK);
        
        screen.print(":");
        
        if (setTimeField == TENS) screen.setTextColor(WHITE, BLACK);
        screen.print(minute / 10);
        screen.setTextColor(BLACK);
        
        if (setTimeField == ONES) screen.setTextColor(WHITE, BLACK);
        screen.print(minute % 10);
        screen.setTextColor(BLACK);
        
        softKeys("back", "okay");
        
        if (key == 'L') mode = HOME;
        
        if (setTimeField == HOUR) {
          if (key == 'U') hour = (hour + 1) % 24;
          if (key == 'D') hour = (hour + 23) % 24;
          if (key == 'R') setTimeField = TENS;
        } else if (setTimeField == TENS) {
          if (key == 'U') minute = (minute + 10) % 60;
          if (key == 'D') minute = (minute + 50) % 60;
          if (key == 'R') setTimeField = ONES;
        } else if (setTimeField == ONES) {
          if (key == 'U') minute = (minute / 10) * 10 + (minute % 10 + 1) % 10;
          if (key == 'D') minute = (minute / 10) * 10 + (minute % 10 + 9) % 10;
          if (key == 'R') {
            clock.setTime(clock.getYear(), clock.getMonth(), clock.getDay(), hour, minute, 0);
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
      screen.println("Calling:");
      screen.print(number);
      softKeys("end");
      
      if (key == 'L') {
        vcs.hangCall();
        while (!vcs.ready());
      }
      break;
      
    case RECEIVINGCALL:
      blank = false;
      vcs.retrieveCallingNumber(number, sizeof(number));
      screen.println("incoming:");
      screen.print(number);
      softKeys("end", "answer");
      if (key == 'L') {
        vcs.hangCall();
        while (!vcs.ready());
      }
      if (key == 'R') {
        vcs.answerCall();
        while (!vcs.ready());
      }
      break;
      
    case TALKING:
      screen.println("Connected:");
      screen.print(number);
      softKeys("end");
      
      if (key == 'L') {
        vcs.hangCall();
        while (!vcs.ready());
      }
      break;
  }
}

void initEditEntryFromPhoneBookEntry()
{
  entryIndex = phoneBookIndices[PHONEBOOKENTRY()];
  strcpy(entryName, phoneBookNames[PHONEBOOKENTRY()]);
  strcpy(entryNumber, phoneBookNumbers[PHONEBOOKENTRY()]);
}

void initTextFromPhoneBookEntry() {
  strcpy(number, phoneBookNumbers[PHONEBOOKENTRY()]);
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
  vcs.voiceCall(phoneBookNumbers[PHONEBOOKENTRY()]);
  while (!vcs.ready());
  strcpy(number, phoneBookNumbers[PHONEBOOKENTRY()]);
}

void deletePhoneBookEntry() {
  pb.deletePhoneBookEntry(phoneBookIndices[PHONEBOOKENTRY()]);
}

boolean savePhoneBookEntry(int index, char *name, char *number) {
//  // new entry; find available index
//  if (index == 0) {
//    for (index = 1; index < 256; index++) {
//      pb.readPhoneBookEntry(index);
//      while (!pb.ready());
//      if (!pb.gotNumber) break;
//    }
//    
//    if (index == 256) return false;
//  }
  if (index == 0) pb.addPhoneBookEntry(number, name);
  else pb.writePhoneBookEntry(index, number, name);
  while (!pb.ready());
  
  return true;
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
  screen.print(buf);
  screen.setTextColor(WHITE, BLACK);
  screen.print(" ");
  screen.setTextColor(BLACK);
  
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
  if (millis() - lastKeyPressTime > 1000) {
    screen.print(buf);
    screen.setTextColor(WHITE, BLACK);
    screen.print(" ");
    screen.setTextColor(BLACK);
  } else {
    for (int i = 0; i < strlen(buf) - 1; i++) screen.print(buf[i]);
    screen.setTextColor(WHITE, BLACK);
    screen.print(buf[strlen(buf) - 1]);
    screen.setTextColor(BLACK);
  }
  screen.println();
  if (key >= '0' && key <= '9') {
    if (millis() - lastKeyPressTime > 1000 || key - '0' != lastKey) {
      // append new letter
      lastKeyIndex = 0;
      lastKey = key - '0';
      int i = strlen(buf);
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
  }
}

void softKeys(char *left)
{
  softKeys(left, "");
}

void softKeys(char *left, char *right)
{
  screen.setCursor(0, 40);
  screen.setTextColor(WHITE, BLACK);
  screen.print(left);
  screen.setTextColor(BLACK);
  for (int i = 0; i < 14 - strlen(left) - strlen(right); i++) screen.print(" ");
  screen.setTextColor(WHITE, BLACK);
  screen.print(right);
  screen.display();
}

