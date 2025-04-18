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


#include <SimpleTimer.h>
SimpleTimer timer;

#define Sound_Delay 1000                      // Delay after which to cancel sound
#define SD 500 

//Serial connections
//Serial2 is RFID M4 communication
//Serial3 is DCC-EX communication
//Serial4 is GUI ESP32 communication




//Helper Functinos

void parseIncomingData();
//Sends data to DCC-EX over Serial3
void sendDataDccEX(char data[], unsigned long int delay = 0);
//Sends data to Gui Board over Serial2
void sendDataGui(int data, unsigned long int delay = 0);


void setup() {

    pinMode(A4, OUTPUT);                      // Connected to RFID M4 pin 9
    pinMode(A5, INPUT);                       // Connected to RFID M4 pin 10
    pinMode(A6, OUTPUT);                      // Connected to ESP-32 pin 10
    pinMode(A7, INPUT);                       // Connected to ESP-32 pin 9
  
    SerialUSB.begin(115200);
    Serial2.begin(115200);   
    Serial3.begin(115200);
    Serial4.begin(115200);
   
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
    SerialUSB.println(GUI_data);
  }

  if (new_GUI_data) {

    if (GUI_data == 101) {
      sendDataDccEX("<F 3 0 1>");                       // Lights ON / OFF
    } else if (GUI_data == 100) {
        sendDataDccEX("<F 3 0 0>");
      }
      
    if (GUI_data == 251) {RFID_sensor = 1;}             // RFID sensor ON / OFF
      else if (GUI_data == 250) {RFID_sensor = 0;}

    if (GUI_data == 253) {park_switch = 1;}             // park_switch ON / OFF
      else if (GUI_data == 252) {park_switch = 0;}

    if (GUI_data == 255) {train_direction = 1;}
      else if (GUI_data == 254) {train_direction = 0;}    // Direction FWD / REV

    if (GUI_data == 161) {sound_switch = 1;}            // Sound ON / OFF       
      else if (GUI_data == 160) {sound_switch = 0;}

    if (GUI_data == 151){smoke_switch = 1;}             // Smoke ON / OFF
      else if (GUI_data == 150) {smoke_switch = 0;}


  //  GUI initiated speed control (Start / Faster speed, Slower speed, Stop) - PUSH BUTTON control

    // Train Start / Faster /Slower
    if (GUI_data == 7) {train_speed = train_speed + 1;}     
    if (GUI_data == 8) {train_speed = train_speed - 1;} 

    // Only send speed command if speed or direction changed
  //if (train_speed != last_sent_speed || train_direction != last_sent_direction) {
  // char speedCmd[30];
    //int speedValue = (train_speed == 0) ? -1 : train_speed * 10;

    //sprintf(speedCmd, "<t 1 03 %d %d>", speedValue, train_direction);
    //sendDataDccEX(speedCmd);

    //last_sent_speed = train_speed;
  //last_sent_direction = train_direction;
  //
    if (train_speed >= 10) {train_speed = 10;}
    if (train_speed <= 0) {train_speed = 0;}   

    // Train stop
    if (GUI_data == 9) {                        // Train stop
      sendDataDccEX("<t 1 03 -1 1>");
      train_speed = 0;
      }  


    if (GUI_data == 41) {
      sendDataDccEX("<F 3 7 1>");                         // AM F1 Bell
      sendDataDccEX("<F 3 7 0>", Sound_Delay);            // Cancel Bell after Sound_Delay
    }

    if (GUI_data == 42) {
      sendDataDccEX("<F 3 2 1>");                         // AM F2 Long whistles
      sendDataDccEX("<F 3 2 0>", Sound_Delay);            // Cancel long whistles after Sound_Delay
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
      sendDataDccEX("<F 3 2 1>");                         // K3 F2 Conductor calling "All Aboard"
      sendDataDccEX("<F 3 2 0>", Sound_Delay);            // Cancel conductor calling after Sound_Delay
    }

    if (GUI_data == 68) {
      sendDataDccEX("<F 3 3 1>");                         // K3 F3 Conductor's whistle
      sendDataDccEX("<F 3 3 0>", Sound_Delay);            // Cancel conductor's whistle after Sound_Delay
    }

    if (GUI_data == 69) {}                                // K3 Open F4

    if (GUI_data == 70) {
      sendDataDccEX("<F 3 5 1>");                         // K3 F5 Coal shoveling
      sendDataDccEX("<F 3 5 0>", Sound_Delay);            // Cancel coal shoveling after Sound_Delay
    }

    if (GUI_data == 71) {
      sendDataDccEX("<F 3 6 1>");                         // K3 F6 Injector sound
      sendDataDccEX("<F 3 6 0>", Sound_Delay);            // Cancel injector after Sound_Delay
    }

    if (GUI_data == 72) {
      sendDataDccEX("<F 3 7 1>");                         // K3 F7 Bell
      sendDataDccEX("<F 3 7 0>", Sound_Delay);            // Cancel Bell after Sound_Delay
    }

    if (GUI_data == 73) {}                                // K3 F8 Sound ON/OFF toggle via V3

    if (GUI_data == 74) {
      sendDataDccEX("<F 3 9 1>");                         // K3 F9 Whistle
      sendDataDccEX("<F 3 9 0>", Sound_Delay);            // Cancel Whistle after Sound_Delay
    } 

    if (GUI_data == 75) {
      sendDataDccEX("<F 3 10 1>");                         // K3 F10 Coupler/uncoupler
      sendDataDccEX("<F 3 10 0>", Sound_Delay);            // Cancel coupler/uncoupler after Sound_Delay
    }

    if (GUI_data == 76) {
      sendDataDccEX("<F 3 11 1>");                         // K3 F11 Air pump
      sendDataDccEX("<F 3 11 0>", Sound_Delay);            // Cancel air pump after Sound_Delay
    }

    if (GUI_data == 77) {
      sendDataDccEX("<F 3 12 1>");                         // K3 F12 Boiler sound
      sendDataDccEX("<F 3 12 0>", Sound_Delay);            // Cancel boiler sound after Sound_Delay
    }

    if (GUI_data == 78) {
      sendDataDccEX("<F 3 13 1>");                         // K3 F13 Steam venting
      sendDataDccEX("<F 3 13 0>", Sound_Delay);            // Cancel steam venting after Sound_Delay
    }

    if (GUI_data == 79) {}                                 // K3 Open F14

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

    if (RFID_data == 42) {
          sendDataDccEX("<f 3 130>");
          sendDataDccEX("<f 3 128>", Sound_Delay);
        }

        if (RFID_data == 43) {
          sendDataDccEX("<F 3 9 1>");
          sendDataDccEX("<F 3 9 0>", Sound_Delay);
        }

        if (RFID_data == 44) {
          sendDataDccEX("<f 3 136>");
          sendDataDccEX("<f 3 128>", Sound_Delay);
        }

        if (RFID_data == 41) {
          sendDataDccEX("<F 3 7 1>");
          sendDataDccEX("<F 3 7 0>", Sound_Delay);
        }

        // Speed settings
        if (RFID_data == 20) { train_speed = 2; sendDataDccEX("<t 1 03 20 1>"); }
        else if (RFID_data == 21) { train_speed = 4; sendDataDccEX("<t 1 03 40 1>"); }
        else if (RFID_data == 22) { train_speed = 6; sendDataDccEX("<t 1 03 60 1>"); }
        else if (RFID_data == 24) { train_speed = 10; sendDataDccEX("<t 1 03 100 1>"); }

        // Park logic
        if (park_switch == 1 && train_direction == 1) {
          if (RFID_data == 31) { train_speed = 5; sendDataDccEX("<t 1 03 50 1>"); }
          if (RFID_data == 32) { train_speed = 3; sendDataDccEX("<t 1 03 30 1>"); }
          if (RFID_data == 25) { train_speed = 0; sendDataDccEX("<t 1 03 -1 1>"); }
        }

        if (train_speed != train_speed_hold) {
          sendDataGui(((train_speed * 10) + 100), 0);
          train_speed_hold = train_speed;
        }

    new_RFID_data = false;

  }


}  // end of parseIncomingData loop
 




// Helper Functions

//Sends data to DCC++ board over Serial3
void sendDataDccEX(char data[], unsigned long int delay) {
  if (!delay) {  //delay is 0, execute immediately
//    if (Serial3.availableForWrite()) {
      Serial3.print(data);
      SerialUSB.println(data);
 //   }
  } else {  //delay is longer than 0, execute after delay through timer library
    timer.setTimeout(delay,
                     [=]() {
                       Serial3.print(data);
                     });
  }
}

void sendDataGui(int data, unsigned long int delay) {
  if (!delay) {  //delay is 0, execute immediately
    if (Serial4.availableForWrite()) {
      Serial4.write(data);
    }
  } else {  //delay is longer than 0, execute after delay through timer library
    timer.setTimeout(delay,
                     [=]() {
                       Serial4.write(data);
                     });
  }
}

