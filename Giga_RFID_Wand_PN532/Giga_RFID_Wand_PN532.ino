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

// FeatherWing OLED specific initialization
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);



// Create the PN532 RFID/NFC reader with SPI communication
Adafruit_PN532 nfc(PN532_SS, &SPI);

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

uint8_t keya[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

byte trainCommandTextArray[NUM_TRAIN_COMMANDS][TRAIN_COMMAND_STR_LENGTH] = {
  "", "Long whistles", "Whistle", "Short whistle", "Bell", "Speed 20%",
  "Speed 40%", "Speed 60%", "Speed 80%", "Speed 100%", "Park Trigger 1",
  "Park Trigger 2", "Park", "Station 1", "Station 2", "Station 3",
  "Station 4", "Station 5", "Station 6"
};

// Forward declarations
void buttons();
void commandDisplay(int command);
void tFIIncrementer(int inc);
int tFIMatch(byte tagText[]);
void RFID();

// Single line centered text (clears after delay)
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

  if (delayTime > 0) {
    delay(delayTime);
    display.clearDisplay();
    display.display();
  }
}

// Two-line centered text (clears after delay)
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

  int x1pos = (display.width() - w1) / 2;
  int x2pos = (display.width() - w2) / 2;

  display.setCursor(x1pos, y1pos);
  display.print(line1);
  display.setCursor(x2pos, y2pos);
  display.print(line2);
  display.display();

  if (delayTime > 0) {
    delay(delayTime);
    display.clearDisplay();
    display.display();
  }
}


void setup() {
  Serial.begin(115200);
  
  SPI.begin();  // Initialize SPI bus
  
  // Initialize the NFC reader
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while(1);
  }
  nfc.SAMConfig();

  // Initialize the OLED display (FeatherWing OLED specific)
  if(!display.begin(0x3C, true)) {
    Serial.println("SH1107 allocation failed");
    for(;;);
  }

  // Set rotation specifically for FeatherWing OLED
  display.setRotation(3);

  // Clear the display initially
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  displayCenteredTextTwoLines("Read", "Mode", D_DELAY);
  timer.setInterval(30, buttons);
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    if (mode == READ_MODE) {
      if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
        Serial.println("Auth failed");
      } else {
        byte buffer[32];
        bool success = true;
        for (int i = 0; i < 2; i++) {
          if (!nfc.mifareclassic_ReadDataBlock(4+i, &buffer[i*16])) {
            Serial.println("Read failed");
            success = false;
            break;
          }
        }
        if (success) {
          int commandIndex = tFIMatch(buffer);
          commandDisplay(commandIndex);
          delay(1000);
        }
      }
    } else if (mode == WRITE_MODE) {
      RFID();
      delay(750);
    }
  }

  timer.run();
}

void RFID() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    Serial.println("No tag found.");
    return;
  }

  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya)) {
    Serial.println("Auth failed");
    return;
  }

  for (int i=0; i<2; i++) {
    uint8_t blockData[16];
    memset(blockData, 0, 16);
    memcpy(blockData, &trainCommandTextArray[trainCommandIndex][i*16], 16);

    if (!nfc.mifareclassic_WriteDataBlock(4+i, blockData)) {
      Serial.println("Write failed");
      return;
    }
  }

  // For successful write
  displayCenteredText("Wrote:", D_DELAY);
  delay(500);

  // Get the command text for the current index
  char* commandText = (char*)trainCommandTextArray[trainCommandIndex];

  // Find if the command has a space (indicating two words)
  char* spacePos = strchr(commandText, ' ');

  if (spacePos == NULL) {
    // One word case - center it vertically and horizontally
    displayCenteredText(commandText, D_DELAY);
  } else {
    // Two word case - split and display on two lines
    // Temporarily replace space with null terminator
    *spacePos = '\0';
    
    // First word centered horizontally on top line, second word beneath it
    displayCenteredTextTwoLines(commandText, spacePos + 1, D_DELAY);
    
    // Restore the space
    *spacePos = ' ';
  }
}

void buttons() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;
  static bool firstCommandSelected = false;

  bool a = digitalRead(BUTTON_A);
  bool b = digitalRead(BUTTON_B);
  bool c = digitalRead(BUTTON_C);

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (a == LOW && lastAState == HIGH) {
      mode = (mode == WRITE_MODE) ? READ_MODE : WRITE_MODE;
      displayCenteredTextTwoLines((mode == WRITE_MODE) ? "Write" : "Read", "Mode", D_DELAY);
      
      // If entering write mode, reset the flag and clear the screen after a delay
      if (mode == WRITE_MODE) {
        delay(1000); // Show "Write Mode" for 1 second
        firstCommandSelected = false;
        display.clearDisplay();
        display.display();
      }
      else {
        // If entering read mode, reset the flag
        firstCommandSelected = false;
      }
    }
    
    if (mode == WRITE_MODE) {
      if (c == LOW && lastCState == HIGH) {  // Button on pin 15 now scrolls down (increments)
        if (!firstCommandSelected) {
          trainCommandIndex = 1;
          firstCommandSelected = true;
        } else {
          tFIIncrementer(1);
          if (trainCommandIndex == 0) tFIIncrementer(1);
        }
        commandDisplay(trainCommandIndex);
      }
      
      if (b == LOW && lastBState == HIGH) {  // Button on pin 14 now scrolls up (decrements)
        if (!firstCommandSelected) {
          trainCommandIndex = NUM_TRAIN_COMMANDS - 1;
          firstCommandSelected = true;
        } else {
          tFIIncrementer(-1);
          if (trainCommandIndex == 0) tFIIncrementer(-1);
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


void commandDisplay(int command){
  switch(command){
    case 0: displayCenteredText("", D_DELAY); break;
    case 1: displayCenteredTextTwoLines("Long", "whistles", D_DELAY); break;
    case 2: displayCenteredText("Whistle", D_DELAY); break;
    case 3: displayCenteredTextTwoLines("Short", "whistle", D_DELAY); break;
    case 4: displayCenteredText("Bell", D_DELAY); break;
    case 5: displayCenteredText("Speed 20%", D_DELAY); break;
    case 6: displayCenteredText("Speed 40%", D_DELAY); break;
    case 7: displayCenteredText("Speed 60%", D_DELAY); break;
    case 8: displayCenteredText("Speed 80%", D_DELAY); break;
    case 9: displayCenteredText("Speed 100%", D_DELAY); break;
    case 10: displayCenteredTextTwoLines("Park", "trigger 1", D_DELAY); break;
    case 11: displayCenteredTextTwoLines("Park", "trigger 2", D_DELAY); break;
    case 12: displayCenteredText("Park", D_DELAY); break;
    case 13: displayCenteredText("Station 1", D_DELAY); break;
    case 14: displayCenteredText("Station 2", D_DELAY); break;
    case 15: displayCenteredText("Station 3", D_DELAY); break;
    case 16: displayCenteredText("Station 4", D_DELAY); break;
    case 17: displayCenteredText("Station 5", D_DELAY); break;
    case 18: displayCenteredText("Station 6", D_DELAY); break;
    default: displayCenteredText("!No Match!", D_DELAY);
  }
}

void tFIIncrementer(int inc){
  trainCommandIndex += inc;
  if(trainCommandIndex < 0) trainCommandIndex = NUM_TRAIN_COMMANDS - 1;
  if(trainCommandIndex >= NUM_TRAIN_COMMANDS) trainCommandIndex = 0;
}

int tFIMatch(byte tagText[]){
  for(int i = 0; i < NUM_TRAIN_COMMANDS; i++){
    if(!strcmp((char*)tagText, (char*)trainCommandTextArray[i])){
      return i;
    }
  }
  return NUM_TRAIN_COMMANDS + 1;
}