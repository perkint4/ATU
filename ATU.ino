/* Auto Top Up (ATU) controller
  2018 Jan
  Tim Perkins
  Code written to control a pump for running RO top up on a reef tank (can be use for any automated top up)
    This version based on two float switches. Also has a safety cut out if pump runs for too long

  Hardware:
    Arduino (any version)
    RGB LED (can change fro any 3 LEDs)
    2 Float switches
    MOSFET or Relay (if using relay ensure no more than 40mA drain)
    A pump suitable to the MOSFET/relay in use
  Libraries
    None!
  IDE
    Arduino IDE v1.8.4

  Support thread:http://www.ultimatereef.net/

*/

const int const_Pump    = 7;               // Control for the pump MOSFET
const int const_LEDGnd  = 8;               // GND for the RGB LED
const int const_Red     = 9;               // Red for the RGB LED
const int const_Green   = 10;              // Green for the RGB LED
const int const_Blue    = 11;              // Blue for the RGB LED
const int const_Float1a = A2;              // One of the PINs for Float 1
const int const_Float1b = A3;              // The other PINs for Float 1
const int const_Float2a = A4;              // One of the PINs for Float 2
const int const_Float2b = A5;              // The other PINs for Float 2

const int const_PumpMinRun   = 3;          // The minimum number of second the pump can run for (hysteresis)
const int const_PumpMaxRun   = 10;         // The max number of second the pump can run for before alarming
// This alert stops the ATU from running again till reset
const int const_FloatWait    = 10;         // The amount of seconds to wait before pumping after both floats trigger
// This is to stop the pump triggering as a result of waves (debounce/hysteresis)
const int const_MaxWithoutTU = 24;         // Alert if not topped up for this number of hours
// This alert does not stop the ATU from running and will reset automatically
// if the pump triggers

int           redPWM          = 0;
int           greenPWM        = 0;
int           bluePWM         = 0;
int           errorState      = 0;
unsigned long lastTU          = 0;
unsigned long startTU         = 0;
unsigned long floatsTriggered = 0;
int           pumpMinRun      = const_PumpMinRun * 1000;      // convert the seconds into milliseconds
int           pumpMaxRun      = const_PumpMaxRun * 1000;      // convert the seconds into milliseconds
int           floatWait       = const_FloatWait * 1000;       // convert the seconds into milliseconds
unsigned long maxWithoutTU    = const_MaxWithoutTU * 3600000; // convert the hours into milliseconds
char          float1Go        = 'N';                          // flag to say if the float has triggered
char          float2Go        = 'N';                          // flag to say if the float has triggered
char          pumpOn          = 'N';                          // flag to say if the pump is on or not
char          floatStatus     = '0';                          // What are the floats up to:
                                                              // 0 = Both off. COuld be OK, could be stuck float
                                                              // 1 = One float on, one off (normal)
                                                              // 2 = floats calling for water
char          cuurentStatus   = '0';                          // What are we currently up to:
                                                              // 0 = Just started up
                                                              // 1 = One float on, one off (normal)
                                                              // 2 = floats calling for water, but waiting for floatWait
                                                              // 3 = pumping with floats calling for more
                                                              // 4 = pumping with floats saying no mor needed (pumpMinRun)
                                                              // 5 = pumping with floats saying no mor needed (pumpMinRun)

void setup() {
  Serial.begin(38400);
  //  Set the mode for all the IO pins
  pinMode(const_Pump,        OUTPUT);
  digitalWrite(const_Pump,   LOW);
  pinMode(const_LEDGnd,      OUTPUT);
  digitalWrite(const_LEDGnd, LOW);
  // Flash the three LEDs on start up. Helps check the wiring is right!
  //         setLED(PIN to change, current PWM, new PWM, change delay
  redPWM   = setLED(const_Red, redPWM, 255, 1);
  redPWM   = setLED(const_Red, redPWM, 0, 1);
  greenPWM = setLED(const_Green, greenPWM, 255, 1);
  greenPWM = setLED(const_Green, greenPWM, 0, 1);
  bluePWM  = setLED(const_Blue, bluePWM, 255, 1);
  bluePWM  = setLED(const_Blue, bluePWM, 0, 1);
  greenPWM = setLED(const_Green, greenPWM, 255, 1);
  pinMode(const_Float1a,      OUTPUT);
  digitalWrite(const_Float1a, LOW);
  pinMode(const_Float1b,      INPUT_PULLUP);
  pinMode(const_Float2a,      OUTPUT);
  digitalWrite(const_Float2a, LOW);
  pinMode(const_Float2b,      INPUT_PULLUP);
}

void loop() {
  if (errorState > 50) {
    // errorstate over 50 means something has gone pear shaped - do nothing except flash for help!
    greenPWM = setLED(const_Green, greenPWM, 0, 1);
    bluePWM  = setLED(const_Blue, bluePWM, 0, 1);
    redPWM   = setLED(const_Red, redPWM, 255, 1);
    redPWM   = setLED(const_Red, redPWM, 0, 1);
  } else {
    if (digitalRead(const_Float1b) == LOW) {
      float1Go = 'Y';
    } else {
      float1Go = 'N';
    }
    if (digitalRead(const_Float2b) == LOW ) {
      float2Go = 'Y';
    } else {
      float2Go = 'N';
    }
    controlPump();
  }
}


void controlPump() {
  if (float1Go == 'Y' && float2Go == 'Y') {
    if (floatsTriggered == 0) {
      floatsTriggered = millis();
      //      bluePWM  = setLED(const_Blue, bluePWM, 0, 1);
      //      redPWM   = setLED(const_Red, redPWM, 0, 1);
      //      greenPWM = setLED(const_Green, greenPWM, 255, 5);
      //      greenPWM = setLED(const_Green, greenPWM, 125, 5);
    } else if ((millis() - floatsTriggered) > floatWait) {
      if ((startTU != 0) &&
          ((millis() - startTU) > pumpMaxRun)) {
        errorState = 51;
        digitalWrite(const_Pump, LOW);
        pumpOn = 'N';
      } else {
        if (startTU == 0 ) {
          startTU    = millis();
          errorState = 0; // turn off any alert for not having run for
        }
        lastTU = millis();
        digitalWrite(const_Pump, HIGH);
        pumpOn = 'Y';
        redPWM   = setLED(const_Red, redPWM, 0, 1);
        greenPWM = setLED(const_Green, greenPWM, 0, 1);
        bluePWM  = setLED(const_Blue, bluePWM, 255, 1);
      }
    }
  }
  if (pumpOn == 'Y') {
    if (((millis() - startTU) > pumpMinRun) && (float1Go == 'N' || float2Go == 'N')) {
      digitalWrite(const_Pump, LOW);
      pumpOn = 'N';
      bluePWM = setLED(const_Blue, bluePWM, 0, 1);
      startTU = 0;
    }
  } else {
    if ((millis() - lastTU) > maxWithoutTU) {
      errorState = 1;
      greenPWM = setLED(const_Green, greenPWM, 0, 1);
      bluePWM  = setLED(const_Blue, bluePWM, 0, 1);
      redPWM   = setLED(const_Red, redPWM, 255, 1);
      redPWM   = setLED(const_Red, redPWM, 0, 1);
    } else if (float1Go == 'N' && float2Go == 'N') {
      floatsTriggered = 0;
      // both floats off could mean one has stuck! Low level alarm
      greenPWM = setLED(const_Green, greenPWM, 0, 1);
      bluePWM  = setLED(const_Blue, bluePWM, 0, 1);
      redPWM   = setLED(const_Red, redPWM, 40, 25);
      redPWM   = setLED(const_Red, redPWM, 15, 25);
    } else if (errorState == 0) {
      if (float1Go == 'N' || float2Go == 'N') {
        floatsTriggered = 0;
      }
      bluePWM  = setLED(const_Blue, bluePWM, 0, 1);
      redPWM   = setLED(const_Red, redPWM, 0, 1);
      greenPWM = setLED(const_Green, greenPWM, 255, 5);
      greenPWM = setLED(const_Green, greenPWM, 125, 5);
    }
  }
}

int setLED(byte pin, int prevPWM, int newPWM, int fadeDelay) {
  unsigned long lastMillis    = 0;
  unsigned long currentMillis = 0;
  int increment               = 0;
  int currentPWM              = prevPWM;
  if (prevPWM < newPWM) {
    increment =  1;
  } else {
    increment = -1;
  }
  for (; currentPWM != newPWM; ) {
    currentMillis = millis();
    if ((currentMillis - lastMillis) > fadeDelay) {
      lastMillis = currentMillis;
      currentPWM = currentPWM + increment;
      analogWrite(pin, currentPWM);
    }
  }
  return currentPWM;
}
