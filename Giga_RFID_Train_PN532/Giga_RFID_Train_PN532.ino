#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <SimpleTimer.h>

#define PN532_SS 13
SimpleTimer timer;

#define NUM_TRAIN_COMMANDS 19
#define TRAIN_COMMAND_STR_LENGTH 32
#define MAX_COMMAND_LENGTH 30

PN532_SPI pn532spi(SPI, PN532_SS);
PN532 nfc(pn532spi);

uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

byte trainCommandTextArray[NUM_TRAIN_COMMANDS][TRAIN_COMMAND_STR_LENGTH] = {
  "", "Long whistles", "Whistle", "Short whistle", "Bell", "Speed 20%",
  "Speed 40%", "Speed 60%", "Speed 80%", "Speed 100%", "Park Trigger 1",
  "Park Trigger 2", "Park", "Station 1", "Station 2", "Station 3",
  "Station 4", "Station 5", "Station 6"
};

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial1.begin(115200);

  SPI.begin(5, 21, 19);  // SCK, MISO, MOSI for Feather ESP32 V2
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1);
  }
  nfc.SAMConfig();
}

void loop() {
  timer.run();
  static unsigned long lastReadTime = 0;
  static int lastMatch = -1;

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    if (millis() - lastReadTime > 1000) {
      if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 7, 0, keya)) {
                return;
      }
      
      byte buffer[32];
      for (int i = 0; i < 2; i++) {
        if (!nfc.mifareclassic_ReadDataBlock(4 + i, &buffer[i * 16])) {
          Serial.println("Read failed");
          return;
        }
      }

            

      int match = tFIMatch(buffer);
      if (match != lastMatch) {
        char cleanCommand[TRAIN_COMMAND_STR_LENGTH];
        strncpy(cleanCommand, (char*)trainCommandTextArray[match], TRAIN_COMMAND_STR_LENGTH);
        cleanCommand[TRAIN_COMMAND_STR_LENGTH - 1] = '\0';
        for (int i = strlen(cleanCommand) - 1; i >= 0 && cleanCommand[i] == ' '; i--) {
          cleanCommand[i] = '\0';
        }
        int commandCode = 0;
switch (match) {
  case 1: commandCode = 42; break;
  case 2: commandCode = 43; break;
  case 3: commandCode = 44; break;
  case 4: commandCode = 41; break;
  case 5: commandCode = 20; break;
  case 6: commandCode = 21; break;
  case 7: commandCode = 22; break;
  case 8: commandCode = 23; break;
  case 9: commandCode = 24; break;
  case 10: commandCode = 31; break;
  case 11: commandCode = 32; break;
  case 12: commandCode = 33; break;
  case 13: commandCode = 25; break;
  case 14: commandCode = 26; break;
  case 15: commandCode = 27; break;
  case 16: commandCode = 28; break;
  case 17: commandCode = 29; break;
  case 18: commandCode = 30; break;
  default: commandCode = 0; break;
}
Serial.printf("%02d %s\n", commandCode, cleanCommand);

        sendAndDisplayCommand(match);
        lastMatch = match;
        lastReadTime = millis();
      }
    }
  } else {
    lastMatch = -1;
  }
}

void sendDataCentral(int data, unsigned long int delay = 0) {
  if (!delay) {
    if (Serial1.availableForWrite()) {
      Serial1.write(data);
      
    }
  } else {
    timer.setTimeout(delay, [=]() {
      Serial1.write(data);
      Serial.println(data);
    });
  }
}


void sendAndDisplayCommand(int trainCommandIndex) {
  switch (trainCommandIndex) {
    case 0: break;
    case 1: sendDataCentral(42); break;
    case 2: sendDataCentral(43); break;
    case 3: sendDataCentral(44); break;
    case 4: sendDataCentral(41); break;
    case 5: sendDataCentral(20); break;
    case 6: sendDataCentral(21); break;
    case 7: sendDataCentral(22); break;
    case 8: sendDataCentral(23); break;
    case 9: sendDataCentral(24); break;
    case 10: sendDataCentral(31); break;
    case 11: sendDataCentral(32); break;
    case 12: sendDataCentral(33); break;
    case 13: sendDataCentral(25); break;
    case 14: sendDataCentral(26); break;
    case 15: sendDataCentral(27); break;
    case 16: sendDataCentral(28); break;
    case 17: sendDataCentral(29); break;
    case 18: sendDataCentral(30); break;
    default: break;
  }
}

int tFIMatch(byte tagText[]) {
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {
    if (strncmp((char*)tagText, (char*)trainCommandTextArray[i], TRAIN_COMMAND_STR_LENGTH) == 0) {
      return i;
    }
  }
  return NUM_TRAIN_COMMANDS + 1;
}
