#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH1107_Ext.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <SimpleTimer.h>

// Pin Definitions
#define PN532_SS 13
#define BUTTON_A 32
#define BUTTON_B 14
#define BUTTON_C 15

// Display parameters
#define D_HEIGHT 64
#define D_WIDTH 128
#define D_DELAY 1000

Adafruit_SH1107_Ext display(D_HEIGHT, D_WIDTH, &Wire, -1, 1000000, 1000000);
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

void setup() {
  Serial.begin(115200);
  SPI.begin(5, 21, 19);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while(1);
  }
  nfc.SAMConfig();

  display.begin(0x3C, true);
  display.setRotation(1);
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  display.centeredDisplay("Read","Mode",D_DELAY);
  timer.setInterval(30, buttons);
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
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
      delay(1000);
    }
  }

  timer.run();
  display.timerRun();
}

void RFID() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
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
    memcpy(blockData, &trainCommandTextArray[trainCommandIndex][i*16],16);

    if (!nfc.mifareclassic_WriteDataBlock(4+i, blockData)) {
      Serial.println("Write failed");
      return;
    }
  }

  // For successful write
display.centeredDisplay("Wrote:", D_DELAY);
delay(1000);

// Get the command text for the current index
char* commandText = (char*)trainCommandTextArray[trainCommandIndex];

// Find if the command has a space (indicating two words)
char* spacePos = strchr(commandText, ' ');

if (spacePos == NULL) {
  // One word case - center it vertically and horizontally
  display.centeredDisplay(commandText, D_DELAY);
} else {
  // Two word case - split and display on two lines
  // Temporarily replace space with null terminator
  *spacePos = '\0';
  
  // First word centered horizontally on top line, second word beneath it
  display.centeredDisplay(commandText, spacePos + 1, D_DELAY);
  
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
      display.centeredDisplay((mode == WRITE_MODE) ? "Write" : "Read", "Mode", D_DELAY);
      
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
      if (b == LOW && lastBState == HIGH) {
        if (!firstCommandSelected) {
          // First button press after entering write mode
          trainCommandIndex = 1;
          firstCommandSelected = true;
        } else {
          // Normal increment
          tFIIncrementer(1);
          if (trainCommandIndex == 0) tFIIncrementer(1);
        }
        commandDisplay(trainCommandIndex);
      }
      
      if (c == LOW && lastCState == HIGH) {
        if (!firstCommandSelected) {
          // First button press after entering write mode
          trainCommandIndex = NUM_TRAIN_COMMANDS - 1; // Start from the last command
          firstCommandSelected = true;
        } else {
          // Normal decrement
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
    case 0: display.centeredDisplay("",D_DELAY); break;
    case 1: display.centeredDisplay("Long","whistles",D_DELAY); break;
    case 2: display.centeredDisplay("Whistle",D_DELAY); break;
    case 3: display.centeredDisplay("Short","whistle",D_DELAY); break;
    case 4: display.centeredDisplay("Bell",D_DELAY); break;
    case 5: display.centeredDisplay("Speed 20%",D_DELAY); break;
    case 6: display.centeredDisplay("Speed 40%",D_DELAY); break;
    case 7: display.centeredDisplay("Speed 60%",D_DELAY); break;
    case 8: display.centeredDisplay("Speed 80%",D_DELAY); break;
    case 9: display.centeredDisplay("Speed 100%",D_DELAY); break;
    case 10: display.centeredDisplay("Park","trigger 1",D_DELAY); break;
    case 11: display.centeredDisplay("Park","trigger 2",D_DELAY); break;
    case 12: display.centeredDisplay("Park",D_DELAY); break;
    case 13: display.centeredDisplay("Station 1",D_DELAY); break;
    case 14: display.centeredDisplay("Station 2",D_DELAY); break;
    case 15: display.centeredDisplay("Station 3",D_DELAY); break;
    case 16: display.centeredDisplay("Station 4",D_DELAY); break;
    case 17: display.centeredDisplay("Station 5",D_DELAY); break;
    case 18: display.centeredDisplay("Station 6",D_DELAY); break;
    default: display.centeredDisplay("!No Match!",D_DELAY);
  }
}

void tFIIncrementer(int inc){
  trainCommandIndex += inc;
  if(trainCommandIndex<0) trainCommandIndex=NUM_TRAIN_COMMANDS-1;
  if(trainCommandIndex>=NUM_TRAIN_COMMANDS) trainCommandIndex=0;
}

int tFIMatch(byte tagText[]){
  for(int i=0; i<NUM_TRAIN_COMMANDS; i++){
    if(!strcmp((char*)tagText,(char*)trainCommandTextArray[i])){
      return i;
    }
  }
  return NUM_TRAIN_COMMANDS+1;
}


