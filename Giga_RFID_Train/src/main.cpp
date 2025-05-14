// Refactored Feather ESP32 PN532 Sketch with No Lambdas and Optimized Logic

#include <SPI.h>
#include <Adafruit_PN532.h>
#include <SimpleTimer.h>
#include <string.h>  // For strlen()

const uint8_t PN532_SS = 13;
const int TRAIN_COMMAND_STR_LENGTH = 32;
const int NUM_TRAIN_COMMANDS = 19;
const int BLOCK_TO_READ = 4;

SimpleTimer timer;
Adafruit_PN532 nfc(PN532_SS, &SPI);

uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const char trainCommandTextArray[NUM_TRAIN_COMMANDS][TRAIN_COMMAND_STR_LENGTH] = {
  "", "Long whistles", "Whistle", "Short whistle", "Bell", "Speed 20%",
  "Speed 40%", "Speed 60%", "Speed 80%", "Speed 100%", "Park Trigger 1",
  "Park Trigger 2", "Park", "Station 1", "Station 2", "Station 3",
  "Station 4", "Station 5", "Station 6"
};

const int commandCodes[NUM_TRAIN_COMMANDS] = {
  0, 42, 43, 44, 41, 20, 21, 22, 23, 24, 31,
  32, 33, 25, 26, 27, 28, 29, 30
};

int pendingData = -1;
bool hasPendingData = false;

void sendDataCentral(int data);
void sendPendingData();
void sendAndDisplayCommand(int index);
int tFIMatch(byte tagText[]);
void trimTrailingSpaces(char* str);

void setup() {
  delay(500);
  Serial.begin(115200);
  Serial1.begin(115200);

  // Feather ESP32 V2 SPI pins: SCK=5, MISO=21, MOSI=19
  SPI.begin(5, 21, 19);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1);
  }

  nfc.SAMConfig();
  timer.setInterval(50, sendPendingData);
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
        if (!nfc.mifareclassic_ReadDataBlock(BLOCK_TO_READ + i, &buffer[i * 16])) {
          Serial.println("Read failed");
          return;
        }
      }

      int match = tFIMatch(buffer);
      if (match != lastMatch && match >= 0 && match < NUM_TRAIN_COMMANDS) {
        char cleanCommand[TRAIN_COMMAND_STR_LENGTH];
        strncpy(cleanCommand, trainCommandTextArray[match], TRAIN_COMMAND_STR_LENGTH);
        cleanCommand[TRAIN_COMMAND_STR_LENGTH - 1] = '\0';
        trimTrailingSpaces(cleanCommand);

        Serial.print(commandCodes[match]);
        Serial.print(" ");
        Serial.println(cleanCommand);

        sendAndDisplayCommand(match);

        lastMatch = match;
        lastReadTime = millis();
      }
    }
  } else {
    lastMatch = -1;
  }
}

void sendDataCentral(int data) {
  pendingData = data;
  hasPendingData = true;
}

void sendPendingData() {
  if (hasPendingData && Serial1.availableForWrite()) {
    Serial1.write(pendingData);
    Serial.println(pendingData);
    hasPendingData = false;
  }
}

void sendAndDisplayCommand(int index) {
  if (index > 0 && index < NUM_TRAIN_COMMANDS) {
    sendDataCentral(commandCodes[index]);
  }
}

int tFIMatch(byte tagText[]) {
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {
    if (strncmp((char*)tagText, trainCommandTextArray[i], TRAIN_COMMAND_STR_LENGTH) == 0) {
      return i;
    }
  }
  return -1;
}

void trimTrailingSpaces(char* str) {
  for (int i = strlen(str) - 1; i >= 0 && str[i] == ' '; i--) {
    str[i] = '\0';
  }
}
