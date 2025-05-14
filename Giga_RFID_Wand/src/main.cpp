// Refactored RFID Sketch with Unified Display Logic and Improvements
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_PN532.h>
#include <SimpleTimer.h>

// Pin Definitions
#define PN532_SS 13
#define BUTTON_A 32
#define BUTTON_B 14
#define BUTTON_C 15

#define D_DELAY 750

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
Adafruit_PN532 nfc(PN532_SS, &SPI);

// Enums for clarity
enum Mode { WRITE_MODE = 0, READ_MODE = 1 };
Mode mode = READ_MODE;

#define NUM_TRAIN_COMMANDS 19
#define TRAIN_COMMAND_STR_LENGTH 32
int trainCommandIndex = 0;

SimpleTimer timer;
bool lastAState = HIGH, lastBState = HIGH, lastCState = HIGH;
bool firstCommandSelected = false;

uint8_t keya[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
const char trainCommandTextArray[NUM_TRAIN_COMMANDS][TRAIN_COMMAND_STR_LENGTH] = {
  "", "Long whistles", "Whistle", "Short whistle", "Bell", "Speed 20%",
  "Speed 40%", "Speed 60%", "Speed 80%", "Speed 100%", "Park Trigger 1",
  "Park Trigger 2", "Park", "Station 1", "Station 2", "Station 3",
  "Station 4", "Station 5", "Station 6"
};

void displayCenteredText(const char* text, int delayTime);
void displayCenteredTextTwoLines(const char* line1, const char* line2, int delayTime);
void smartDisplay(const char* text);
void commandDisplay(int command);
void tFIIncrementer(int inc);
int tFIMatch(byte tagText[]);
void buttons();
void RFID();

void setup() {
  Serial.begin(115200);
  SPI.begin();
  nfc.begin();

  if (!nfc.getFirmwareVersion()) {
    Serial.println("Didn't find PN53x board");
    while (1);
  }
  nfc.SAMConfig();

  if (!display.begin(0x3C, true)) {
    Serial.println("SH1107 allocation failed");
    while (1);
  }
  display.setRotation(3);
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  smartDisplay("Read Mode");
  timer.setInterval(30, buttons);
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    if (mode == READ_MODE) {
      if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
        smartDisplay("Auth failed");
        return;
      }

      byte buffer[32];
      bool success = true;
      for (int i = 0; i < 2; i++) {
        if (!nfc.mifareclassic_ReadDataBlock(4 + i, &buffer[i * 16])) {
          smartDisplay("Read failed");
          success = false;
          break;
        }
      }
      if (success) {
        int commandIndex = tFIMatch(buffer);
        if (commandIndex >= 0) commandDisplay(commandIndex);
      }
    } else if (mode == WRITE_MODE) {
      RFID();
    }
  }

  timer.run();
}

void RFID() {
  uint8_t uid[7];
  uint8_t uidLength;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) return;
  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
    smartDisplay("Auth failed");
    return;
  }

  for (int i = 0; i < 2; i++) {
    uint8_t blockData[16];
    memset(blockData, 0, 16);
    memcpy(blockData, &trainCommandTextArray[trainCommandIndex][i * 16], 16);
    if (!nfc.mifareclassic_WriteDataBlock(4 + i, blockData)) {
      smartDisplay("Write failed");
      return;
    }
  }

  smartDisplay("Wrote:");
  smartDisplay(trainCommandTextArray[trainCommandIndex]);
}

void displayCenteredText(const char* text, int delayTime) {
  display.clearDisplay();
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (display.width() - w) / 2;
  int y = (display.height() - h) / 2;
  display.setCursor(x, y);
  display.print(text);
  display.display();
  delay(delayTime);
}

void displayCenteredTextTwoLines(const char* line1, const char* line2, int delayTime) {
  display.clearDisplay();
  display.setTextSize(2);
  int16_t x1, y1, x2, y2;
  uint16_t w1, h1, w2, h2;
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
  display.getTextBounds(line2, 0, 0, &x2, &y2, &w2, &h2);
  const int lineSpacing = 2;
  int totalHeight = h1 + h2 + lineSpacing;
  int y1pos = (display.height() - totalHeight) / 2;
  int y2pos = y1pos + h1 + lineSpacing;
  display.setCursor((display.width() - w1) / 2, y1pos);
  display.print(line1);
  display.setCursor((display.width() - w2) / 2, y2pos);
  display.print(line2);
  display.display();
  delay(delayTime);
}

void smartDisplay(const char* text) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(2);
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  if (w <= display.width()) {
    displayCenteredText(text, D_DELAY);
  } else {
    char buffer[TRAIN_COMMAND_STR_LENGTH];
    strncpy(buffer, text, TRAIN_COMMAND_STR_LENGTH);
    char* spacePos = strchr(buffer, ' ');
    if (spacePos != NULL) {
      *spacePos = '\0';
      displayCenteredTextTwoLines(buffer, spacePos + 1, D_DELAY);
    } else {
      displayCenteredText(text, D_DELAY);
    }
  }
}

void commandDisplay(int command) {
  if (command >= 0 && command < NUM_TRAIN_COMMANDS) {
    smartDisplay(trainCommandTextArray[command]);
  } else {
    smartDisplay("!No Match!");
  }
}

void tFIIncrementer(int inc) {
  trainCommandIndex += inc;
  if (trainCommandIndex < 1) trainCommandIndex = NUM_TRAIN_COMMANDS - 1;
  if (trainCommandIndex >= NUM_TRAIN_COMMANDS) trainCommandIndex = 1;
}

int tFIMatch(byte tagText[]) {
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {
    if (!strcmp((char*)tagText, trainCommandTextArray[i])) {
      return i;
    }
  }
  return -1;
}

void buttons() {
  bool a = digitalRead(BUTTON_A);
  bool b = digitalRead(BUTTON_B);
  bool c = digitalRead(BUTTON_C);
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (a == LOW && lastAState == HIGH) {
      mode = (mode == WRITE_MODE) ? READ_MODE : WRITE_MODE;
      smartDisplay((mode == WRITE_MODE) ? "Write Mode" : "Read Mode");
      firstCommandSelected = false;
    }
    if (mode == WRITE_MODE) {
      if (c == LOW && lastCState == HIGH) {
        if (!firstCommandSelected) {
          trainCommandIndex = 1;
          firstCommandSelected = true;
        } else {
          tFIIncrementer(1);
        }
        commandDisplay(trainCommandIndex);
      }
      if (b == LOW && lastBState == HIGH) {
        if (!firstCommandSelected) {
          trainCommandIndex = NUM_TRAIN_COMMANDS - 1;
          firstCommandSelected = true;
        } else {
          tFIIncrementer(-1);
        }
        commandDisplay(trainCommandIndex);
      }
    }
    lastDebounceTime = millis();
  }
  lastAState = a;
  lastBState = b;
  lastCState = c;
}
