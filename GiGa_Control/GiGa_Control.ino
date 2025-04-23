//State Variables
int active = 0;
int GUI_data = 0;  
int RFID_data = 0;
int RFID_sensor = 0;
int park_switch = 0;
int RFID_READY = 0;
int sound_switch = 1;
int light_switch = 0;
int smoke_switch = 0;
int train_direction = 1;
int train_speed = 0;
int train_speed_hold = 0;
int speed_switch = 0;
int station_number_data = 0;
int last_sent_speed = -1;
int last_sent_direction = -1;

unsigned long previousMillis = 0;

bool new_GUI_data = false;
bool new_RFID_data = false;

// Display variables
String lastGUIData = "";
String lastRFIDData = "";
String GUI_text = "";
String RFID_text = "";

// Include libraries for the display
#include "Arduino_GigaDisplay_GFX.h"
GigaDisplay_GFX tft;

#include <SimpleTimer.h>
SimpleTimer timer;

#define Sound_Delay 1000                      // Delay after which to cancel sound
#define SD 500 

//Serial connections
//Serial2 is RFID ESP-32 communication
//Serial3 is DCC-EX communication
//Serial4 is GUI ESP-32 communication

#define GC9A01A_CYAN    0x07FF
#define GC9A01A_RED     0xf800
#define GC9A01A_BLUE    0x001F
#define GC9A01A_GREEN   0x07E0
#define GC9A01A_MAGENTA 0xF81F
#define GC9A01A_WHITE   0xffff
#define GC9A01A_BLACK   0x0000
#define GC9A01A_YELLOW  0xFFE0

//Helper Functions
void parseIncomingData();
//Sends data to DCC-EX over Serial3
void sendDataDccEX(char data[], unsigned long int delay = 0);
//Sends data to Gui Board over Serial2
void sendDataGui(int data, unsigned long int delay = 0);
// Update display with received data
void updateDisplay();

void setup() {
 
    pinMode(A4, OUTPUT);                      // Connected to GUI ESP-32 pin 32
    pinMode(A5, INPUT);                       // Connected to GUI ESP-32 pin 15
    pinMode(A6, INPUT);                      // Connected to RFID ESP-32 pin 15
    pinMode(A7, OUTPUT);                       // Connected to RFID ESP-32 pin 32
  
    SerialUSB.begin(115200);
    Serial2.begin(115200);   
    Serial3.begin(115200);
    Serial4.begin(115200);

  
    tft.begin();
    tft.setRotation(0);  
    tft.fillScreen(GC9A01A_BLACK);


    delay(2000);

    Serial3.println("<0>");
    delay(500);
    Serial3.println("<1>");
    delay(500);
    Serial3.println("<F 3 8 1>");

    delay(5000);

    Serial3.println("<F 3 2 1>");            // K3 Conductor calling
    delay(2000);
    Serial3.println("<F 3 2 0>");            // Cancel Conductor calling after Sound_Delay

}


void loop() {
  parseIncomingData();
  timer.run();
}

void parseIncomingData() {
  
  // Read GUI data
  if (Serial4.available()) {
    uint8_t guiChar = Serial4.read();  
    GUI_data = (int)guiChar;

    new_GUI_data = true;
   
    SerialUSB.print("GUI Data Received: ");
    SerialUSB.print(GUI_data);
    SerialUSB.println();
  
  }

  if (new_GUI_data) {

    if (GUI_data == 111) {
      GUI_text = "Lights ON";
      sendDataDccEX("<F 3 0 1>");                       // Lights ON / OFF
    } else if (GUI_data == 110) {
        GUI_text = "Lights OFF";
        sendDataDccEX("<F 3 0 0>");
      }

    if (GUI_data == 121){                               // Smoke ON / OFF
      GUI_text = "Smoke ON";
      sendDataDccEX("<F 3 4 1>");                       
    } else if (GUI_data == 120) {
        GUI_text = "Smoke OFF";
        sendDataDccEX("<F 3 4 0>");
      } 

    if (GUI_data == 131) {                               // Sound ON / OFF 
      GUI_text = "Sound ON";        
      sendDataDccEX("<F 3 8 1>");                       
    } else if (GUI_data == 130) {
        GUI_text = "Sound OFF";
        sendDataDccEX("<F 3 8 0>");
      }

    if (GUI_data == 141) {                              // RFID sensor ON / OFF 
      GUI_text = "RFID ON";
      RFID_sensor = 1;
      }             
      else if (GUI_data == 140) {
        GUI_text = "RFID OFF";
        RFID_sensor = 0;
      }

    if (GUI_data == 151) {                              // Park_switch ON / OFF
      GUI_text = "Park ON";
      park_switch = 1;}             
      else if (GUI_data == 150) {
        GUI_text = "Park OFF";
        park_switch = 0;}

    if (GUI_data == 161) {                              // Direction FWD / REV                            
      GUI_text = "Direction - FORWARD";
      train_direction = 1;}
      else if (GUI_data == 160) {
        GUI_text = "Direction - REVERSE";
        train_direction = 0;}  


  //  GUI initiated speed control (Start / Faster speed, Slower speed, Stop) - PUSH BUTTON control

    if (GUI_data == 7) {                                
      GUI_text = "Speed FASTER ";                // Train Start / Faster /Slower
      train_speed = train_speed + 1;}     
    if (GUI_data == 8) {
      GUI_text = "Speed SLOWER"; 
      train_speed = train_speed - 1;} 

    // Clamp train speed before any checks
if (train_speed >= 10) train_speed = 10;
if (train_speed <= 0) train_speed = 0;

// Only send speed command if speed or direction changed
if (train_speed != last_sent_speed || train_direction != last_sent_direction) {
  char speedCmd[30];
  int speedValue = (train_speed == 0) ? -1 : train_speed * 10;

  sprintf(speedCmd, "<t 03 %d %d>", speedValue, train_direction);
  sendDataDccEX(speedCmd);
  sendDataGui(train_speed);
  SerialUSB.println(train_speed);

  last_sent_speed = train_speed;
  last_sent_direction = train_direction;
}


    // Train stop
    if (GUI_data == 9) {                        // Train stop
      sendDataDccEX("<t 03 0 0>");
      train_speed = 0;
      }  


    if (GUI_data == 41) {
      sendDataDccEX("<F 3 7 1>");                         // AM F1 Bell
      sendDataDccEX("<F 3 7 0>", Sound_Delay);            // Cancel Bell after Sound_Delay
      GUI_text = "Bell";
    }

    if (GUI_data == 42) {
      sendDataDccEX("<F 3 2 1>");                         // AM F2 Long whistles
      sendDataDccEX("<F 3 2 0>", Sound_Delay);            // Cancel long whistles after Sound_Delay
      GUI_text = "Long whistles";
    }

    if (GUI_data == 43) {
      Serial3.print("<F 3 9 1>");                         // AM F3 Whistle
      Serial3.print("<F 3 9 0>");                         // Cancel train whistle after Sound_Delay
      delay(1000);
    }

    if (GUI_data == 44) {
      sendDataDccEX("<F 3 4 1>");                         // AM F4 Short whistle
      sendDataDccEX("<F 3 4 0>", Sound_Delay);            // Cancel short whistle after Sound_Delay
    }

    if (GUI_data == 45) {}                                // AM F5 Smoke ON/OFF via Smoke switch (V2)

    if (GUI_data == 46) {}                                // AM Open F6

    if (GUI_data == 47) {}                                // AM Open F7

    if (GUI_data == 48) {}                                // AM F8 Sound ON/OFF via Sound switch (V5)

    if (GUI_data == 49) {
      sendDataDccEX("<F 3 9 1>");                         // AM F9 Wheels squealing
      sendDataDccEX("<F 3 9 0>", Sound_Delay);            // Cancel wheels squealing after Sound_Delay
    }

    if (GUI_data == 50) {
      sendDataDccEX("<F 3 10 1>");                        // AM 50 Shoveling coal
      sendDataDccEX("<F 3 10 2>", Sound_Delay);           // Cancel shoveling coal after Sound_Delay
    }

    if (GUI_data == 51) {
      sendDataDccEX("<F 3 11 1>");                        // AM Blower sound
      sendDataDccEX("<F 3 11 0>", Sound_Delay);           // Cancel blower after Sound_Delay
    }

    if (GUI_data == 52) {
      sendDataDccEX("<F 3 12 1>");                        // AM Coupler/Uncoupler
      sendDataDccEX("<F 3 12 0>", Sound_Delay);           // Cancel coupler/uncoupler after Sound_Delay
    }

    if (GUI_data == 53) {
      sendDataDccEX("<F 3 13 1>");                        // AM Coal dropping
      sendDataDccEX("<F 3 13 0>", Sound_Delay);           // Cancel coal dropping after Sound_Delay
    }

    if (GUI_data == 54) {
      sendDataDccEX("<F 3 13 1>");                        // AM Steam venting
      sendDataDccEX("<F 3 13 0>", Sound_Delay);           // Cancel steam venting after Sound_Delay
    }

    if (GUI_data == 55) {}                                // AM Open F15

    if (GUI_data == 56) {}                                // AM Open F16

    if (GUI_data == 57) {
      sendDataDccEX("<F 3 17 1>");                        // AM F17 Conductor calling "All Aboard"
      sendDataDccEX("<F 3 17 0>", Sound_Delay);           // Cancel conductor calling after Sound_Delay
    }

    if (GUI_data == 58) {
      sendDataDccEX("<F 3 18 1>");                        // AM F18 Generator sound
      sendDataDccEX("<F 3 18 2>", Sound_Delay);           // Cancel generator sound after Sound_Delay
    }

    if (GUI_data == 59) {
      sendDataDccEX("<F 3 19 1>");                        // AM F19 Air pump
      sendDataDccEX("<F 3 19 0>", Sound_Delay);           // Cancel air pump after Sound_Delay
    }

    if (GUI_data == 60) {
      sendDataDccEX("<F 3 20 1>");                        // AM F20 Coal unloading into hopper
      sendDataDccEX("<F 3 20 0>", Sound_Delay);           // Cancel coal unloading after Sound_Delay
    }

    if (GUI_data == 61) {
      sendDataDccEX("<F 3 21 1>");                        // AM F21 Water filling tank
      sendDataDccEX("<F 3 21 0>", Sound_Delay);           // Cancel water filling tank after Sound_Delay
    }

    if (GUI_data == 62) {}                                // AM Open F22

    if (GUI_data == 63) {}                                // AM Open F23

    if (GUI_data == 64) {}                                // AM Open F24


  // K3 Stainz (K3) Sounds/Functions triggered by GUI input - Push button control

    if (GUI_data == 65) {}                                // K3 F0 Lights ON/OFF toggle via AM F0

    if (GUI_data == 66) {}                                // K3 Open F1

    if (GUI_data == 67) {
      GUI_text = "Conductor calling";
      tft.setCursor(0,100);
      tft.setTextSize(4);
      tft.setTextColor(GC9A01A_BLUE);    
      tft.println(GUI_text);
      sendDataDccEX("<F 3 2 1>");                         // K3 F2 Conductor calling "All Aboard"
      sendDataDccEX("<F 3 2 0>", Sound_Delay);            // Cancel conductor calling after Sound_Delay

    }

    if (GUI_data == 68) {
      GUI_text = "Conductor's whistle";
      sendDataDccEX("<F 3 3 1>");                         // K3 F3 Conductor's whistle
      sendDataDccEX("<F 3 3 0>", Sound_Delay);            // Cancel conductor's whistle after Sound_Delay
      
    }

    if (GUI_data == 69) {}                                // K3 Open F4

    if (GUI_data == 70) {
      sendDataDccEX("<F 3 5 1>");                         // K3 F5 Coal shoveling
      sendDataDccEX("<F 3 5 0>", Sound_Delay);            // Cancel coal shoveling after Sound_Delay
      GUI_text = "Coal shoveling";
    }

    if (GUI_data == 71) {
      sendDataDccEX("<F 3 6 1>");                         // K3 F6 Injector sound
      sendDataDccEX("<F 3 6 0>", Sound_Delay);            // Cancel injector after Sound_Delay
      GUI_text = "Injector sound";
    }

    if (GUI_data == 72) {
      sendDataDccEX("<F 3 7 1>");                         // K3 F7 Bell
      sendDataDccEX("<F 3 7 0>", Sound_Delay);            // Cancel Bell after Sound_Delay
      GUI_text = "Bell";
    }

    if (GUI_data == 73) {}                                // K3 F8 Sound ON/OFF toggle via V3

    if (GUI_data == 74) {
      sendDataDccEX("<F 3 9 1>");                         // K3 F9 Whistle
      sendDataDccEX("<F 3 9 0>", Sound_Delay);            // Cancel Whistle after Sound_Delay
      GUI_text = "Whistle";
    } 

    if (GUI_data == 75) {
      sendDataDccEX("<F 3 10 1>");                         // K3 F10 Coupler/uncoupler
      sendDataDccEX("<F 3 10 0>", Sound_Delay);            // Cancel coupler/uncoupler after Sound_Delay
      GUI_text = "Coupler / Uncoupler";
    }

    if (GUI_data == 76) {
      sendDataDccEX("<F 3 11 1>");                         // K3 F11 Air pump
      sendDataDccEX("<F 3 11 0>", Sound_Delay);            // Cancel air pump after Sound_Delay
      GUI_text = "Air pump";
    }

    if (GUI_data == 77) {
      sendDataDccEX("<F 3 12 1>");                         // K3 F12 Boiler sound
      sendDataDccEX("<F 3 12 0>", Sound_Delay);            // Cancel boiler sound after Sound_Delay
      GUI_text = "Boiler sound";
    }

    if (GUI_data == 78) {
      sendDataDccEX("<F 3 13 1>");                         // K3 F13 Steam venting
      sendDataDccEX("<F 3 13 0>", Sound_Delay);            // Cancel steam venting after Sound_Delay
      GUI_text = "Steam venting";
    }

    if (GUI_data == 79) {}                                 // K3 Open F14

    if (train_speed != train_speed_hold) {
      sendDataGui(((train_speed * 10) + 100), 0);
      train_speed_hold = train_speed;
    }

  updateGuiDisplay(GUI_text); 

  SerialUSB.print("GUI Command: ");
  SerialUSB.print(GUI_text);
  SerialUSB.println();

  new_GUI_data = false;

  }    // End of GUI read loop

  // Read RFID data
  if (Serial2.available()) {
    uint8_t rfidChar = Serial2.read();  
    RFID_data = (int)rfidChar;

    new_RFID_data = true;

    SerialUSB.print("RFID Data Received: ");
    SerialUSB.println(RFID_data);
  }

  if (new_RFID_data) {

    if (RFID_data == 42) {                                  // Long whistles
          RFID_text = "Long Whistles";
          sendDataDccEX("<F 3 2 1>");
          sendDataDccEX("<F 3 2 0>", Sound_Delay);
        }

        if (RFID_data == 43) {                              // Whistle
          RFID_text = "Whistle";
          sendDataDccEX("<F 3 9 1>");
          sendDataDccEX("<F 3 9 0>", Sound_Delay);
        }

        if (RFID_data == 44) {                              // Short whistle
          RFID_text = "Short Whistle";
          sendDataDccEX("<F 3 4 1>");
          sendDataDccEX("<F 3 4 0>", Sound_Delay);
        }

        if (RFID_data == 41) {                              // Bell
          RFID_text = "Bell";
          sendDataDccEX("<F 3 7 1>");
          sendDataDccEX("<F 3 7 0>", Sound_Delay);
        }

        // Speed settings
        if (RFID_data == 20) {                              // Train Speed 20%
          RFID_text = "Speed 20%";
          train_speed = 2; 
          sendDataDccEX("<t 1 03 20 1>");
        }
        
        else if (RFID_data == 21) {                         // Train Speed 40%
          RFID_text = "Speed 40%";
          train_speed = 4; 
          sendDataDccEX("<t 1 03 40 1>");
        }

        else if (RFID_data == 22) {                        // Train Speed 60%
          RFID_text = "Speed 60%";
          train_speed = 6; 
          sendDataDccEX("<t 1 03 60 1>");
        }

        else if (RFID_data == 23) {                         // Train Speed 80%
          RFID_text = "Speed 80%";
          train_speed = 8; 
          sendDataDccEX("<t 1 03 80 1>");
        }

        else if (RFID_data == 24) {                         // Train Speed 100%
          RFID_text = "Speed 100%";
          train_speed = 10; 
          sendDataDccEX("<t 1 03 100 1>");
        }
        
        // Park logic
        if (park_switch == 1 && train_direction == 1) {
          if (RFID_data == 31) { 
            RFID_text = "Speed 50%";
            train_speed = 5; 
            sendDataDccEX("<t 1 03 50 1>"); }
          if (RFID_data == 32) { 
            RFID_text = "Speed 30%";
            train_speed = 3; 
            sendDataDccEX("<t 1 03 30 1>"); }
          if (RFID_data == 25) { 
            RFID_text = "Parked";
            train_speed = 0; 
            sendDataDccEX("<t 1 03 -1 1>"); }
        }

  updateRfidDisplay(RFID_text); 

  SerialUSB.print("RFID Command: ");
  SerialUSB.print(RFID_text);
  SerialUSB.println();

  new_RFID_data = false;

  }


}  // end of parseIncomingData loop
 

// Helper Functions

void updateGuiDisplay(String text) {
  int width = tft.width();    // ✅ capture for lambda
  int textSizeHeader = 2;
  int textSizeBody = 3;

  tft.setTextWrap(false);
  tft.fillRect(0, 120, width, 64, GC9A01A_BLACK);

  String header = "GUI Command Received:";
  int headerX = (width - (header.length() * 6 * textSizeHeader)) / 2;
  tft.setTextSize(textSizeHeader);
  tft.setCursor(headerX, 120);
  tft.setTextColor(GC9A01A_CYAN);
  tft.print(header);

  int commandX = (width - (text.length() * 6 * textSizeBody)) / 2;
  tft.setTextSize(textSizeBody);
  tft.setCursor(commandX, 150);
  tft.setTextColor(GC9A01A_YELLOW);
  tft.println(text);

  timer.setTimeout(1000, [width]() {
    tft.fillRect(0, 120, width, 64, GC9A01A_BLACK);
  });
}


void updateRfidDisplay(String text) {
  int width = tft.width();   // ✅ capture for lambda
  int textSizeHeader = 2;
  int textSizeBody = 3;

  tft.setTextWrap(false);
  tft.fillRect(0, 320, width, 64, GC9A01A_BLACK);

  String header = "RFID Command Received:";
  int headerX = (width - (header.length() * 6 * textSizeHeader)) / 2;
  tft.setTextSize(textSizeHeader);
  tft.setCursor(headerX, 320);
  tft.setTextColor(GC9A01A_CYAN);
  tft.print(header);

  int commandX = (width - (text.length() * 6 * textSizeBody)) / 2;
  tft.setTextSize(textSizeBody);
  tft.setCursor(commandX, 350);
  tft.setTextColor(GC9A01A_RED);
  tft.println(text);

  timer.setTimeout(1000, [width]() {
    tft.fillRect(0, 320, width, 64, GC9A01A_BLACK);
  });
}


//Sends data to DCC++ board over Serial3
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
