/**********************************************************************

  ErrorMonitor.cpp
  COPYRIGHT (c) Costin Grigoras

  Part of DCC++ BASE STATION for the Arduino / extended for Pololu shield

**********************************************************************/

#include "DCCpp_Uno.h"
#include "ErrorMonitor.h"
#include "Comm.h"

///////////////////////////////////////////////////////////////////////////////

long int lastPowerOffTime = 0;

//#define ERROR_MONITOR_DEBUG true
//#define ERROR_MONITOR_DEBUG_FINE true

#define MILLIS_TO_RECOVER_MAX 10
#define ACT_UP_TO 100

void errorCheck() {
  if (digitalRead(SIGNAL_ENABLE_PIN_PROG) == LOW)
    lastPowerOffTime = millis();

  if (digitalRead(ERROR_MONITOR_PIN) == 0 && digitalRead(SIGNAL_ENABLE_PIN_PROG) == HIGH) {
    digitalWrite(SIGNAL_ENABLE_PIN_PROG, LOW);                                                    // disable both Motor Shield Channels
    digitalWrite(SIGNAL_ENABLE_PIN_MAIN, LOW);                                                    // regardless of which caused current overload

    if (millis() - lastPowerOffTime < ACT_UP_TO) {
      // at startup time large capacitors charging simultaneously can trigger the error state
      // re-enable the power for a few milliseconds to allow them to charge

      const long int recoveryStarted = millis();

      long lastRecoveryAttempt = 0;

      while (millis() - recoveryStarted < MILLIS_TO_RECOVER_MAX) {
        lastRecoveryAttempt = millis();

        digitalWrite(SIGNAL_ENABLE_PIN_MAIN, LOW);
        digitalWrite(SIGNAL_ENABLE_PIN_PROG, LOW);

        digitalWrite(SIGNAL_ENABLE_PIN_MAIN, HIGH);
        digitalWrite(SIGNAL_ENABLE_PIN_PROG, HIGH);

        while (digitalRead(ERROR_MONITOR_PIN) == 1 && (millis() - recoveryStarted < MILLIS_TO_RECOVER_MAX)) ;

        if (digitalRead(ERROR_MONITOR_PIN) == 0) {
#ifdef ERROR_MONITOR_DEBUG_FINE
          INTERFACE.print("\nRetry @ ");
          INTERFACE.println(millis() - recoveryStarted);
#endif
        }
        else {
#ifdef ERROR_MONITOR_DEBUG
          INTERFACE.print("\nWorked@");
          INTERFACE.println(millis() - lastRecoveryAttempt);
#endif

          break;
        }
      }

#ifdef ERROR_MONITOR_DEBUG
      if (digitalRead(ERROR_MONITOR_PIN) == 0)
        INTERFACE.println("\nRecovery didn't work");
#endif

      // let it process messages and maybe try again later, or show the error if not
      return;
    }
#ifdef ERROR_MONITOR_DEBUG
    else
      INTERFACE.println("\nNo recovery");
#endif

    if (digitalRead(ERROR_MONITOR_PIN) == 0) {
#ifdef ERROR_MONITOR_DEBUG
      INTERFACE.print("\nShort circuit\n");
#endif

      INTERFACE.print("<p2>");                                                                            // print corresponding error message
    }
  }
}
