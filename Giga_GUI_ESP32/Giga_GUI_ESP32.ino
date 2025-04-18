#define BLYNK_TEMPLATE_ID "TMPL8F0MjIJU"
#define BLYNK_TEMPLATE_NAME "K3 Stainz"
#define BLYNK_AUTH_TOKEN "dBj4EvfjiPQuzEZGIH7ecmV5B0xbOK4W"

//#define BLYNK_TEMPLATE_ID "TMPL27oFosZB8"
//#define BLYNK_TEMPLATE_NAME "Deck Train"
//#define BLYNK_AUTH_TOKEN "HGJM2L_Z9bSIUmTeMgHWPZP1KpRbEHt3"


// Diplay parameters
#define D_HEIGHT 64  //Display height
#define D_WIDTH 128  //Display width

// Determines how long text displayed prior to being cleared from the screen
#define D_DELAY 1000
#define DT 500

// Station 1-4 LED display duration
// Determines how long Station LED on the Blynk GUI stay on
#define STATION_LED_DURATION 3000

#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_SH1107_Ext.h>

int train_speed = 0;
int train_speed_hold = 0;


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


//Display instance

//Note this is our _Ext or extention of the Adafruit library to include the following functions:
// 1. void display.centeredDisplay("line1", ...); with up to four lines as arguments with an optional
// last numeric delay parameter which clears the display after delay
// 2. void display.clear_Disp(); can be called with no argument (executes immediately)
// or with a number of miliseconds to clear the display after
// 3. void display.timerRun(); to make the timer in the new display library run

Adafruit_SH1107_Ext display = Adafruit_SH1107_Ext(D_HEIGHT, D_WIDTH, &Wire, 1000000, 1000000);


// Blynk variables

BlynkTimer timer;  //Using blynk timer but could also use SimpleTimer
char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "BR Guest";
char pass[] = "Pamma355!";

//  char ssid[] = "Train";
//  char pass[] = "Photon8589";


// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED() {
//  sendDataCentral(39);  //Train Ready Command
//  display.centeredDisplay("Wifi", "Connected", D_DELAY);
}


void setup() {

  //Serial1 connect the GUI board to the central board
  Serial.begin(115200);
  Serial1.begin(115200);

  pinMode (12, OUTPUT);
  pinMode (27, INPUT);

  //Display Settings
  display.begin(0x3C, true);  // Address 0x3C default
  display.setRotation(3);
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.display();

  Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  // Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  // Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

  // Setup a function to be called every second
  // timer.setInterval(1000L, myTimerEvent);

  Blynk.virtualWrite(V1, LOW);     // Lights OFF
  Blynk.virtualWrite(V2, LOW);     // Smoke OFF
  Blynk.virtualWrite(V3, LOW);     // RFID sensor OFF
  Blynk.virtualWrite(V4, LOW);     // Park OFF
  Blynk.virtualWrite(V5, HIGH);    // Sound ON
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
  display.timerRun();
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


int receiveCentralData() {
  if (Serial1.available()) {
    return Serial1.read();
  } else return -1;
}


// Code to receive station information from the central board sent to it by the RFID tag reader board
// Needs to be called in loop
  
void parseIncomingCommands() {
  int command = receiveCentralData();

// Sends Station # to GUI
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
}


// GUI activated Train function controls
// Sent from ESP32 to Due
// ** Switch ON / OFF **

BLYNK_WRITE(V1) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(101);
    display.centeredDisplay("Lights ON", D_DELAY);
  } else if (pinValue == 0) {
    sendDataCentral(100);
    display.centeredDisplay("Lights OFF", D_DELAY);
  }
}

BLYNK_WRITE(V2) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(151);
    display.centeredDisplay("Smoke ON", D_DELAY);
  } else if (pinValue == 0) {
    sendDataCentral(150);
    display.centeredDisplay("Smoke OFF", D_DELAY);
  }
}

BLYNK_WRITE(V3) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(161);
    display.centeredDisplay("Sound ON", D_DELAY);
  } else if (pinValue == 0) {
    sendDataCentral(160);
    display.centeredDisplay("Sound OFF", D_DELAY);
  }
}

BLYNK_WRITE(V4) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(251);
    display.centeredDisplay("RFID ON", D_DELAY);
  } else if (pinValue == 0) {
    sendDataCentral(250);
    display.centeredDisplay(RFID"RFID OFF", D_DELAY);
  }
}

BLYNK_WRITE(V5) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(253);
    display.centeredDisplay("Park ON", D_DELAY);
  } else if (pinValue == 0) {
    sendDataCentral(252);
    display.centeredDisplay("Park OFF", D_DELAY);
  }
}

BLYNK_WRITE(V6) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(255);
    display.centeredDisplay("Forward", D_DELAY);
  } else if (pinValue == 0) {
    sendDataCentral(254);
    display.centeredDisplay("Reverse", D_DELAY);
  }
}


// GUI activated Train function controls (Start / Faster speed, Slower speed, Stop)
// ** Press ON / Release OFF **

BLYNK_WRITE(V7) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(7);
    display.centeredDisplay("Start/", "Faster", D_DELAY);
  }
}

BLYNK_WRITE(V8) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(8);
    display.centeredDisplay("Slower", D_DELAY);
  }
}

BLYNK_WRITE(V9) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(9);
    display.centeredDisplay("Stop", D_DELAY);
  }
}


// GUI activated instantaneous sounds (push button)
// American Mogul (V41 - V64)

BLYNK_WRITE(V41) {                // AM F1
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(41);
    display.centeredDisplay("Bell", D_DELAY);
  }
  else if (pinValue == 0) {}
}

BLYNK_WRITE(V42) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(42);
    display.centeredDisplay("Long", "whistles", D_DELAY);
  }
}

BLYNK_WRITE(V43) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(43);
    display.centeredDisplay("Whistle", D_DELAY);
  }
}

BLYNK_WRITE(V44) {                // AM F4
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(44);
    display.centeredDisplay("Short", "Whistle", D_DELAY);
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

BLYNK_WRITE(V49) {                // AM F9
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(49);
    display.centeredDisplay("Wheels", "squealing", D_DELAY);
  }
}

BLYNK_WRITE(V50) {                // AM F10
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(50);
    display.centeredDisplay("Coal", "shoveling", D_DELAY);
  }
}

BLYNK_WRITE(V51) {                // AM F11
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(51);
    display.centeredDisplay("Blower", "sound", D_DELAY);
  }
}

BLYNK_WRITE(V52) {                // AM F12
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(52);
    display.centeredDisplay("Coupler", "uncoupler", D_DELAY);
  }
}

BLYNK_WRITE(V53) {                // AM F13
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(53);
    display.centeredDisplay("Coal", "dropping", D_DELAY);
  }
}

BLYNK_WRITE(V54) {                // AM F14
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(54);
    display.centeredDisplay("Steam", "venting", D_DELAY);
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

BLYNK_WRITE(V57) {                // AM F17
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(57);
    display.centeredDisplay("Conductor", "calling", D_DELAY);
  }
}

BLYNK_WRITE(V58) {                // AM F18
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(58);
    display.centeredDisplay("Generator", "sound", D_DELAY);
  }
}

BLYNK_WRITE(V59) {                // AM F19
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(59);
    display.centeredDisplay("Air", "pump", D_DELAY);
  }
}

BLYNK_WRITE(V60) {                // AM F20
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(60);
    display.centeredDisplay("Coal", "unloading", D_DELAY);
  }
}

BLYNK_WRITE(V61) {                // AM F21
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(61);
    display.centeredDisplay("Water", "filling", D_DELAY);
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

BLYNK_WRITE(V65) {                // K3 F0
  int pinValue = param.asInt();
  if (pinValue == 1) {
    display.centeredDisplay("Lights ON",D_DELAY);
    sendDataCentral(101); 
  } else if (pinValue == 0) {
    display.centeredDisplay("Lights OFF", D_DELAY);
    sendDataCentral(100);
  }
}

BLYNK_WRITE(V66) {                // K3 Open F1
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V67) {                // K3 F2
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(67);
    display.centeredDisplay("Conductor", "calling", D_DELAY);
  }
}

BLYNK_WRITE(V68) {                // K3 F3
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(68);
    display.centeredDisplay("Conductors", "whistle", D_DELAY);
  }
}

BLYNK_WRITE(V69) {                // K3 Open F4
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}

BLYNK_WRITE(V70) {                // K3 F5
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(70);
    display.centeredDisplay("Coal", "shoveling", D_DELAY);
  }
}

BLYNK_WRITE(V71) {                // K3 F6
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(71);
    display.centeredDisplay("Injector", "sound", D_DELAY);
  }
}

BLYNK_WRITE(V72) {                // K3 F7
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(72);
    display.centeredDisplay("Bell", D_DELAY);
  }
}

BLYNK_WRITE(V73) {                // K3 F8
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(161);
    display.centeredDisplay("Sound ON", D_DELAY);
  } else if (pinValue == 0) {
    sendDataCentral(160);
    display.centeredDisplay("Sound OFF", D_DELAY);
  }
}

BLYNK_WRITE(V74) {                // K3 F9
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(74);
    display.centeredDisplay("Whistle", D_DELAY);
  }
}

BLYNK_WRITE(V75) {                // K3 F10
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(75);
    display.centeredDisplay("Coupler", "uncoupler", D_DELAY);
  }
}

BLYNK_WRITE(V76) {                // K3 F11
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(76);
    display.centeredDisplay("Air", "pump", D_DELAY);
  }
}

BLYNK_WRITE(V77) {                // K3 F12
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(77);
    display.centeredDisplay("Boiler", "sound", D_DELAY);
  }
}

BLYNK_WRITE(V78) {                // K3 F13
  int pinValue = param.asInt();
  if (pinValue == 1) {
    sendDataCentral(78);
    display.centeredDisplay("Steam", "venting", D_DELAY);
  }
}

BLYNK_WRITE(V79) {                // K3 Open F14
  int pinValue = param.asInt();
  if (pinValue == 1) {
  } else if (pinValue == 0) {
  }
}




