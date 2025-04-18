/**********************************************************************

CurrentMonitor.h
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

#ifndef ErrorMonitor_h
#define ErrorMonitor_h

#include "Arduino.h"

// Pololu MC33926 SF pin connected to pin 4 of Arduino
#define ERROR_MONITOR_PIN 4

void errorCheck();

#endif
