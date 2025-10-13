// define pins for reed switches
#define POSITIVE_REED_PIN 11
#define CENTER_REED_PIN 12

// define digital output pins for success/failure
#define SUCCESS_OUT_PIN 5
#define FAILURE_OUT_PIN 6

const unsigned long REED_DEBOUNCE_MS = 100;
const unsigned long HIGH_PULSE_MS = 100;

unsigned long lastCenterChange = 0;
unsigned long lastPositiveChange = 0;
bool centerClosed = false;
bool positiveClosed = false;
unsigned long successPulseEnd = 0;
unsigned long failurePulseEnd = 0;


void setup() {
  pinMode(CENTER_REED_PIN, INPUT_PULLUP);
  pinMode(POSITIVE_REED_PIN, INPUT_PULLUP);

  pinMode(FAILURE_OUT_PIN, OUTPUT);
  pinMode(SUCCESS_OUT_PIN, OUTPUT);

  digitalWrite(FAILURE_OUT_PIN, LOW);
  digitalWrite(SUCCESS_OUT_PIN, LOW);
}

void loop() {
  unsigned long now = millis();

  bool centerReading = (digitalRead(CENTER_REED_PIN) == LOW);
  delay(500);
  bool positiveReading = (digitalRead(POSITIVE_REED_PIN) == LOW);

  if (centerReading != centerClosed && (now - lastCenterChange > REED_DEBOUNCE_MS)) {
    centerClosed = centerReading;
    lastCenterChange = now;
  }

  if (positiveReading != positiveClosed && (now - lastPositiveChange > REED_DEBOUNCE_MS)) {
    positiveClosed = positiveReading;
    lastPositiveChange = now;
  }

  if (centerClosed && positiveClosed) {
    digitalWrite(SUCCESS_OUT_PIN, HIGH);
    successPulseEnd = now + HIGH_PULSE_MS;
  } else if (centerClosed && !positiveClosed) {
    digitalWrite(FAILURE_OUT_PIN, HIGH);
    failurePulseEnd = now + HIGH_PULSE_MS;
  }

  if (now >= successPulseEnd) {
    digitalWrite(SUCCESS_OUT_PIN, LOW);
    successPulseEnd = 0;
  }

  if (now >= failurePulseEnd) {
    digitalWrite(FAILURE_OUT_PIN, LOW);
    failurePulseEnd = 0;
  }
  delay(500);
}
