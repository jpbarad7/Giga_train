// Giga_Train_Control - working copy 05/07/2025

//============================================================
// Train Selection
//============================================================
int Train_selection = 1;     
    // K3 Stainz (K3) = 1 
    // American Mogul (AM) = 0

//============================================================
// Include libraries
//============================================================
#include "Arduino_GigaDisplay_GFX.h"
#include <SimpleTimer.h>

//============================================================
// Constants
//============================================================
#define SOUND_DELAY 1000  // Delay after which to cancel sound (ms)
#define SD 500             // Short delay

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
// Enum Definitions
//============================================================
enum GuiCommands {
    GUI_SPEED_FASTER      = 7,
    GUI_SPEED_SLOWER      = 8,
    GUI_STOP              = 9,
    GUI_LIGHTS_OFF        = 110,
    GUI_LIGHTS_ON         = 111,
    GUI_SMOKE_OFF         = 120,
    GUI_SMOKE_ON          = 121,
    GUI_SOUND_OFF         = 130,
    GUI_SOUND_ON          = 131,
    GUI_RFID_OFF          = 140,
    GUI_RFID_ON           = 141,
    GUI_PARK_OFF          = 150,
    GUI_PARK_ON           = 151,
    GUI_DIRECTION_REVERSE = 160,
    GUI_DIRECTION_FORWARD = 161
};

enum RfidCommands {
    RFID_STATION_1        = 25,
    RFID_STATION_2        = 26,
    RFID_STATION_3        = 27,
    RFID_STATION_4        = 28,
    RFID_STATION_5        = 29,
    RFID_STATION_6        = 30,
    RFID_SPEED_20         = 20,
    RFID_SPEED_40         = 21,
    RFID_SPEED_60         = 22,
    RFID_SPEED_80         = 23,
    RFID_SPEED_100        = 24,
    RFID_PARK_1           = 31,
    RFID_PARK_2           = 32,
    RFID_PARKED           = 33,
    RFID_BELL_AM          = 41,
    RFID_LONG_WHISTLE_AM  = 42,
    RFID_WHISTLE_AM       = 43,
    RFID_SHORT_WHISTLE_AM = 44,
    RFID_BELL_K3          = 41,
    RFID_WHISTLE_K3       = 43
};

// GUI Sound Effect Codes (used in processSoundEffects)
enum GuiSoundEffects {
    GUI_AM_BELL                  = 41,
    GUI_AM_LONG_WHISTLE          = 42,
    GUI_AM_WHISTLE               = 43,
    GUI_AM_SHORT_WHISTLE         = 44,
    GUI_AM_WHEELS_SQUEALING      = 49,
    GUI_AM_SHOVELING_COAL        = 50,
    GUI_AM_BLOWER_SOUND          = 51,
    GUI_AM_COUPLER               = 52,
    GUI_AM_COAL_DROPPING         = 53,
    GUI_AM_STEAM_VENTING         = 54,
    GUI_AM_CONDUCTOR_CALLING     = 57,
    GUI_AM_GENERATOR             = 58,
    GUI_AM_AIR_PUMP              = 59,
    GUI_AM_COAL_UNLOADING        = 60,
    GUI_AM_WATER_FILLING_TANK    = 61,

    GUI_K3_CONDUCTOR_CALLING     = 67,
    GUI_K3_CONDUCTOR_WHISTLE     = 68,
    GUI_K3_COAL_SHOVELING        = 70,
    GUI_K3_INJECTOR              = 71,
    GUI_K3_BELL                  = 72,
    GUI_K3_WHISTLE               = 74,
    GUI_K3_COUPLER               = 75,
    GUI_K3_AIR_PUMP              = 76,
    GUI_K3_BOILER                = 77,
    GUI_K3_STEAM_VENTING         = 78
};

//============================================================
// Speed Symbolic Constants
//============================================================
#define SPEED_20 2
#define SPEED_40 4
#define SPEED_60 6
#define SPEED_80 8
#define SPEED_100 10
#define SPEED_PARK_1 5
#define SPEED_PARK_2 3

//============================================================
// State Variables
//============================================================
int active = 0;
int RFID_sensor = 0;
int park_switch = 0;
int RFID_READY = 0;
int sound_switch = 1;
int light_switch = 0;
int smoke_switch = 0;
int speed_switch = 0;

int train_direction = 1;
int train_speed = 0;
int train_speed_hold = 0;
int last_sent_speed = -1;
int last_sent_direction = -1;

int GUI_data = 0;
int RFID_data = 0;
int station_number_data = 0;

unsigned long previousMillis = 0;

bool new_GUI_data = false;
bool new_RFID_data = false;

String lastGUIData = "";
String lastRFIDData = "";
String GUI_text = "";
String RFID_text = "";
String Motion_text = "Train Parked";
String Speed_text = "Speed = 0";

// Display timeout flags and timestamps
bool clearGuiDisplayPending = false;
bool clearRfidDisplayPending = false;
bool clearMotionDisplayPending = false;
bool clearSpeedDisplayPending = false;
bool clearSwitchDisplayPending = false;

unsigned long guiDisplayClearTime = 0;
unsigned long rfidDisplayClearTime = 0;
unsigned long motionDisplayClearTime = 0;
unsigned long speedDisplayClearTime = 0;
unsigned long switchDisplayClearTime = 0;

// Timer callback data structures
struct GuiDisplayTimerData {
    int buttonX;
    int originalY;
    int buttonWidth;
    int buttonHeight;
    int cornerRadius;
    uint16_t darkerBlue;
    String header;
    int textSizeHeader;
    int headerX;
    int headerY;
};

struct RfidDisplayTimerData {
    int buttonX;
    int originalY;
    int buttonWidth;
    int buttonHeight;
    int cornerRadius;
    uint16_t redBackgroundColor;
    String header;
    int textSizeHeader;
    int headerX;
    int headerY;
};

// Global callback data structures
GuiDisplayTimerData guiTimerData;
RfidDisplayTimerData rfidTimerData;

// For delayed commands
char dccCommandBuffer[64];
int delayedGuiData = 0;

//============================================================
// Display and Timer Setup
//============================================================
GigaDisplay_GFX tft;
SimpleTimer timer;

//============================================================
// Function Declarations 
//============================================================
void parseIncomingData();
void processGuiData();
void processRfidData();
void processSoundEffects();
void sendDataDccEX(const char* data, unsigned long int delay = 0);
void sendDataGui(int data, unsigned long int delay = 0);
void updateGuiDisplay(String text);
void updateRfidDisplay(String text);
void updateMotionDisplay(String text);
void updateSpeedDisplay(String text);
void updateSwitchDisplay();
void drawGuiHeaderOnly();
void drawRfidHeaderOnly();
void drawMotionHeaderOnly();
void drawSpeedHeaderOnly();
void drawSwitchHeaderOnly();

// Callback functions that replace lambdas
void guiDisplayTimerCallback();
void rfidDisplayTimerCallback();
void delayedDccCommand();
void delayedGuiCommand();

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

    // Register proper callbacks for timers
    timer.setTimer(1500, guiDisplayTimerCallback, 1);
    timer.setTimer(1500, rfidDisplayTimerCallback, 1);

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
    updateSpeedDisplay(Speed_text);
    updateSwitchDisplay();
}

//============================================================
// Main Loop
//============================================================
void loop() {
    parseIncomingData();
    timer.run();

    if (clearGuiDisplayPending && millis() >= guiDisplayClearTime) {
        drawGuiHeaderOnly();
    }

    if (clearRfidDisplayPending && millis() >= rfidDisplayClearTime) {
        drawRfidHeaderOnly();
    }

    if (clearMotionDisplayPending && millis() >= motionDisplayClearTime) {
        drawMotionHeaderOnly();
    }

    if (clearSpeedDisplayPending && millis() >= speedDisplayClearTime) {
        drawSpeedHeaderOnly();
    }

    if (clearSwitchDisplayPending && millis() >= switchDisplayClearTime) {
        drawSwitchHeaderOnly();
    }
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
    if (GUI_data == GUI_LIGHTS_ON || GUI_data == GUI_LIGHTS_OFF) {
        light_switch = (GUI_data == GUI_LIGHTS_ON) ? 1 : 0;
        GUI_text = light_switch ? "Lights ON" : "Lights OFF";
        sendDataDccEX(light_switch ? "<F 3 0 1>" : "<F 3 0 0>");
    }

    if (GUI_data == GUI_SMOKE_ON || GUI_data == GUI_SMOKE_OFF) {
        smoke_switch = (GUI_data == GUI_SMOKE_ON) ? 1 : 0;
        GUI_text = smoke_switch ? "Smoke ON" : "Smoke OFF";
        sendDataDccEX(smoke_switch ? "<F 3 4 1>" : "<F 3 4 0>");
    }

    if (GUI_data == GUI_SOUND_ON || GUI_data == GUI_SOUND_OFF) {
        GUI_text = (GUI_data == GUI_SOUND_ON) ? "Sound ON" : "Sound OFF";
        sendDataDccEX((GUI_data == GUI_SOUND_ON) ? "<F 3 8 1>" : "<F 3 8 0>");
    }

    if (GUI_data == GUI_RFID_ON || GUI_data == GUI_RFID_OFF) {
        RFID_sensor = (GUI_data == GUI_RFID_ON) ? 1 : 0;
        GUI_text = RFID_sensor ? "RFID ON" : "RFID OFF";
    }

    if (GUI_data == GUI_PARK_ON || GUI_data == GUI_PARK_OFF) {
        park_switch = (GUI_data == GUI_PARK_ON) ? 1 : 0;
        GUI_text = park_switch ? "Park ON" : "Park OFF";
    }

    if (GUI_data == GUI_DIRECTION_FORWARD || GUI_data == GUI_DIRECTION_REVERSE) {
        train_direction = (GUI_data == GUI_DIRECTION_FORWARD) ? 1 : 0;
        GUI_text = train_direction ? "Direction - FORWARD" : "Direction - REVERSE";
    }

    if (GUI_data == GUI_SPEED_FASTER || GUI_data == GUI_SPEED_SLOWER) {
        train_speed += (GUI_data == GUI_SPEED_FASTER) ? 1 : -1;
        GUI_text = (GUI_data == GUI_SPEED_FASTER) ? "Speed FASTER" : "Speed SLOWER";
    }

    if (train_speed >= 10) train_speed = 10;
    if (train_speed <= 0) train_speed = 0;

    if (GUI_data == GUI_STOP) {
        train_speed = 0;
        GUI_text = "Train STOP";
        sendDataDccEX("<t 03 0 0>");
        updateMotionDisplay("Train STOPPED");
        updateSpeedDisplay("");
    }

    if (train_speed != last_sent_speed || train_direction != last_sent_direction) {
        int speedValue = (train_speed == 0) ? -1 : train_speed * 10;

        char speedCmd[30];
        sprintf(speedCmd, "<t 03 %d %d>", speedValue, train_direction);
        sendDataDccEX(speedCmd);
        sendDataGui(train_speed);

        String motionText = (train_speed == 0)
            ? "Train STOPPED"
            : (train_direction == 1 ? "Train Moving FORWARD" : "Train Moving REVERSE");
        String speedText = (train_speed == 0)
            ? "Speed = 0"
            : "Speed = " + String(train_speed * 10) + "%";

        updateMotionDisplay(motionText);
        updateSpeedDisplay(speedText);

        last_sent_speed = train_speed;
        last_sent_direction = train_direction;
    }

    if (train_speed != train_speed_hold) {
        sendDataGui(((train_speed * 10) + 100), 0);
        train_speed_hold = train_speed;
    }

    processSoundEffects();
    updateSwitchDisplay();
}

//============================================================
// Sound Effects Processing
//============================================================
void processSoundEffects() {
    struct SoundEffect {
        int code;
        const char* cmdOn;
        const char* cmdOff;
        const char* label;
    };

    static const SoundEffect AM_SOUNDS[] = {
        {GUI_AM_BELL,              "<F 3 7 1>",  "<F 3 7 0>",  "Bell"},
        {GUI_AM_LONG_WHISTLE,      "<F 3 2 1>",  "<F 3 2 0>",  "Long whistles"},
        {GUI_AM_WHISTLE,           "<F 3 9 1>",  "<F 3 9 0>",  "Whistle"},
        {GUI_AM_SHORT_WHISTLE,     "<F 3 4 1>",  "<F 3 4 0>",  "Short whistle"},
        {GUI_AM_WHEELS_SQUEALING,  "<F 3 9 1>",  "<F 3 9 0>",  "Wheels squealing"},
        {GUI_AM_SHOVELING_COAL,    "<F 3 10 1>", "<F 3 10 2>", "Shoveling coal"},
        {GUI_AM_BLOWER_SOUND,      "<F 3 11 1>", "<F 3 11 0>", "Blower sound"},
        {GUI_AM_COUPLER,           "<F 3 12 1>", "<F 3 12 0>", "Coupler/Uncoupler"},
        {GUI_AM_COAL_DROPPING,     "<F 3 13 1>", "<F 3 13 0>", "Coal dropping"},
        {GUI_AM_STEAM_VENTING,     "<F 3 13 1>", "<F 3 13 0>", "Steam venting"},
        {GUI_AM_CONDUCTOR_CALLING, "<F 3 17 1>", "<F 3 17 0>", "Conductor calling"},
        {GUI_AM_GENERATOR,         "<F 3 18 1>", "<F 3 18 2>", "Generator sound"},
        {GUI_AM_AIR_PUMP,          "<F 3 19 1>", "<F 3 19 0>", "Air pump"},
        {GUI_AM_COAL_UNLOADING,    "<F 3 20 1>", "<F 3 20 0>", "Coal unloading"},
        {GUI_AM_WATER_FILLING_TANK,"<F 3 21 1>", "<F 3 21 0>", "Water filling tank"}
    };

    static const SoundEffect K3_SOUNDS[] = {
        {GUI_K3_CONDUCTOR_CALLING, "<F 3 2 1>",  "<F 3 2 0>",  "Conductor calling"},
        {GUI_K3_CONDUCTOR_WHISTLE, "<F 3 3 1>",  "<F 3 3 0>",  "Conductor's whistle"},
        {GUI_K3_COAL_SHOVELING,    "<F 3 5 1>",  "<F 3 5 0>",  "Coal shoveling"},
        {GUI_K3_INJECTOR,          "<F 3 6 1>",  "<F 3 6 0>",  "Injector sound"},
        {GUI_K3_BELL,              "<F 3 7 1>",  "<F 3 7 0>",  "Bell"},
        {GUI_K3_WHISTLE,           "<F 3 9 1>",  "<F 3 9 0>",  "Whistle"},
        {GUI_K3_COUPLER,           "<F 3 10 1>", "<F 3 10 0>", "Coupler / Uncoupler"},
        {GUI_K3_AIR_PUMP,          "<F 3 11 1>", "<F 3 11 0>", "Air pump"},
        {GUI_K3_BOILER,            "<F 3 12 1>", "<F 3 12 0>", "Boiler sound"},
        {GUI_K3_STEAM_VENTING,     "<F 3 13 1>", "<F 3 13 0>", "Steam venting"}
    };

    const SoundEffect* table = (Train_selection == 0) ? AM_SOUNDS : K3_SOUNDS;
    size_t tableSize = (Train_selection == 0) ? sizeof(AM_SOUNDS) / sizeof(SoundEffect)
                                              : sizeof(K3_SOUNDS) / sizeof(SoundEffect);

    for (size_t i = 0; i < tableSize; i++) {
        if (GUI_data == table[i].code) {
            // Debug output
            SerialUSB.print("Processing sound: ");
            SerialUSB.println(table[i].label);
            
            // Explicitly send commands with direct timing
            Serial3.println(table[i].cmdOn);
            
            // Store the command for delayed execution
            strncpy(dccCommandBuffer, table[i].cmdOff, sizeof(dccCommandBuffer) - 1);
            dccCommandBuffer[sizeof(dccCommandBuffer) - 1] = '\0';  // Ensure null termination
            
            // Create a timer for the sound delay
            timer.setTimeout(SOUND_DELAY, delayedDccCommand);
            
            GUI_text = table[i].label;
            break;
        }
    }
}

void processRfidData() {
    struct RfidAction {
        int code;
        const char* text;
        const char* cmdOn;
        bool isSpeedCommand;
        int speedValue;
    };

    static const RfidAction AM_SOUNDS[] = {
        {RFID_LONG_WHISTLE_AM, "Long Whistles", "<F 3 2 1>", false, 0},
        {RFID_WHISTLE_AM,      "Whistle",       "<F 3 3 1>", false, 0},
        {RFID_SHORT_WHISTLE_AM,"Short Whistle", "<F 3 4 1>", false, 0},
        {RFID_BELL_AM,         "Bell",          "<F 3 1 1>", false, 0}
    };

    static const RfidAction K3_SOUNDS[] = {
        {RFID_WHISTLE_K3, "Whistle", "<F 3 9 1>", false, 0},
        {RFID_BELL_K3,    "Bell",    "<F 3 7 1>", false, 0}
    };

    static const RfidAction STATIONS[] = {
        {RFID_STATION_1, "Station 1", nullptr, false, 0},
        {RFID_STATION_2, "Station 2", nullptr, false, 0},
        {RFID_STATION_3, "Station 3", nullptr, false, 0},
        {RFID_STATION_4, "Station 4", nullptr, false, 0},
        {RFID_STATION_5, "Station 5", nullptr, false, 0},
        {RFID_STATION_6, "Station 6", nullptr, false, 0}
    };

    static const RfidAction SPEEDS[] = {
        {RFID_SPEED_20, "Speed 20%", "<t 1 03 20 1>", true, SPEED_20},
        {RFID_SPEED_40, "Speed 40%", "<t 1 03 40 1>", true, SPEED_40},
        {RFID_SPEED_60, "Speed 60%", "<t 1 03 60 1>", true, SPEED_60},
        {RFID_SPEED_80, "Speed 80%", "<t 1 03 80 1>", true, SPEED_80},
        {RFID_SPEED_100,"Speed 100%","<t 1 03 100 1>", true, SPEED_100}
    };

    // Process speeds
    for (const auto& s : SPEEDS) {
        if (RFID_data == s.code) {
            RFID_text = s.text;
            train_speed = s.speedValue;
            sendDataDccEX(s.cmdOn);
            break;
        }
    }

    // Process stations
    for (const auto& station : STATIONS) {
        if (RFID_data == station.code) {
            RFID_text = station.text;
            sendDataGui(station.code);
            break;
        }
    }

    // Process parking
    if (park_switch == 1 && train_direction == 1) {
        if (RFID_data == RFID_PARK_1) {
            RFID_text = "Speed 50%";
            train_speed = SPEED_PARK_1;
            updateMotionDisplay("Train Parking 1");
            sendDataDccEX("<t 1 03 50 1>");
        }
        else if (RFID_data == RFID_PARK_2) {
            RFID_text = "Speed 30%";
            train_speed = SPEED_PARK_2;
            updateMotionDisplay("Train Parking 2");
            sendDataDccEX("<t 1 03 30 1>");
        }
        else if (RFID_data == RFID_PARKED) {
            RFID_text = "Parked";
            train_speed = 0;
            updateMotionDisplay("Train Parked");
            sendDataDccEX("<t 1 03 -1 1>");
        }
    }

    // Process sounds based on train selection
    bool soundProcessed = false;
    if (Train_selection == 0) {
        for (const auto& sound : AM_SOUNDS) {
            if (RFID_data == sound.code) {
                RFID_text = sound.text;
                sendDataDccEX(sound.cmdOn);
                sendDataDccEX(sound.cmdOn, SOUND_DELAY);
                soundProcessed = true;
                break;
            }
        }
    } else if (Train_selection == 1) {
        for (const auto& sound : K3_SOUNDS) {
            if (RFID_data == sound.code) {
                RFID_text = sound.text;
                sendDataDccEX(sound.cmdOn);
                sendDataDccEX(sound.cmdOn, SOUND_DELAY);
                soundProcessed = true;
                break;
            }
        }
    }

    Speed_text = "Speed = " + String(train_speed * 10) + "%";
    updateSpeedDisplay(Speed_text);
}

//============================================================
// Display Update Functions
//============================================================

// Struct to store parameters for timer callbacks
struct GuiDisplayParams {
    int width;
    int buttonX;
    int originalY;
    int buttonWidth;
    int buttonHeight;
    int cornerRadius;
    uint16_t buttonColor;
    String header;
    int textSizeHeader;
    int headerX;
    int headerY;
};

// Global structs for storing callback parameters
GuiDisplayParams guiParams;
GuiDisplayParams rfidParams;

// Timer callback functions
void clearGuiDisplayCallback() {
    // Redraw the button
    tft.fillRoundRect(guiParams.buttonX, guiParams.originalY, guiParams.buttonWidth, guiParams.buttonHeight, 
                     guiParams.cornerRadius, guiParams.buttonColor);
    tft.drawRoundRect(guiParams.buttonX, guiParams.originalY, guiParams.buttonWidth, guiParams.buttonHeight, 
                     guiParams.cornerRadius, GC9A01A_WHITE);
    
    // Redraw only the header text
    tft.setTextSize(guiParams.textSizeHeader);
    tft.setTextColor(GC9A01A_WHITE);
    tft.setCursor(guiParams.headerX, guiParams.headerY);
    tft.print(guiParams.header);
}

void clearRfidDisplayCallback() {
    // Redraw the button
    tft.fillRoundRect(rfidParams.buttonX, rfidParams.originalY, rfidParams.buttonWidth, rfidParams.buttonHeight, 
                     rfidParams.cornerRadius, rfidParams.buttonColor);
    tft.drawRoundRect(rfidParams.buttonX, rfidParams.originalY, rfidParams.buttonWidth, rfidParams.buttonHeight, 
                     rfidParams.cornerRadius, GC9A01A_WHITE);
    
    // Redraw only the header text
    tft.setTextSize(rfidParams.textSizeHeader);
    tft.setTextColor(GC9A01A_WHITE);
    tft.setCursor(rfidParams.headerX, rfidParams.headerY);
    tft.print(rfidParams.header);
}

// Update timer callbacks to directly use the clear functions
void guiDisplayTimerCallback() {
    drawGuiHeaderOnly();
}

void rfidDisplayTimerCallback() {
    drawRfidHeaderOnly();
}

// Global variables for delayed command execution
char dccExCommandBuffer[64];

// Add the callback function that was previously a lambda
void delayedDccCommand() {
    Serial3.println(dccCommandBuffer);
    SerialUSB.print("Delayed command sent: ");
    SerialUSB.println(dccCommandBuffer);
}

// Callback function for delayed DCC-EX command
void sendDelayedDccExCommand() {
    Serial3.print(dccExCommandBuffer);
}

// Callback function for delayed GUI command

void sendDelayedGuiCommand() {
    Serial4.write(delayedGuiData);
}

void updateGuiDisplay(String text) {
    int width = tft.width();
    int textSizeHeader = 3;
    int textSizeBody = 3;
    
    // Button specifications
    int standardButtonHeight = 63;    // Height of buttons in previous functions
    int verticalSpacing = 20;         // Spacing used in previous buttons
    int buttonHeight = 2 * standardButtonHeight + verticalSpacing; // Height of two buttons plus spacing
    int margin = 20;                  // Same margin as in updateSwitchDisplay
    int singleButtonWidth = 220;      // Width of a single button from updateSwitchDisplay
    int buttonWidth = 2 * singleButtonWidth + margin; // Width of two buttons plus margin
    int cornerRadius = 8;             // Same radius for rounded corners
    
    // Calculate the horizontal center position
    int buttonX = (width - buttonWidth) / 2;
    
    // Position the top button 20 pixels higher than before
    int distanceFromEdge = 800 - (650 + standardButtonHeight);
    int topButtonY = distanceFromEdge - 20; // Moved up 20 pixels
    
    // Use the same darker blue color as in SwitchDisplay
    uint16_t darkerBlue = tft.color565(0, 0, 80); // Darker navy blue matching SwitchDisplay
    
    // Clear the area
    tft.fillRect(0, topButtonY, width, buttonHeight, GC9A01A_BLACK);
    
    // Draw the darker blue button with white border
    tft.fillRoundRect(buttonX, topButtonY, buttonWidth, buttonHeight, cornerRadius, darkerBlue);
    tft.drawRoundRect(buttonX, topButtonY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
    
    // Set text properties
    tft.setTextWrap(false);
    tft.setTextColor(GC9A01A_WHITE);
    
    // Draw header text
    String header = "= GUI Command =";
    tft.setTextSize(textSizeHeader);
    
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(header, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    int headerX = buttonX + (buttonWidth - textWidth) / 2;
    int headerY = topButtonY + buttonHeight/4; // Position at 1/4 of the button height
    
    tft.setCursor(headerX, headerY);
    tft.print(header);
    
    // Draw command text
    tft.setTextSize(textSizeBody);
    tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    int commandX = buttonX + (buttonWidth - textWidth) / 2;
    int commandY = topButtonY + buttonHeight * 5/8; // Changed from 3/4 to 5/8 of the button height
    
    tft.setCursor(commandX, commandY);
    tft.print(text);
    
    // Store original Y position for the timer callback
    int originalY = topButtonY;
    
    // Store parameters for the timer callback
    guiParams.width = width;
    guiParams.buttonX = buttonX;
    guiParams.originalY = originalY;
    guiParams.buttonWidth = buttonWidth;
    guiParams.buttonHeight = buttonHeight;
    guiParams.cornerRadius = cornerRadius;
    guiParams.buttonColor = darkerBlue;
    guiParams.header = header;
    guiParams.textSizeHeader = textSizeHeader;
    guiParams.headerX = headerX;
    guiParams.headerY = headerY;
    
    // Set timeout to clear the command text but keep the button and header
    timer.setTimeout(1500, clearGuiDisplayCallback);

    clearGuiDisplayPending = true;
    guiDisplayClearTime = millis() + 1500;

    // Replace the lambda with a properly defined function
    if (!text.isEmpty()) {
        clearGuiDisplayPending = true;
        guiDisplayClearTime = millis() + 1500;
    }
}

void updateRfidDisplay(String text) {
    int width = tft.width();
    int textSizeHeader = 3;
    int textSizeBody = 3;
    
    // Button specifications
    int standardButtonHeight = 63;    // Height of buttons in previous functions
    int verticalSpacing = 20;         // Spacing used in previous buttons
    int buttonHeight = 2 * standardButtonHeight + verticalSpacing; // Height of two buttons plus spacing
    int margin = 20;                  // Same margin as in updateSwitchDisplay
    int singleButtonWidth = 220;      // Width of a single button from updateSwitchDisplay
    int buttonWidth = 2 * singleButtonWidth + margin; // Width of two buttons plus margin
    int cornerRadius = 8;             // Same radius for rounded corners
    
    // Calculate the horizontal center position
    int buttonX = (width - buttonWidth) / 2;
    
    // Get position of first button
    int distanceFromEdge = 800 - (650 + standardButtonHeight);
    int topButtonY = distanceFromEdge - 20; // First button moved up 20 pixels
    
    // Position the second button below the first with the same spacing
    int bottomButtonY = topButtonY + buttonHeight + verticalSpacing;
    
    // Red background color
    uint16_t redBackgroundColor = tft.color565(160, 0, 0); // Deep red color
    
    // Clear the area
    tft.fillRect(0, bottomButtonY, width, buttonHeight, GC9A01A_BLACK);
    
    // Draw the red button with white border
    tft.fillRoundRect(buttonX, bottomButtonY, buttonWidth, buttonHeight, cornerRadius, redBackgroundColor);
    tft.drawRoundRect(buttonX, bottomButtonY, buttonWidth, buttonHeight, cornerRadius, GC9A01A_WHITE);
    
    // Set text properties
    tft.setTextWrap(false);
    tft.setTextColor(GC9A01A_WHITE);
    
    // Draw header text
    String header = "= RFID Command =";
    tft.setTextSize(textSizeHeader);
    
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds(header, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    int headerX = buttonX + (buttonWidth - textWidth) / 2;
    int headerY = bottomButtonY + buttonHeight/4; // Position at 1/4 of the button height
    
    tft.setCursor(headerX, headerY);
    tft.print(header);
    
    // Draw command text
    tft.setTextSize(textSizeBody);
    tft.getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    int commandX = buttonX + (buttonWidth - textWidth) / 2;
    int commandY = bottomButtonY + buttonHeight * 5/8; // Changed from 3/4 to 5/8 of the button height
    
    tft.setCursor(commandX, commandY);
    tft.print(text);
    
    // Store original Y position for the timer callback
    int originalY = bottomButtonY;
    
    // Store parameters for the timer callback
    rfidParams.width = width;
    rfidParams.buttonX = buttonX;
    rfidParams.originalY = originalY;
    rfidParams.buttonWidth = buttonWidth;
    rfidParams.buttonHeight = buttonHeight;
    rfidParams.cornerRadius = cornerRadius;
    rfidParams.buttonColor = redBackgroundColor;
    rfidParams.header = header;
    rfidParams.textSizeHeader = textSizeHeader;
    rfidParams.headerX = headerX;
    rfidParams.headerY = headerY;
    
    // Set timeout to clear the command text but keep the button and header
    timer.setTimeout(1500, clearRfidDisplayCallback);

    clearRfidDisplayPending = true;
    rfidDisplayClearTime = millis() + 1500;
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
    int buttonY = 408;
    
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

    if (train_speed == 0) {
        clearMotionDisplayPending = true;
        motionDisplayClearTime = millis() + 1500;
    } else {
        clearMotionDisplayPending = false;  // Don't clear motion display if train is moving
    }
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
    // The button in updateMotionDisplay is at y=408
    // Use the same vertical spacing as in updateSwitchDisplay (which was 20 pixels)
    int motionButtonY = 408;
    int speedButtonY = motionButtonY + buttonHeight + 20; // 20 is the vertical gap
    
    // Changed to green background color
    uint16_t greenColor = tft.color565(0, 150, 0);
    
    // Clear the area
    tft.fillRect(0, speedButtonY, width, buttonHeight, GC9A01A_BLACK);
    
    // Draw the green button with white border
    tft.fillRoundRect(buttonX, speedButtonY, buttonWidth, buttonHeight, cornerRadius, greenColor);
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
    tft.setTextColor(GC9A01A_WHITE);
    tft.setCursor(textX, textY);
    tft.print(text);

    if (train_speed == 0) {
        clearSpeedDisplayPending = true;
        speedDisplayClearTime = millis() + 1500;
    } else {
        clearSpeedDisplayPending = false;  // Don't clear motion display if train is moving
    }
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

    clearSwitchDisplayPending = true;
    switchDisplayClearTime = millis() + 1500;
}

void drawGuiHeaderOnly() {
    // Add minimal header redraw logic for GUI
    updateGuiDisplay("");  // reuse existing logic
    clearGuiDisplayPending = false;
}

void drawRfidHeaderOnly() {
    updateRfidDisplay("");  // reuse existing logic
    clearRfidDisplayPending = false;
}

void drawMotionHeaderOnly() {
    updateMotionDisplay(Motion_text);  // or "" if you want it cleared
    clearMotionDisplayPending = false;
}

void drawSpeedHeaderOnly() {
    updateSpeedDisplay(Speed_text);  // or "" to clear
    clearSpeedDisplayPending = false;
}

void drawSwitchHeaderOnly() {
    // Simply redraw current switch states
    updateSwitchDisplay();
    clearSwitchDisplayPending = false;
}

//============================================================
// Communication Functions
//============================================================
void sendDataDccEX(const char* data, unsigned long delay) {
    if (delay == 0) {
        Serial3.print(data);
    } else {
        // Copy data to buffer for delayed sending
        strncpy(dccExCommandBuffer, data, sizeof(dccExCommandBuffer) - 1);
        dccExCommandBuffer[sizeof(dccExCommandBuffer) - 1] = '\0';  // Ensure null termination
        
        // Set timeout for delayed sending
        timer.setTimeout(delay, sendDelayedDccExCommand);
    }
}

void sendDataGui(int data, unsigned long delay) {
    if (delay == 0) {
        Serial4.write(data);
    } else {
        // Store data for delayed sending
        delayedGuiData = data;
        
        // Set timeout for delayed sending
        timer.setTimeout(delay, sendDelayedGuiCommand);
    }
}