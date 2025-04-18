#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH1107_Ext.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <SimpleTimer.h>

// Pin Definitions
#define PN532_SS   13
#define BUTTON_A 32
#define BUTTON_B 14
#define BUTTON_C 15

// Display parameters
#define D_HEIGHT 64
#define D_WIDTH 128
#define D_DELAY 1000

Adafruit_SH1107_Ext display = Adafruit_SH1107_Ext(D_HEIGHT, D_WIDTH, &Wire, -1, 1000000, 1000000);
PN532_SPI pn532spi(SPI, PN532_SS);
PN532 nfc(pn532spi);

#define WRITE_MODE 0
#define READ_MODE 1
int mode = READ_MODE;

#define NUM_TRAIN_COMMANDS 19
#define TRAIN_COMMAND_STR_LENGTH 32
int trainCommandIndex = 0;

SimpleTimer timer;

bool lastAState = HIGH;
bool lastBState = HIGH;
bool lastCState = HIGH;

uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

byte trainCommandTextArray[NUM_TRAIN_COMMANDS][TRAIN_COMMAND_STR_LENGTH] = {
  "", "Long whistles", "Whistle", "Short whistle", "Bell", "Speed 20%",
  "Speed 40%", "Speed 60%", "Speed 80%", "Speed 100%", "Park Trigger 1",
  "Park Trigger 2", "Park", "Station 1", "Station 2", "Station 3",
  "Station 4", "Station 5", "Station 6"
};

void setup() {
  Serial.begin(115200);
  SPI.begin(5, 21, 19); // SCK, MISO, MOSI for ESP32 Feather V2
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1);
  }
  nfc.SAMConfig();

  display.begin(0x3C, true);
  display.setRotation(3);
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  display.centeredDisplay("Read", "Mode", D_DELAY);
  timer.setInterval(30, buttons);
}

void loop() {
  static unsigned long lastReadTime = 0;
  static int lastCommandIndex = -1;

  uint8_t uid[7];
  uint8_t uidLength;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    if (mode == READ_MODE && millis() - lastReadTime > 1000) {
      if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
        Serial.println("Auth failed");
      } else {
        byte buffer[32];
        bool success = true;
        for (int i = 0; i < 2; i++) {
          if (!nfc.mifareclassic_ReadDataBlock(4 + i, &buffer[i * 16])) {
            Serial.println("Read failed");
            success = false;
            break;
          }
        }
        if (success) {
          int commandIndex = tFIMatch(buffer);
          if (commandIndex != lastCommandIndex) {
            commandDisplay(commandIndex);
            lastCommandIndex = commandIndex;
            lastReadTime = millis();
          }
        }
      }
    }
  } else {
    lastCommandIndex = -1;
  }
  timer.run();
  display.timerRun();
}

void RFID() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) return;

  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
    Serial.println("Auth failed");
    return;
  }

  byte buffer[32];
  for (int i = 0; i < 2; i++) {
    if (!nfc.mifareclassic_ReadDataBlock(4 + i, &buffer[i * 16])) {
      Serial.println("Read failed");
      return;
    }
  }

  if (mode == READ_MODE) {
    commandDisplay(tFIMatch(buffer));
  } else {
    for (int i = 0; i < 2; i++) {
      if (!nfc.mifareclassic_WriteDataBlock(4 + i, &trainCommandTextArray[trainCommandIndex][i * 16])) {
        Serial.println("Write failed");
        return;
      }
    }

    byte verifyBuffer[32];
    for (int i = 0; i < 2; i++) {
      if (!nfc.mifareclassic_ReadDataBlock(4 + i, &verifyBuffer[i * 16])) {
        Serial.println("Verify read failed");
        return;
      }
    }

    if (!strcmp((char*)verifyBuffer, (char*)trainCommandTextArray[trainCommandIndex])) {
      display.centeredDisplay("Wrote:", D_DELAY);
      delay(D_DELAY);
      char* command = (char*)trainCommandTextArray[trainCommandIndex];
      char* space = strchr(command, ' ');
      if (space) {
        *space = '\0';
        display.centeredDisplay(command, space + 1, D_DELAY);
        *space = ' '; // restore original string
      } else {
        display.centeredDisplay(command, D_DELAY);
      }
    }
  }
}

void commandDisplay(int command) {
  switch (command) {
    case 0: display.centeredDisplay("", D_DELAY); break;
    case 1: display.centeredDisplay("Long", "whistles", D_DELAY); break;
    case 2: display.centeredDisplay("Whistle", D_DELAY); break;
    case 3: display.centeredDisplay("Short", "whistle", D_DELAY); break;
    case 4: display.centeredDisplay("Bell", D_DELAY); break;
    case 5: display.centeredDisplay("Speed 20%", D_DELAY); break;
    case 6: display.centeredDisplay("Speed 40%", D_DELAY); break;
    case 7: display.centeredDisplay("Speed 60%", D_DELAY); break;
    case 8: display.centeredDisplay("Speed 80%", D_DELAY); break;
    case 9: display.centeredDisplay("Speed 100%", D_DELAY); break;
    case 10: display.centeredDisplay("Park", "trigger 1", D_DELAY); break;
    case 11: display.centeredDisplay("Park", "trigger 2", D_DELAY); break;
    case 12: display.centeredDisplay("Park", D_DELAY); break;
    case 13: display.centeredDisplay("Station 1", D_DELAY); break;
    case 14: display.centeredDisplay("Station 2", D_DELAY); break;
    case 15: display.centeredDisplay("Station 3", D_DELAY); break;
    case 16: display.centeredDisplay("Station 4", D_DELAY); break;
    case 17: display.centeredDisplay("Station 5", D_DELAY); break;
    case 18: display.centeredDisplay("Station 6", D_DELAY); break;
    default: display.centeredDisplay("!No Match!", D_DELAY);
  }
}

void buttons() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 10;

  bool a = digitalRead(BUTTON_A);
  bool b = digitalRead(BUTTON_B);
  bool c = digitalRead(BUTTON_C);

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (mode == WRITE_MODE && a == LOW && lastAState == HIGH) {
            tFIIncrementer(1);
      commandDisplay(trainCommandIndex);
    }
    if (mode == WRITE_MODE && c == LOW && lastCState == HIGH) {
            tFIIncrementer(-1);
      commandDisplay(trainCommandIndex);
    }
    if (b == LOW && lastBState == HIGH) {
            mode = (mode == WRITE_MODE) ? READ_MODE : WRITE_MODE;
      display.centeredDisplay((mode == WRITE_MODE) ? "Write" : "Read", "Mode", D_DELAY);
    }
    lastDebounceTime = millis();
  }

  lastAState = a;
  lastBState = b;
  lastCState = c;
}

void tFIIncrementer(int inc) {
  trainCommandIndex += inc;
  if (trainCommandIndex < 0) trainCommandIndex = NUM_TRAIN_COMMANDS - 1;
  if (trainCommandIndex >= NUM_TRAIN_COMMANDS) trainCommandIndex = 0;
}

int tFIMatch(byte tagText[]) {
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {
    if (!strcmp((char*)tagText, (char*)trainCommandTextArray[i])) {
      return i;
    }
  }
  return NUM_TRAIN_COMMANDS + 1;
}
