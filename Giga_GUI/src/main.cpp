#define BLYNK_TEMPLATE_ID "TMPL8F0MjIJU"
#define BLYNK_TEMPLATE_NAME "K3 Stainz"
#define BLYNK_AUTH_TOKEN "dBj4EvfjiPQuzEZGIH7ecmV5B0xbOK4W"

//#define BLYNK_TEMPLATE_ID "TMPL27oFosZB8"
//#define BLYNK_TEMPLATE_NAME "Deck Train"
//#define BLYNK_AUTH_TOKEN "HGJM2L_Z9bSIUmTeMgHWPZP1KpRbEHt3"

#define DT 500

#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

int train_speed = 0;
int train_speed_hold = 0;
int new_Control_data = 0;
int Control_data = 0;
int command = 0;


// Function Declarations
// virtualWrite replaces Blynk.virtualWrite

// Takes arguments vPin and state and executes immediately
// or vPin, state, and Delay to execute the virtualWrite after a delay

void virtualWrite(int vPin, int state);
void virtualWrite(int vPin, int state, ulong delay);


//Send Data from the GUI blynk board to the central board
//parameter are either the int data to execute immediately or
//data, delay to execute after a delay

void sendDataCentral(int data, unsigned long int delay = 0);

// Read Data coming back from the central board
int receiveCentralData();
 
// Parse what is coming back from the central board
void parseIncomingCommands();


// Blynk variables

BlynkTimer timer;  //Using blynk timer but could also use SimpleTimer
char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "BR Guest";
char pass[] = "Pamma355!";

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED() {
  sendDataCentral(39);  //Train Ready Command

}


void setup() {

  //Serial1 connect the GUI board to the central board
  Serial.begin(115200);
  Serial1.begin(115200);

  pinMode (15, OUTPUT);
  pinMode (32, INPUT);

  Blynk.begin(auth, ssid, pass);
 
  // Setup a function to be called every second
  // timer.setInterval(1000L, myTimerEvent);

  Blynk.virtualWrite(V1, LOW);     // Lights OFF
  Blynk.virtualWrite(V2, LOW);     // Smoke OFF
  Blynk.virtualWrite(V3, HIGH);    // Sound ON
  Blynk.virtualWrite(V4, LOW);     // RFID OFF
  Blynk.virtualWrite(V5, LOW);     // Park OFF
  Blynk.virtualWrite(V6, HIGH);    // Direction FWD 
  Blynk.virtualWrite(V10, LOW);    // Speed = 0 
  Blynk.virtualWrite(V25, LOW);    // Station 1 LED OFF
  Blynk.virtualWrite(V26, LOW);    // Station 2 LED OFF
  Blynk.virtualWrite(V27, LOW);    // Station 3 LED OFF
  Blynk.virtualWrite(V28, LOW);    // Station 4 LED OFF
  Blynk.virtualWrite(V29, LOW);    // Station 5 LED OFF
  Blynk.virtualWrite(V30, LOW);    // Station 6 LED OFF  
}


void loop() {
  Blynk.run();
  parseIncomingCommands();
  timer.run();
}


  // Helper Functions

void virtualWrite(int vPin, int state) {
  Blynk.virtualWrite(vPin, state);
}

void virtualWrite(int vPin, int state, ulong delay) {
  timer.setTimeout(delay, [=]() {
    Blynk.virtualWrite(vPin, state);
  });
}

void sendDataCentral(int data, unsigned long int delay) {
  if (!delay) {  //delay is 0, execute immediately
    if (Serial1.availableForWrite()) {
      Serial1.write(data);
      Serial.println(data);
    }
  } else {  //delay is longer than 0, execute after delay through timer library
    timer.setTimeout(delay,
                     [=]() {
                       Serial1.write(data);
                     });
  }
}


// Code to receive station information from the central board sent to it by the RFID tag reader board
// Needs to be called in loop
  
void parseIncomingCommands() {

  if (Serial1.available()) {
    uint8_t controlChar = Serial1.read();  
    Control_data = (int)controlChar;

    new_Control_data = true;
  }

// Sends Station # to GUI
  if (new_Control_data) {
    
    Serial.print("Control Data Received: ");
    Serial.println(Control_data);

  command = Control_data;
  if (command > -1) {
    if (command == 25) {
      virtualWrite(V25, 1);        // turns on Station 1 LED and others off
      virtualWrite(V26, 0);
      virtualWrite(V27, 0);
      virtualWrite(V28, 0);
      virtualWrite(V29, 0);
      virtualWrite(V30, 0);
    }
    if (command == 26) {
      virtualWrite(V25, 0);
      virtualWrite(V26, 1);         // turns on Station 2 LED and others off
      virtualWrite(V27, 0);
      virtualWrite(V28, 0);
      virtualWrite(V29, 0);
      virtualWrite(V30, 0);
    }
    if (command == 27) {
      virtualWrite(V25, 0);
      virtualWrite(V26, 0);
      virtualWrite(V27, 1);         // turns on Station 3 LED and others off
      virtualWrite(V28, 0);
      virtualWrite(V29, 0);
      virtualWrite(V30, 0);
    }
    if (command == 28) {
      virtualWrite(V25, 0);
      virtualWrite(V26, 0);
      virtualWrite(V27, 0);
      virtualWrite(V28, 1);         // turns on Station 4 LED and others off
      virtualWrite(V29, 0);
      virtualWrite(V30, 0);
    }
    if (command == 29) {
      virtualWrite(V25, 0);
      virtualWrite(V26, 0);
      virtualWrite(V27, 0);
      virtualWrite(V28, 0);
      virtualWrite(V29, 1);         // turns on Station 5 LED and others off
      virtualWrite(V30, 0);
    }
    if (command == 30) {
      virtualWrite(V25, 0);
      virtualWrite(V26, 0);
      virtualWrite(V27, 0);
      virtualWrite(V28, 0);
      virtualWrite(V29, 0);
      virtualWrite(V30, 1);         // turns on Station 6 LED and others off
    }

//  Sends train_speed data to GUI
    if (command >= 100) {              
      train_speed = (command - 100);  
      virtualWrite(V10, train_speed);
    } 
  }  
  new_Control_data = false;  
}
}

// GUI activated Train function controls
// Sent from ESP32 Serial1 to Giga Serial4

// ** Switch ON / OFF **
// 3 digit number (1 = 'Switch ON/OFF function'; Virtual pin number; ON = 1, OFF = 0)

BLYNK_WRITE(V1) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(111);     // Lights ON
  } else if (pinValue == 0) {
    sendDataCentral(110);     // Lights OFF
  }
}

BLYNK_WRITE(V2) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(121);   // Smoke ON
  } else if (pinValue == 0) {
    sendDataCentral(120);   // Smoke OFF
  }
}

BLYNK_WRITE(V3) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(131);   // Sound ON
  } else if (pinValue == 0) {
    sendDataCentral(130);   // Sound OFF
  }
}

BLYNK_WRITE(V4) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(141);   // Read RFID ON
  } else if (pinValue == 0) {
    sendDataCentral(140);   // Read RFID OFF
  }
}

BLYNK_WRITE(V5) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(151);   // Park function ON
  } else if (pinValue == 0) {
    sendDataCentral(150);   // Park function OFF
  }
}

BLYNK_WRITE(V6) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(161);   // Direction = FORWARD
  } else if (pinValue == 0) {
    sendDataCentral(160);   // Direction = REVERSE
  }
}


// GUI activated Train function controls (Start / Faster speed, Slower speed, Stop)
// ** Press ON / Release OFF **

BLYNK_WRITE(V7) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(7);   // Start / Faster
  }
}

BLYNK_WRITE(V8) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(8);   // Slower
  }
}

BLYNK_WRITE(V9) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(9);   // Stop
  }
}


// GUI activated instantaneous sounds
// ** Press ON / Release OFF **

// American Mogul (V40 - V64)

// V40 = F0 function for lights - already controlled by V1

BLYNK_WRITE(V41) {                // AM F1 - Bell
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(41);
  }
  else if (pinValue == 0) {}
}

BLYNK_WRITE(V42) {                // AM F2 - Long whistles
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(42);
  }
}

BLYNK_WRITE(V43) {                // AM F3 - Whistle
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(43);
  }
}

BLYNK_WRITE(V44) {                // AM F4 - Short whistle
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(44);
  }
  else if (pinValue == 0) {}
}

BLYNK_WRITE(V45) {                // AM Open F5
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } 
  else if (pinValue == 0) {}
}

BLYNK_WRITE(V46) {                // AM Open F6
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } 
  else if (pinValue == 0) {}
}

BLYNK_WRITE(V47) {                // AM Open F7
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V48) {                // AM F9 is Mute ON/OFF activated by V5 function (Sound ON/OFF)
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V49) {                // AM F9 - Wheels squealing
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(49);
  }
}

BLYNK_WRITE(V50) {                // AM F10 - Coal shoveling
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(50);
  }
}

BLYNK_WRITE(V51) {                // AM F11 - Blower sound
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(51);
  }
}

BLYNK_WRITE(V52) {                // AM F12 - Coupler / uncoupler
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(52);
  }
}

BLYNK_WRITE(V53) {                // AM F13 - Coal dropping
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(53);
  }
}

BLYNK_WRITE(V54) {                // AM F14 - Steam venting
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(54);
  }
}

BLYNK_WRITE(V55) {                // AM Open F15
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V56) {                // AM Open F16
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V57) {                // AM F17 - Conductor calling 'All Aboard'
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(57);

  }
}

BLYNK_WRITE(V58) {                // AM F18 - Generator sound
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(58);
  }
}

BLYNK_WRITE(V59) {                // AM F19 - Air pump
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(59);
  }
}

BLYNK_WRITE(V60) {                // AM F20 - Coal unloading
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(60);
  }
}

BLYNK_WRITE(V61) {                // AM F21 - Water filling
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(61);
  }
}

BLYNK_WRITE(V62) {                // AM Open F22
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V63) {                // AM Open F23
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V64) {                // AM Open F24
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}


// K3 Stainz sounds/function (V65 - V79)

// V65 = F0 function for lights - already controlled by V1

BLYNK_WRITE(V66) {                // K3 Open F1
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V67) {                // K3 F2 - Conductor calling 'All Aboard'
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(67);
  }
}

BLYNK_WRITE(V68) {                // K3 F3 - Conductor's whistle
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(68);
  }
}

BLYNK_WRITE(V69) {                // K3 Open F4
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V70) {                // K3 F5 - Coal shoveling
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(70);
  }
}

BLYNK_WRITE(V71) {                // K3 F6 - Injector sound
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(71);
  }
}

BLYNK_WRITE(V72) {                // K3 F7 - Bell
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(72);
  }
}

BLYNK_WRITE(V73) {                // K3 F8 - Sound ON / OFF Toggle
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(161);
  } else if (pinValue == 0) {
    sendDataCentral(160);
  }
}

BLYNK_WRITE(V74) {                // K3 F9 - Whistle
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(74);
  }
}

BLYNK_WRITE(V75) {                // K3 F10 - Coupler / uncoupler
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(75);
  }
}

BLYNK_WRITE(V76) {                // K3 F11 - Air pump
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(76);
  }
}

BLYNK_WRITE(V77) {                // K3 F12 - Boiler sound
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(77);
  }
}

BLYNK_WRITE(V78) {                // K3 F13 - Steam venting
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(78);
  }
}

BLYNK_WRITE(V79) {                // K3 Open F14
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}
