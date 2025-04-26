//============================================================
// Train Control System
// Uses Arduino Giga to interface with DCC-EX, RFID and GUI
//============================================================

int Train_selection = 1;     // K3 Stainz (K3) = 1, American Mogul (AM) = 0

// Include libraries
#include "Arduino_GigaDisplay_GFX.h"
#include <SimpleTimer.h>

//============================================================
// Constants
//============================================================
#define SOUND_DELAY 1000  // Delay after which to cancel sound (ms)
#define SD 500           // Short delay

// Display colors
#define GC9A01A_CYAN    0x07FF
#define GC9A01A_RED     0xf800
#define GC9A01A_BLUE    0x001F
#define GC9A01A_GREEN   0x07E0
#define GC9A01A_MAGENTA 0xF81F
#define GC9A01A_WHITE   0xffff
#define GC9A01A_BLACK   0x0000
#define GC9A01A_YELLOW  0xFFE0

//============================================================
// Serial connections
//============================================================
// Serial2 - RFID ESP-32 communication
// Serial3 - DCC-EX communication
// Serial4 - GUI ESP-32 communication

//============================================================
// State Variables
//============================================================
// Control states
int active = 0;
int RFID_sensor = 0;
int park_switch = 0;
int RFID_READY = 0;
int sound_switch = 1;
int light_switch = 0;
int smoke_switch = 0;
int speed_switch = 0;

// Train controls
int train_direction = 1;
int train_speed = 0;
int train_speed_hold = 0;
int last_sent_speed = -1;
int last_sent_direction = -1;

// Communication data
int GUI_data = 0;  
int RFID_data = 0;
int station_number_data = 0;

// Timing
unsigned long previousMillis = 0;

// Flags
bool new_GUI_data = false;
bool new_RFID_data = false;

// Display variables
String lastGUIData = "";
String lastRFIDData = "";
String GUI_text = "";
String RFID_text = "";
String Motion_text = "Train Parked";
String Speed_text = "";

// Initialize objects
GigaDisplay_GFX tft;
SimpleTimer timer;

//============================================================
// Function Declarations
//============================================================
void parseIncomingData();
void sendDataDccEX(char data[], unsigned long int delay = 0);
void sendDataGui(int data, unsigned long int delay = 0);
void updateGuiDisplay(String text);
void updateRfidDisplay(String text);
void updateMotionDisplay(String text);
void updateSpeedDisplay(String text);

//============================================================
// Setup
//============================================================
void setup() {
    // Configure pins
    pinMode(A4, OUTPUT);  // Connected to GUI ESP-32 pin 32
    pinMode(A5, INPUT);   // Connected to GUI ESP-32 pin 15
    pinMode(A6, INPUT);   // Connected to RFID ESP-32 pin 15
    pinMode(A7, OUTPUT);  // Connected to RFID ESP-32 pin 32
  
    // Initialize serial communication
    SerialUSB.begin(115200);
    Serial2.begin(115200);   
    Serial3.begin(115200);
    Serial4.begin(115200); 

    // Initialize display
    tft.begin();
    tft.setRotation(0);  
    tft.fillScreen(GC9A01A_BLACK);

    // Initial system setup with delays
    delay(1000);

    // Initialize DCC-EX
    Serial3.println("<0>");
    delay(500);
    Serial3.println("<1>");
    delay(500);
    Serial3.println("<F 3 8 1>");

    delay(1000);

    // Initial sound: Conductor calling
    Serial3.println("<F 3 2 1>");
    delay(2500);
    Serial3.println("<F 3 2 0>");

    updateGuiDisplay(GUI_text);
    updateRfidDisplay(RFID_text);
    updateMotionDisplay(Motion_text);
    updateSwitchDisplay();
     
}

//============================================================
// Main Loop
//============================================================
void loop() {
    parseIncomingData();
    timer.run();
}

//============================================================
// Data Processing
//============================================================
void parseIncomingData() {
    //-----------------------------------
    // Process GUI data
    //-----------------------------------
    if (Serial4.available()) {
        uint8_t guiChar = Serial4.read();  
        GUI_data = (int)guiChar;
        new_GUI_data = true;
        
        SerialUSB.print("GUI Data Received: ");
        SerialUSB.println(GUI_data);
    }

    if (new_GUI_data) {
        processGuiData();
        
        // Update display status data
        updateGuiDisplay(GUI_text); 
        
        SerialUSB.print("GUI Command: ");
        SerialUSB.println(GUI_text);
        
        new_GUI_data = false;
    }

    //-----------------------------------
    // Process RFID data
    //-----------------------------------
    if (RFID_sensor == 1 && Serial2.available()) {
        uint8_t rfidChar = Serial2.read();  
        RFID_data = (int)rfidChar;
        new_RFID_data = true;
        
        SerialUSB.print("RFID Data Received: ");
        SerialUSB.println(RFID_data);

    } else if (RFID_sensor == 0 && Serial2.available()) {
    // Discard data when RFID sensor is off
    Serial2.read();
    }


    if ((new_RFID_data) && (RFID_sensor == 1)) {
        processRfidData();
        
        // Update display status data
        updateRfidDisplay(RFID_text); 
        
        SerialUSB.print("RFID Command: ");
        SerialUSB.println(RFID_text);
        
        new_RFID_data = false;
    }
}

//============================================================
// GUI Data Processing
//============================================================
void processGuiData() {
    // Process light controls
    if (GUI_data == 111) {
        GUI_text = "Lights ON";
        light_switch = 1;
        sendDataDccEX("<F 3 0 1>");
    } else if (GUI_data == 110) {
        GUI_text = "Lights OFF";
        light_switch = 0;
        sendDataDccEX("<F 3 0 0>");
    }

    // Process smoke controls
    if (GUI_data == 121) {
        GUI_text = "Smoke ON";
        smoke_switch = 1;
        sendDataDccEX("<F 3 4 1>");
    } else if (GUI_data == 120) {
        GUI_text = "Smoke OFF";
        smoke_switch = 0;
        sendDataDccEX("<F 3 4 0>");
    } 

    // Process sound controls
    if (GUI_data == 131) {
        GUI_text = "Sound ON";        
        sendDataDccEX("<F 3 8 1>");
    } else if (GUI_data == 130) {
        GUI_text = "Sound OFF";
        sendDataDccEX("<F 3 8 0>");
    }

    // Process RFID sensor controls
    if (GUI_data == 141) {
        GUI_text = "RFID ON";
        RFID_sensor = 1;
    } else if (GUI_data == 140) {
        GUI_text = "RFID OFF";
        RFID_sensor = 0;
    }

    // Process park switch controls
    if (GUI_data == 151) {
        GUI_text = "Park ON";
        park_switch = 1;
    } else if (GUI_data == 150) {
        GUI_text = "Park OFF";
        park_switch = 0;
    }

    // Process direction controls
    if (GUI_data == 161) {
        GUI_text = "Direction - FORWARD";
        train_direction = 1;
    } else if (GUI_data == 160) {
        GUI_text = "Direction - REVERSE";
        train_direction = 0;
    }  

    // Process speed controls
    if (GUI_data == 7) {
        GUI_text = "Speed FASTER";
        train_speed = train_speed + 1;
    }
    if (GUI_data == 8) {
        GUI_text = "Speed SLOWER"; 
        train_speed = train_speed - 1;
    } 

    // Clamp train speed
    if (train_speed >= 10) train_speed = 10;
    if (train_speed <= 0) train_speed = 0;

    // Only send speed command if speed or direction changed
    if (train_speed != last_sent_speed || train_direction != last_sent_direction) {
        char speedCmd[30];
        int speedValue = (train_speed == 0) ? -1 : train_speed * 10;

        if (train_speed == 0) {
            Motion_text = "Train STOPPED";
            Speed_text = "";
        }
        else {
            if (train_direction == 1) { 
                Motion_text = "Train Moving FORWARD";
            }
            else { 
                Motion_text = "Train Moving REVERSE";
            }
            Speed_text = "Speed = " + String(train_speed * 10) + "%";
        }     

        sprintf(speedCmd, "<t 03 %d %d>", speedValue, train_direction);
        sendDataDccEX(speedCmd);
        sendDataGui(train_speed);
        updateMotionDisplay(Motion_text);
        updateSpeedDisplay(Speed_text);
    
        last_sent_speed = train_speed;
        last_sent_direction = train_direction;
    }

    // Train stop
    if (GUI_data == 9) {
        train_speed = 0;
        GUI_text = "Train STOP";
        Motion_text = "Train STOPPED";
        Speed_text = "";
        sendDataDccEX("<t 03 0 0>");
        updateMotionDisplay(Motion_text);
        updateSpeedDisplay(Speed_text);
    }  

    // Process sound effects
    processSoundEffects();

    // Update speed indicator
    if (train_speed != train_speed_hold) {
        sendDataGui(((train_speed * 10) + 100), 0);
        train_speed_hold = train_speed;
    }

    updateSwitchDisplay();
}

//============================================================
// Sound Effects Processing
//============================================================
void processSoundEffects() {
    // AM Sound Effects (41-64)
    if (Train_selection == 0) {
        if (GUI_data == 41) {
            sendDataDccEX("<F 3 7 1>");
            sendDataDccEX("<F 3 7 0>", SOUND_DELAY);
            GUI_text = "Bell";
        }
        else if (GUI_data == 42) {
            sendDataDccEX("<F 3 2 1>");
            sendDataDccEX("<F 3 2 0>", SOUND_DELAY);
            GUI_text = "Long whistles";
        }
        else if (GUI_data == 43) {
            sendDataDccEX("<F 3 9 1>");
            sendDataDccEX("<F 3 9 0>", SOUND_DELAY);
            GUI_text = "Whistle";
        }
        else if (GUI_data == 44) {
            sendDataDccEX("<F 3 4 1>");
            sendDataDccEX("<F 3 4 0>", SOUND_DELAY);
            GUI_text = "Short whistle";
        }
        else if (GUI_data == 49) {
            sendDataDccEX("<F 3 9 1>");
            sendDataDccEX("<F 3 9 0>", SOUND_DELAY);
            GUI_text = "Wheels squealing";
        }
        else if (GUI_data == 50) {
            sendDataDccEX("<F 3 10 1>");
            sendDataDccEX("<F 3 10 2>", SOUND_DELAY);
            GUI_text = "Shoveling coal";
        }
        else if (GUI_data == 51) {
            sendDataDccEX("<F 3 11 1>");
            sendDataDccEX("<F 3 11 0>", SOUND_DELAY);
            GUI_text = "Blower sound";
        }
        else if (GUI_data == 52) {
            sendDataDccEX("<F 3 12 1>");
            sendDataDccEX("<F 3 12 0>", SOUND_DELAY);
            GUI_text = "Coupler/Uncoupler";
        }
        else if (GUI_data == 53) {
            sendDataDccEX("<F 3 13 1>");
            sendDataDccEX("<F 3 13 0>", SOUND_DELAY);
            GUI_text = "Coal dropping";
        }
        else if (GUI_data == 54) {
            sendDataDccEX("<F 3 13 1>");
            sendDataDccEX("<F 3 13 0>", SOUND_DELAY);
            GUI_text = "Steam venting";
        }
        else if (GUI_data == 57) {
            sendDataDccEX("<F 3 17 1>");
            sendDataDccEX("<F 3 17 0>", SOUND_DELAY);
            GUI_text = "Conductor calling";
        }
        else if (GUI_data == 58) {
            sendDataDccEX("<F 3 18 1>");
            sendDataDccEX("<F 3 18 2>", SOUND_DELAY);
            GUI_text = "Generator sound";
        }
        else if (GUI_data == 59) {
            sendDataDccEX("<F 3 19 1>");
            sendDataDccEX("<F 3 19 0>", SOUND_DELAY);
            GUI_text = "Air pump";
        }
        else if (GUI_data == 60) {
            sendDataDccEX("<F 3 20 1>");
            sendDataDccEX("<F 3 20 0>", SOUND_DELAY);
            GUI_text = "Coal unloading";
        }
        else if (GUI_data == 61) {
            sendDataDccEX("<F 3 21 1>");
            sendDataDccEX("<F 3 21 0>", SOUND_DELAY);
            GUI_text = "Water filling tank";
        }
    }

    // K3 Stainz Sounds (65-79)
    if (Train_selection == 1) {
        if (GUI_data == 67) {
            GUI_text = "Conductor calling";
            sendDataDccEX("<F 3 2 1>");
            sendDataDccEX("<F 3 2 0>", SOUND_DELAY);
        }
        else if (GUI_data == 68) {
            GUI_text = "Conductor's whistle";
            sendDataDccEX("<F 3 3 1>");
            sendDataDccEX("<F 3 3 0>", SOUND_DELAY);
        }
        else if (GUI_data == 70) {
            sendDataDccEX("<F 3 5 1>");
            sendDataDccEX("<F 3 5 0>", SOUND_DELAY);
            GUI_text = "Coal shoveling";
        }
        else if (GUI_data == 71) {
            sendDataDccEX("<F 3 6 1>");
            sendDataDccEX("<F 3 6 0>", SOUND_DELAY);
            GUI_text = "Injector sound";
        }
        else if (GUI_data == 72) {
            sendDataDccEX("<F 3 7 1>");
            sendDataDccEX("<F 3 7 0>", SOUND_DELAY);
            GUI_text = "Bell";
        }
        else if (GUI_data == 74) {
            sendDataDccEX("<F 3 9 1>");
            sendDataDccEX("<F 3 9 0>", SOUND_DELAY);
            GUI_text = "Whistle";
        } 
        else if (GUI_data == 75) {
            sendDataDccEX("<F 3 10 1>");
            sendDataDccEX("<F 3 10 0>", SOUND_DELAY);
            GUI_text = "Coupler / Uncoupler";
        }
        else if (GUI_data == 76) {
            sendDataDccEX("<F 3 11 1>");
            sendDataDccEX("<F 3 11 0>", SOUND_DELAY);
            GUI_text = "Air pump";
        }
        else if (GUI_data == 77) {
            sendDataDccEX("<F 3 12 1>");
            sendDataDccEX("<F 3 12 0>", SOUND_DELAY);
            GUI_text = "Boiler sound";
        }
        else if (GUI_data == 78) {
            sendDataDccEX("<F 3 13 1>");
            sendDataDccEX("<F 3 13 0>", SOUND_DELAY);
            GUI_text = "Steam venting";
        }
    }    
}

//============================================================
// RFID Data Processing
//============================================================
void processRfidData() {

    // Process sound effects
    if (Train_selection == 0) {         // American Mogul
        if (RFID_data == 42) {
            RFID_text = "Long Whistles";
            sendDataDccEX("<F 3 2 1>");
            sendDataDccEX("<F 3 2 0>", SOUND_DELAY);
        }
        else if (RFID_data == 43) {
            RFID_text = "Whistle";
            sendDataDccEX("<F 3 3 1>");
            sendDataDccEX("<F 3 3 0>", SOUND_DELAY);
        }
        else if (RFID_data == 44) {
            RFID_text = "Short Whistle";
            sendDataDccEX("<F 3 4 1>");
            sendDataDccEX("<F 3 4 0>", SOUND_DELAY);
        }
        else if (RFID_data == 41) {
            RFID_text = "Bell";
            sendDataDccEX("<F 3 1 1>");
            sendDataDccEX("<F 3 1 0>", SOUND_DELAY);
        }
    }

    if (Train_selection == 1) {            // K3
        if (RFID_data == 43) {
            RFID_text = "Whistle";
            sendDataDccEX("<F 3 9 1>");
            sendDataDccEX("<F 3 9 0>", SOUND_DELAY);
        }
        else if (RFID_data == 41) {
            RFID_text = "Bell";
            sendDataDccEX("<F 3 7 1>");
            sendDataDccEX("<F 3 7 0>", SOUND_DELAY);
        }
    }


    // Process station data
    if (RFID_data == 25) {
        RFID_text = "Station 1";
        sendDataGui(25);
    }
    else if (RFID_data == 26) {
        RFID_text = "Station 2";
        sendDataGui(26);
    }
    else if (RFID_data == 27) {
        RFID_text = "Station 3";
        sendDataGui(27);
    }
    else if (RFID_data == 28) {
        RFID_text = "Station 4";
        sendDataGui(28);
    }
    else if (RFID_data == 29) {
        RFID_text = "Station 5";
        sendDataGui(29);
    }
    else if (RFID_data == 30) {
        RFID_text = "Station 6";
        sendDataGui(30);
    }

    // Process speed settings
    else if (RFID_data == 20) {
        RFID_text = "Speed 20%";
        train_speed = 2; 
        sendDataDccEX("<t 1 03 20 1>");
    }
    else if (RFID_data == 21) {
        RFID_text = "Speed 40%";
        train_speed = 4; 
        sendDataDccEX("<t 1 03 40 1>");
    }
    else if (RFID_data == 22) {
        RFID_text = "Speed 60%";
        train_speed = 6; 
        sendDataDccEX("<t 1 03 60 1>");
    }
    else if (RFID_data == 23) {
        RFID_text = "Speed 80%";
        train_speed = 8; 
        sendDataDccEX("<t 1 03 80 1>");
    }
    else if (RFID_data == 24) {
        RFID_text = "Speed 100%";
        train_speed = 10; 
        sendDataDccEX("<t 1 03 100 1>");
    }
    
    // Process park logic
    if (park_switch == 1 && train_direction == 1) {
        if (RFID_data == 31) { 
            RFID_text = "Speed 50%";
            train_speed = 5; 
            sendDataDccEX("<t 1 03 50 1>");
        }
        else if (RFID_data == 32) { 
            RFID_text = "Speed 30%";
            train_speed = 3; 
            sendDataDccEX("<t 1 03 30 1>");
        }
        else if (RFID_data == 33) { 
            RFID_text = "Parked";
            Motion_text = "Train Parked";
            train_speed = 0; 
            sendDataDccEX("<t 1 03 -1 1>");
            updateMotionDisplay(Motion_text);
        }
    }
    Speed_text = "Speed = " + String(train_speed * 10) + "%";
    updateSpeedDisplay(Speed_text);
}


//============================================================
// Display Update Functions
//============================================================
void updateGuiDisplay(String text) {
    int width = tft.width();
    int textSizeHeader = 3;
    int textSizeBody = 3;

    tft.setTextWrap(false);
    tft.fillRect(0, 100, width, 64, GC9A01A_BLACK);

    String header = "= GUI Command =";
    int headerX = (width - (header.length() * 6 * textSizeHeader)) / 2;
    tft.setTextSize(textSizeHeader);
    tft.setCursor(headerX, 100);
    tft.setTextColor(GC9A01A_CYAN);
    tft.print(header);

    int commandX = (width - (text.length() * 6 * textSizeBody)) / 2;
    tft.setTextSize(textSizeBody);
    tft.setCursor(commandX, 150);
    tft.setTextColor(GC9A01A_YELLOW);
    tft.println(text);

    timer.setTimeout(1500, [width]() {
        tft.fillRect(0, 150, width, 32, GC9A01A_BLACK);
    });
}

void updateRfidDisplay(String text) {
    int width = tft.width();
    int textSizeHeader = 3;
    int textSizeBody = 3;

    tft.setTextWrap(false);
    tft.fillRect(0, 250, width, 64, GC9A01A_BLACK);

    String header = "= RFID Command =";
    int headerX = (width - (header.length() * 6 * textSizeHeader)) / 2;
    tft.setTextSize(textSizeHeader);
    tft.setCursor(headerX, 250);
    tft.setTextColor(GC9A01A_CYAN);
    tft.print(header);

    int commandX = (width - (text.length() * 6 * textSizeBody)) / 2;
    tft.setTextSize(textSizeBody);
    tft.setCursor(commandX, 300);
    tft.setTextColor(GC9A01A_RED);
    tft.println(text);

    timer.setTimeout(1500, [width]() {
        tft.fillRect(0, 300, width, 32, GC9A01A_BLACK);
    });
}

void updateMotionDisplay(String text) {
    int width = tft.width();
    int textSizeBody = 3;
    
    // Button specifications
    int buttonHeight = 63;    // Same height as buttons in updateSwitchDisplay
    int margin = 20;          // Same margin as in updateSwitchDisplay
    int singleButtonWidth = 220; // Width of a single button from updateSwitchDisplay
    int buttonWidth = 2 * singleButtonWidth + margin; // Width of two buttons plus margin
    int cornerRadius = 8;     // Same radius for rounded corners
    
    // Calculate the horizontal center position
    int buttonX = (width - buttonWidth) / 2;
    
    // Y position for the button (using 400 from the original function)
    int buttonY = 400;
    
    // Darker gray background color (two shades darker)
    uint16_t darkerGrayColor = tft.color565(80, 80, 80); // Changed from 128 to 80
    
    // Clear the area
    tft.fillRect(0, buttonY, width, buttonHeight, GC9A01A_BLACK);
    
    // Draw the darker gray button with white border
    tft.fillRoundRect(buttonX, buttonY, buttonWidth, buttonHeight, cornerRadius, darkerGrayColor);
    tft.drawRoundRect(buttonX, buttonY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
    
    // Calculate text position to center it in the button
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    int textX = buttonX + (buttonWidth - textWidth) / 2;
    int textY = buttonY + (buttonHeight / 2) - 10; // Vertical center adjustment like in updateSwitchDisplay
    
    // Set text properties and print
    tft.setTextWrap(false);
    tft.setTextSize(textSizeBody);
    tft.setTextColor(GC9A01A_WHITE);
    tft.setCursor(textX, textY);
    tft.print(text);
}

void updateSpeedDisplay(String text) {
    int width = tft.width();
    int textSizeBody = 3;
    
    // Button specifications - matching the updateMotionDisplay button
    int buttonHeight = 63;    // Same height as buttons in updateSwitchDisplay
    int margin = 20;          // Same margin as in updateSwitchDisplay
    int singleButtonWidth = 220; // Width of a single button from updateSwitchDisplay
    int buttonWidth = 2 * singleButtonWidth + margin; // Width of two buttons plus margin
    int cornerRadius = 8;     // Same radius for rounded corners
    
    // Calculate the horizontal center position
    int buttonX = (width - buttonWidth) / 2;
    
    // Calculate Y position for the button
    // The button in updateMotionDisplay is at y=400
    // Use the same vertical spacing as in updateSwitchDisplay (which was 20 pixels)
    int motionButtonY = 400;
    int speedButtonY = motionButtonY + buttonHeight + 20; // 20 is the vertical gap
    
    // Darker gray background color - same as updateMotionDisplay
    uint16_t darkerGrayColor = tft.color565(80, 80, 80);
    
    // Clear the area
    tft.fillRect(0, speedButtonY, width, buttonHeight, GC9A01A_BLACK);
    
    // Draw the darker gray button with white border
    tft.fillRoundRect(buttonX, speedButtonY, buttonWidth, buttonHeight, cornerRadius, darkerGrayColor);
    tft.drawRoundRect(buttonX, speedButtonY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
    
    // Calculate text position to center it in the button
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    int textX = buttonX + (buttonWidth - textWidth) / 2;
    int textY = speedButtonY + (buttonHeight / 2) - 10; // Vertical center adjustment
    
    // Set text properties and print
    tft.setTextWrap(false);
    tft.setTextSize(textSizeBody);
    tft.setTextColor(GC9A01A_WHITE); // Changed from GREEN to WHITE
    tft.setCursor(textX, textY);
    tft.print(text);
}

void updateSwitchDisplay() {
    int width = tft.width();  // Get screen width
    int buttonHeight = 63;    // Button height increased by 25% (from 50 to 63)
    int buttonWidth = 220;    // Button width
    int cornerRadius = 8;     // Radius for rounded corners
    int textSizeBody = 3;
    
    // Define a darker blue color using color565 function
    uint16_t darkerBlue = tft.color565(0, 0, 80); // Darker navy blue (was 128, now 80)
    
    // Calculate horizontal positioning - bringing the buttons closer horizontally
    int margin = 20;          // Reduced space between buttons (was 30)
    // Calculate outer margins to equal the center margin
    int totalUsedWidth = 2 * buttonWidth + margin;
    int sideMargin = (width - totalUsedWidth) / 2; // Equal margin on both sides
    
    int leftButtonX = sideMargin;
    int rightButtonX = leftButtonX + buttonWidth + margin;
    
    // Y positions - maintain the same spacing between button rows
    int topRowY = 580;
    int bottomRowY = topRowY + buttonHeight + 20; // 20 is the original gap
    
    tft.setTextWrap(false);
    tft.setTextSize(textSizeBody);
    tft.setTextColor(GC9A01A_WHITE);

    // Adjusted vertical positioning for the taller buttons
    int textYAdjustment = buttonHeight/2 - 10; // Keeps text vertically centered
    
    // Lights button
    if (light_switch == 1) {
        const char* text = "Lights ON";
        tft.fillRoundRect(leftButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(leftButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = leftButtonX + (buttonWidth - textWidth) / 2;
        int textY = topRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    } else {
        const char* text = "Lights OFF";
        tft.fillRoundRect(leftButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(leftButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = leftButtonX + (buttonWidth - textWidth) / 2;
        int textY = topRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    }

    // Smoke button
    if (smoke_switch == 1) {
        const char* text = "Smoke ON";
        tft.fillRoundRect(rightButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(rightButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = rightButtonX + (buttonWidth - textWidth) / 2;
        int textY = topRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    } else {
        const char* text = "Smoke OFF";
        tft.fillRoundRect(rightButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(rightButtonX, topRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = rightButtonX + (buttonWidth - textWidth) / 2;
        int textY = topRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    } 

    // RFID button
    if (RFID_sensor == 1) {
        const char* text = "RFID ON";
        tft.fillRoundRect(leftButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(leftButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = leftButtonX + (buttonWidth - textWidth) / 2;
        int textY = bottomRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    } else {
        const char* text = "RFID OFF";
        tft.fillRoundRect(leftButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(leftButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = leftButtonX + (buttonWidth - textWidth) / 2;
        int textY = bottomRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    }

    // Park button
    if (park_switch == 1) {
        const char* text = "Park ON";
        tft.fillRoundRect(rightButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(rightButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = rightButtonX + (buttonWidth - textWidth) / 2;
        int textY = bottomRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    } else {
        const char* text = "Park OFF";
        tft.fillRoundRect(rightButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
        tft.drawRoundRect(rightButtonX, bottomRowY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
        
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int textX = rightButtonX + (buttonWidth - textWidth) / 2;
        int textY = bottomRowY + textYAdjustment; // Adjusted vertical positioning
        
        tft.setCursor(textX, textY);
        tft.print(text);
    }
}

//============================================================
// Communication Functions
//============================================================
void sendDataDccEX(char data[], unsigned long int delay) {
    if (!delay) {
        Serial3.print(data);
    } else {
        // Create a string copy of the input data
        String copy = String(data);
        timer.setTimeout(delay, [copy]() {
            Serial3.print(copy);
        });
    }
}

void sendDataGui(int data, unsigned long int delay) {
    if (!delay) {
        Serial4.write(data);
    } else {
        timer.setTimeout(delay, [data]() {
            Serial4.write(data);
        });
    }
}