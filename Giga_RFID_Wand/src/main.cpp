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

#define D_DELAY 750  // Reduced for better button responsiveness

Adafruit_SH1107 display(64, 128, &Wire);
Adafruit_PN532 nfc(PN532_SS, &SPI);

enum Mode { WRITE_MODE = 0, READ_MODE = 1 };
Mode mode = READ_MODE;

#define NUM_TRAIN_COMMANDS 19
#define TRAIN_COMMAND_STR_LENGTH 32
int trainCommandIndex = 1;

SimpleTimer timer;

uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Modified array structure - two strings per command for easier display
// First string is top line, second string is bottom line (empty string means single line)
const char trainCommandTextArray[NUM_TRAIN_COMMANDS][2][TRAIN_COMMAND_STR_LENGTH] = {
  {"", ""},               // 0 - Empty
  {"Long", "whistles"},   // 1
  {"Whistle", ""},        // 2
  {"Short", "whistle"},   // 3
  {"Bell", ""},           // 4
  {"Speed 20%", ""},      // 5
  {"Speed 40%", ""},      // 6
  {"Speed 60%", ""},      // 7
  {"Speed 80%", ""},      // 8
  {"Speed 100%", ""},     // 9
  {"Park", "Trigger 1"},  // 10
  {"Park", "Trigger 2"},  // 11
  {"Park", ""},           // 12
  {"Station 1", ""},      // 13
  {"Station 2", ""},      // 14
  {"Station 3", ""},      // 15
  {"Station 4", ""},      // 16
  {"Station 5", ""},      // 17
  {"Station 6", ""}       // 18
};

// This combined array is used for RFID reading/writing to maintain compatibility
char combinedCommandArray[NUM_TRAIN_COMMANDS][TRAIN_COMMAND_STR_LENGTH];

// Function prototypes - needed for PlatformIO compilation
void displayCenteredText(const char* text, int delayTime);
void displayCenteredTextTwoLines(const char* line1, const char* line2, int delayTime);
void smartDisplay(const char* line1, const char* line2);
void commandDisplay(int command);
int tFIMatch(byte tagText[]);
void RFID(uint8_t* uid, uint8_t uidLength);
void buttons();

// Display text centered on display
void displayCenteredText(const char* text, int delayTime) {
  display.clearDisplay();
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, (display.height() - h) / 2);
  display.print(text);
  display.display();
  delay(delayTime);
  display.clearDisplay();  // Clears after delay
  display.display();
}

// Display two lines of text centered on display
void displayCenteredTextTwoLines(const char* line1, const char* line2, int delayTime) {
  display.clearDisplay();
  display.setTextSize(2);

  int16_t x1, y1, x2, y2;
  uint16_t w1, h1, w2, h2;

  // Measure both lines independently
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
  
  // If there's no second line text, center the first line vertically
  if (line2[0] == '\0') {
    display.setCursor((display.width() - w1) / 2, (display.height() - h1) / 2);
    display.print(line1);
  } else {
    // Calculate for two lines
    display.getTextBounds(line2, 0, 0, &x2, &y2, &w2, &h2);
    
    // Total height calculation for vertical centering
    int totalHeight = h1 + h2 + 2;  // 2 pixels gap
    int startY = (display.height() - totalHeight) / 2;

    // Set cursor and print first line centered
    display.setCursor((display.width() - w1) / 2, startY);
    display.print(line1);

    // Set cursor and print second line centered
    display.setCursor((display.width() - w2) / 2, startY + h1 + 2);
    display.print(line2);
  }

  display.display();
  
  // Instead of blocking with delay(), we'll use a non-blocking method
  // The original function used a delay and then cleared the display
  // We'll keep this behavior but make it non-blocking in a real application
  delay(delayTime); // Still using delay for simplicity in this example
  
  // Clear display after delay
  display.clearDisplay();
  display.display();
}

// Simplified wrapper for displayCenteredTextTwoLines
void smartDisplay(const char* line1, const char* line2) {
  displayCenteredTextTwoLines(line1, line2, D_DELAY);
}

// Display a command from the array
void commandDisplay(int command) {
  if (command >= 0 && command < NUM_TRAIN_COMMANDS)
    displayCenteredTextTwoLines(
      trainCommandTextArray[command][0],
      trainCommandTextArray[command][1],
      D_DELAY
    );
  else
    displayCenteredTextTwoLines("No Match!", "", D_DELAY);
}

// Match tag text with commands in array
int tFIMatch(byte tagText[]) {
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {
    if (!strcmp((char*)tagText, combinedCommandArray[i])) {
      return i;
    }
  }
  return -1;
}

// Write command to RFID tag
void RFID(uint8_t* uid, uint8_t uidLength) {
  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
    smartDisplay("Auth", "failed");
    return;
  }

  for (int i = 0; i < 2; i++) {
    uint8_t blockData[16] = {0};
    memcpy(blockData, &combinedCommandArray[trainCommandIndex][i * 16], 16);
    if (!nfc.mifareclassic_WriteDataBlock(4 + i, blockData)) {
      smartDisplay("Write", "failed");
      return;
    }
  }

  // Display what was written using our two-line format
  displayCenteredTextTwoLines(
    trainCommandTextArray[trainCommandIndex][0], 
    trainCommandTextArray[trainCommandIndex][1], 
    D_DELAY
  );
}

// Handle button presses
void buttons() {
  static bool lastAState = HIGH, lastBState = HIGH, lastCState = HIGH;
  static unsigned long lastButtonTime = 0;
  bool a = digitalRead(BUTTON_A);
  bool b = digitalRead(BUTTON_B);
  bool c = digitalRead(BUTTON_C);
  unsigned long currentTime = millis();
  
  // Only process button presses if enough time has passed since the last press
  if (currentTime - lastButtonTime > 100) { // 100ms debounce time
    if (a == LOW && lastAState == HIGH) {
      lastButtonTime = currentTime;
      mode = (mode == WRITE_MODE) ? READ_MODE : WRITE_MODE;
      smartDisplay((mode == WRITE_MODE) ? "Write Mode" : "Read Mode", "");
    }

    if (mode == WRITE_MODE) {
      if (c == LOW && lastCState == HIGH) {
        lastButtonTime = currentTime;
        trainCommandIndex = (trainCommandIndex + 1) % NUM_TRAIN_COMMANDS;
        if (trainCommandIndex == 0) trainCommandIndex = 1;
        commandDisplay(trainCommandIndex);
      }
      if (b == LOW && lastBState == HIGH) {
        lastButtonTime = currentTime;
        trainCommandIndex = (trainCommandIndex - 1);
        if (trainCommandIndex <= 0) trainCommandIndex = NUM_TRAIN_COMMANDS - 1;
        commandDisplay(trainCommandIndex);
      }
    }
  }

  lastAState = a;
  lastBState = b;
  lastCState = c;
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  nfc.begin();

  // Combine the two-line arrays into single strings for RFID operations
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {
    if (trainCommandTextArray[i][1][0] == '\0') {
      // Only first line has content
      strcpy(combinedCommandArray[i], trainCommandTextArray[i][0]);
    } else {
      // Both lines have content - combine with space
      sprintf(combinedCommandArray[i], "%s %s", 
              trainCommandTextArray[i][0], 
              trainCommandTextArray[i][1]);
    }
  }

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

  smartDisplay("Read Mode", "");
  timer.setInterval(50, buttons);
}

void loop() {
  timer.run();

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 200)) {
    if (mode == READ_MODE) {
      if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
        smartDisplay("Auth", "failed");
        return;
      }

      byte buffer[32];
      bool success = true;
      for (int i = 0; i < 2; i++) {
        if (!nfc.mifareclassic_ReadDataBlock(4 + i, &buffer[i * 16])) {
          smartDisplay("Read", "failed");
          success = false;
          break;
        }
      }
      if (success) {
        int commandIndex = tFIMatch(buffer);
        if (commandIndex >= 0) commandDisplay(commandIndex);
        else smartDisplay("No Match!", "");
      }
    } else if (mode == WRITE_MODE) {
      RFID(uid, uidLength);
    }
  }
}
