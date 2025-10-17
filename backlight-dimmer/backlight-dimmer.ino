// Smooth, non-blocking PWM control for MOSFET module
const int POT_PIN = A1;
const int PWM_OUT_PIN = 5;                  // you may choose a different pin if you change timers
const int READ_INTERVAL_MS = 20;            // how often to sample pot
const float ALPHA = 0.12;                   // smoothing factor: 0..1 (lower = smoother/slower)
const int DEAD_BAND = 2;                    // ADC counts (0..1023)
const int RAMP_STEP = 1;                    // PWM step when ramping
const unsigned long RAMP_INTERVAL_MS = 10;  // time between ramp steps

unsigned long lastReadMs = 0;
unsigned long lastRampMs = 0;
float smoothADC = 0.0;
int targetPWM = 0;
int currentPWM = 0;

void setup() {
  pinMode(PWM_OUT_PIN, OUTPUT);
  analogWrite(PWM_OUT_PIN, 0);
  // initialize smoothing with initial reading
  int init = analogRead(POT_PIN);
  smoothADC = init;
  currentPWM = 0;
  targetPWM = 0;
}

int adcToPWM(int adc) {
  return map(adc, 0, 1023, 0, 255);
}

void loop() {
  unsigned long now = millis();

  // read pot at interval
  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;
    int raw = analogRead(POT_PIN);
    // exponential smoothing
    smoothADC = (ALPHA * raw) + ((1.0 - ALPHA) * smoothADC);
    int smoothInt = int(smoothADC + 0.5);

    // only update target when change exceeds deadband
    int newTarget = adcToPWM(smoothInt);
    if (abs(newTarget - targetPWM) > DEAD_BAND) {
      targetPWM = newTarget;
    }
  }

  // ramp currentPWM toward targetPWM (non-blocking)
  if (now - lastRampMs >= RAMP_INTERVAL_MS) {
    lastRampMs = now;
    if (currentPWM < targetPWM) {
      currentPWM = min(currentPWM + RAMP_STEP, targetPWM);
      analogWrite(PWM_OUT_PIN, currentPWM);
    } else if (currentPWM > targetPWM) {
      currentPWM = max(currentPWM - RAMP_STEP, targetPWM);
      analogWrite(PWM_OUT_PIN, currentPWM);
    }
  }

  // other tasks can run here without being blocked
}
