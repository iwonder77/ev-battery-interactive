/* 
* ----------------------------------------------
* PROJECT NAME: EV Battery Interactive
* File name: quinled.ino
* Description: LED logic for the QuinLED Dig Uno board driving the 12V WS2815 LED strips used in the EV battery interactive
* 
* Original Author: Mike Heaton
* Modified By: Isai Sanchez
* Changes:
*   - added comments to explain LED pixel logic
*   - centralized 2 scenarios (right or wrong battery placement) to this QuinLED board, red or green animation plays
*     depending on signal received from nano
* Date: 9-30-25
* Board Used: QuinLED Dig-Uno (ESP32 based Module)
* Notes:
*   - LED color determined by signal received by nano
* ----------------------------------------------
*/

#include <FastLED.h>

#define NUM_LEDS_GREEN_STRIP 350
#define NUM_LEDS_RED_STRIP 180
#define GREEN_STRIP_DATA_PIN 3
#define RED_STRIP_DATA_PIN 16
#define SUCCESS_INPUT_PIN 15  // Q1 on the QuinLED Dig uno board
#define FAILURE_INPUT_PIN 12  // Q2 on the QuinLED Dig uno board
#define BRIGHTNESS 75         // brightness of LEDs as a percentage (%)

CRGB gleds[NUM_LEDS_GREEN_STRIP];
CRGB rleds[NUM_LEDS_RED_STRIP];

enum AnimationMode { IDLE,
                     SUCCESS,
                     FAILURE };
AnimationMode currentMode = IDLE;

// --- animation timing control ---
unsigned long animationStart = 0;
const unsigned long ANIMATION_DURATION_MS = 7000;
unsigned long lastFrameTimeMs = 0;
uint16_t frameRateFPS = 60;  // frame period in ms (20ms = 50fps)

// --- sliding window animation variables ---
const uint8_t PIXEL_SPACING = 10;  // spacing between lit pixels
const int WINDOW_PIXELS = 6;
const float ANIMATION_SPEED_MULTIPLIER = 50.0f;
float headPos = float(NUM_LEDS_GREEN_STRIP - 1);
float leadingEdge = float(NUM_LEDS_GREEN_STRIP - 1);

bool animationActive = false;
float hue = 0;

inline int positiveModulo(int a, int m) {
  int r = a % m;
  if (r < 0) r += m;
  return r;
}

void animationSlidingWindow(CRGB::HTMLColorCode color) {
  unsigned long now = millis();
  unsigned long dtMs = now - lastFrameTimeMs;

  if (dtMs < 1000 / frameRateFPS) return;
  lastFrameTimeMs = now;

  // clear red strip - only green strip should be active
  fill_solid(rleds, NUM_LEDS_RED_STRIP, CRGB::Black);

  // advance head and leading edge backward (right -> left ) based on elapsed time (time-based motion)
  float dtSec = dtMs / 1000.0f;
  float period = (float)(WINDOW_PIXELS) + PIXEL_SPACING;
  headPos -= ANIMATION_SPEED_MULTIPLIER * dtSec;  // pos = velocity * change in time
  leadingEdge -= ANIMATION_SPEED_MULTIPLIER * 1.5f * dtSec;

  // wrap headPos and clamp leading edge at beginning of strip
  if (headPos < 0) headPos += period;
  if (leadingEdge < 0) leadingEdge = 0;

  // draw pattern across strip from LAST led to FIRST led
  for (int i = NUM_LEDS_GREEN_STRIP - 1; i >= 0; i--) {
    if (i < leadingEdge) {
      gleds[i] = CRGB::Black;  // LED not reached yet
      continue;
    }
    // compute relative position from head to this LED (reversed direction)
    int relativePos = positiveModulo((int)floor(headPos) - i, (int)period);
    if (relativePos < WINDOW_PIXELS) {
      // if LED is inside window add gradient (fade near tail)
      uint8_t bright;
      if (WINDOW_PIXELS <= 1) bright = 255;
      else bright = map(relativePos, 0, WINDOW_PIXELS - 1, 255, 100);
      CRGB scaled = color;
      scaled.nscale8_video(bright);
      gleds[i] = scaled;
    } else {
      gleds[i] = CRGB::Black;
    }
  }

  FastLED.show();
}

void animationRedGlow() {
  unsigned long now = millis();
  unsigned long dtMs = now - lastFrameTimeMs;

  if (dtMs < 1000 / frameRateFPS) {
    return;
  }
  lastFrameTimeMs = now;

  // Clear green strip - only red strip should be active
  fill_solid(gleds, NUM_LEDS_GREEN_STRIP, CRGB::Black);

  // Breathing effect using sine wave
  // Adjust the divisor (100.0) to control breathing speed: higher = slower
  float breathe = (sin(now / 500.0 * PI) + 1.0) / 2.0;  // 0.0 to 1.0

  // Map to brightness range (50-255 for a nice glow, never fully off)
  uint8_t brightness = 50 + (breathe * 205);

  // Fill strip with red at calculated brightness
  fill_solid(rleds, NUM_LEDS_RED_STRIP, CRGB::Red);
  fadeToBlackBy(rleds, NUM_LEDS_RED_STRIP, 255 - brightness);

  FastLED.show();
}

void animationDefault() {
  unsigned long now = millis();

  // frame rate control for consistent timing
  if (now - lastFrameTimeMs < 1000 / frameRateFPS) {
    return;
  }
  lastFrameTimeMs = now;

  // clear green strip in default mode
  fill_solid(gleds, NUM_LEDS_GREEN_STRIP, CRGB::Black);

  // rainbow effect on red strip
  fill_solid(rleds, NUM_LEDS_RED_STRIP, CHSV(hue, 80, 180));
  hue += 0.5;  // Increment hue for rainbow cycling

  FastLED.show();
}

void setup() {
  FastLED.addLeds<WS2815, GREEN_STRIP_DATA_PIN, RGB>(gleds, NUM_LEDS_GREEN_STRIP);
  FastLED.addLeds<WS2815, RED_STRIP_DATA_PIN, RGB>(rleds, NUM_LEDS_RED_STRIP);
  FastLED.setBrightness((255 * BRIGHTNESS) / 100);

  pinMode(SUCCESS_INPUT_PIN, INPUT_PULLDOWN);
  pinMode(FAILURE_INPUT_PIN, INPUT_PULLDOWN);

  fill_solid(gleds, NUM_LEDS_GREEN_STRIP, CRGB::Black);
  fill_solid(rleds, NUM_LEDS_RED_STRIP, CRGB::Black);
  FastLED.show();
}

void loop() {
  unsigned long now = millis();

  bool successTriggered = digitalRead(SUCCESS_INPUT_PIN);
  bool failureTriggered = digitalRead(FAILURE_INPUT_PIN);

  // State transitions: prioritize returning to IDLE after animation completes
  if (successTriggered && currentMode != SUCCESS) {
    // Trigger SUCCESS animation
    currentMode = SUCCESS;
    animationStart = now;
    animationActive = true;
    fill_solid(rleds, NUM_LEDS_RED_STRIP, CRGB::Black);
    FastLED.show();
  } else if (failureTriggered && currentMode != FAILURE) {
    // Trigger FAILURE animation
    currentMode = FAILURE;
    animationStart = now;
    animationActive = true;
    fill_solid(gleds, NUM_LEDS_GREEN_STRIP, CRGB::Black);
    FastLED.show();
  }

  // Return to IDLE if animation duration has expired (regardless of pin state)
  if ((currentMode == SUCCESS || currentMode == FAILURE) && (now - animationStart >= ANIMATION_DURATION_MS)) {
    currentMode = IDLE;
    animationActive = false;
    leadingEdge = NUM_LEDS_GREEN_STRIP - 1;  // reset leading edge

    // immediately clear both strips on transition to IDLE
    fill_solid(gleds, NUM_LEDS_GREEN_STRIP, CRGB::Black);
    fill_solid(rleds, NUM_LEDS_RED_STRIP, CRGB::Black);
    FastLED.show();
  }

  // Execute the appropriate animation based on current mode
  switch (currentMode) {
    case SUCCESS:
      animationSlidingWindow(CRGB::Green);
      break;
    case FAILURE:
      animationRedGlow();
      break;
    case IDLE:
    default:
      animationDefault();
      break;
  }
}
