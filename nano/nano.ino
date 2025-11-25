/* 
* ----------------------------------------------
* PROJECT NAME: EV Battery Interactive
* File name: nano.ino
* Description: this sketch handles the reed switch logic for the battery pack, sending success/failure signals to the 
*               quin led board according to the battery placement
* 
* Author: Isai Sanchez
* Date: 10-10-2025
* Board Used: Arduino Nano
* Notes:
*   - State Machine used for better UX
* ----------------------------------------------
*/

#include <avr/wdt.h>

// --- REED SWITCH PINS ---
#define POSITIVE_REED_PIN 11
#define CENTER_REED_PIN 12

// --- SUCCESS/FAILURE OUTPUT PINS ---
#define SUCCESS_OUT_PIN 5
#define FAILURE_OUT_PIN 6

// --- TIMING CONSTANTS ---
const unsigned long REED_DEBOUNCE_MS = 50;   // debounce time for reed switches
const unsigned long SETTLING_TIME_MS = 300;  // grace period after center closes
const unsigned long SIGNAL_PULSE_MS = 100;   // duration of HIGH pulse for success/failure
const unsigned long COOLDOWN_MS = 500;       // cooldown before accepting new battery placement read

// --- STATE MACHINE ---
enum State {
  IDLE,               // no battery detected
  BATTERY_INSERTING,  // center closed, waiting to see if positive closes
  EVAL_COMPLETE,      // success or failure signal sent, waiting for removal
  COOLDOWN            // battery removed, brief cooldown before next insertion
};

State currentState = IDLE;

// --- DEBOUNCING VARIBALES ---
unsigned long lastCenterDebounceTime = 0;
unsigned long lastPositiveDebounceTime = 0;
bool centerLastReading = false;
bool positiveLastReading = false;
bool centerStable = false;
bool positiveStable = false;

// --- OTHER TIMING VARIABLES ---
unsigned long stateEntryTime = 0;
unsigned long signalPulseStart = 0;


void setup() {
  // disable watchdog if it reset the nano
  wdt_disable();
  delay(1000);
  // re-enable watchdog with 2s timeout
  wdt_enable(WDTO_2S);

  pinMode(CENTER_REED_PIN, INPUT_PULLUP);
  pinMode(POSITIVE_REED_PIN, INPUT_PULLUP);

  pinMode(FAILURE_OUT_PIN, OUTPUT);
  pinMode(SUCCESS_OUT_PIN, OUTPUT);

  digitalWrite(FAILURE_OUT_PIN, LOW);
  digitalWrite(SUCCESS_OUT_PIN, LOW);
}

void loop() {
  unsigned long now = millis();

  // read both switches simultaneously
  bool centerReading = (digitalRead(CENTER_REED_PIN) == LOW);
  bool positiveReading = (digitalRead(POSITIVE_REED_PIN) == LOW);

  // debounce both switches
  // center:
  if (centerReading != centerLastReading) {
    lastCenterDebounceTime = now;
  }
  if ((now - lastCenterDebounceTime) > REED_DEBOUNCE_MS) {
    centerStable = centerReading;  // reading has been stable for debounce period
  }
  centerLastReading = centerReading;
  // positive:
  if (positiveReading != positiveLastReading) {
    lastPositiveDebounceTime = now;
  }
  if ((now - lastPositiveDebounceTime) > REED_DEBOUNCE_MS) {
    positiveStable = positiveReading;
  }
  positiveLastReading = positiveReading;

  // manage signal pulses (turn off after pulse duration)
  if (signalPulseStart > 0 && (now - signalPulseStart >= SIGNAL_PULSE_MS)) {
    digitalWrite(SUCCESS_OUT_PIN, LOW);
    digitalWrite(FAILURE_OUT_PIN, LOW);
    signalPulseStart = 0;
  }

  // state machine logic
  switch (currentState) {
    case IDLE:
      // waiting for battery insertion
      if (centerStable) {
        // center magnet detected - battery is being inserted
        currentState = BATTERY_INSERTING;
        stateEntryTime = now;
      }
      break;
    case BATTERY_INSERTING:
      // center is closed, waiting to see if positive will close
      // first check if battery has been removed/false center reading
      if (!centerStable) {
        // reset state to idle if so
        currentState = IDLE;
        break;
      }

      // wait for settling time to elapse
      if (now - stateEntryTime >= SETTLING_TIME_MS) {
        // settling time complete - now evaluate orientation
        if (centerStable && positiveStable) {
          // both switches closed - send success signal
          digitalWrite(SUCCESS_OUT_PIN, HIGH);
          signalPulseStart = now;
          currentState = EVAL_COMPLETE;
        } else if (centerStable && !positiveStable) {
          // only center closed - send failure signal
          digitalWrite(FAILURE_OUT_PIN, HIGH);
          signalPulseStart = now;
          currentState = EVAL_COMPLETE;
        }
      }
      break;
    case EVAL_COMPLETE:
      // signal has been sent, waiting for battery to be removed
      if (!centerStable) {
        // battery removed - enter cooldown
        currentState = COOLDOWN;
        stateEntryTime = now;
      }
      break;
    case COOLDOWN:
      // very brief cooldown period to prevent bounce when battery removed
      if (now - stateEntryTime >= COOLDOWN_MS) {
        if (!centerStable) {
          // cooldown complete and battery still not present
          currentState = IDLE;
        } else {
          // battery reinserted during cooldown - go to inserting state
          currentState = BATTERY_INSERTING;
          stateEntryTime = now;
        }
      }
      break;
  }

  wdt_reset();
  delay(10);
}
