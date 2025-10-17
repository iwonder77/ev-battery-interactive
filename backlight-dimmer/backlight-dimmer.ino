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

  // clear the CS3x bits (bits 2:0) for Timer/Counter3 Control Register B (TCCR3B)
  // and set them to the following:
  //    - CS32: 0
  //    - CS31: 1
  //    - CS30: 0
  // this modifies the prescale value for
  // Timer/Counter3 to 8 (see https://ww1.microchip.com/downloads/en/devicedoc/atmel-7766-8-bit-avr-atmega16u4-32u4_datasheet.pdf)
  // so now the PWM frequency (for phase correct 8-bit pwm mode) is
  // Frequency = CPU Clock Hz / (Prescaler * 2 * TOP)
  TCCR3B = (TCCR3B & 0b11111000) | 0b010;
  // NOTE: arduino core sets the WGM3x bits to 0001, which corresponds to the PWM, Phase-Correct, 8-bit mode where TOP is 0x00FF (255), so our frequency
  // would now be
  // 16,000,000 / (8 * 2 * 255) = 3.9 kHz

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
