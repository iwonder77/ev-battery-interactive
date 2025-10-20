/* 
* ----------------------------------------------
* PROJECT NAME: EV Battery Interactive
* File name: quinled.ino
* Description: LED logic for the QuinLED Dig Uno boards that drive the 12V WS2815 LED strips used in the EV battery interactive
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
unsigned long lastFrameTime = 0;
uint16_t frameRateMs = 20;              // frame period in ms (20ms = 50fps)
float animationSpeedMultiplier = 1.0f;  // controls animation progression speed

// --- sliding window animation variables ---
const uint8_t PIXEL_SPACING = 10;  // spacing between lit pixels
int leadingPixel = 0;
int trailingPixel = 0;
float pixelShift = 0;  // controls shifting of lit pixels (sliding window)

// float accumulators for smooth sub-pixel progression
float leadingPixelFloat = 0;
float trailingPixelFloat = 0;

bool animationActive = false;
bool trailingPixelStart = false;
float hue = 0;

void animationSlidingWindow(CRGB::HTMLColorCode color) {
  unsigned long now = millis();

  //only update animation if enough time has passed
  if (now - lastFrameTime < frameRateMs) {
    return;
  }
  lastFrameTime = now;

  // Clear red strip - only green strip should be active
  fill_solid(rleds, NUM_LEDS_RED_STRIP, CRGB::Black);

  for (int i = 0; i < NUM_LEDS_GREEN_STRIP; i++) {
    if (((i - (int)pixelShift) % PIXEL_SPACING) == 0) {
      gleds[i] = color;
    }
  }

  // leading pixel reveals animation from left to right (0 -> NUM_LEDS)
  if (leadingPixel < NUM_LEDS_GREEN_STRIP) {
    fill_solid(gleds + leadingPixel + 1, (NUM_LEDS_GREEN_STRIP - leadingPixel - 1), CRGB::Black);
    // progress the leading pixel smoothly
    leadingPixelFloat += animationSpeedMultiplier;
    if (leadingPixelFloat >= 1.0) {
      leadingPixel++;
      leadingPixelFloat -= 1.0;
    }
  }

  if (now - animationStart > ANIMATION_DURATION_MS) {
    if (trailingPixelStart == false && pixelShift < 1.0) {
      trailingPixelStart = true;
    }

    if (trailingPixelStart && trailingPixel < NUM_LEDS_GREEN_STRIP) {
      // Fill from start to trailing pixel with black
      fill_solid(gleds, trailingPixel, CRGB::Black);

      // Progress the trailing pixel smoothly
      trailingPixelFloat += animationSpeedMultiplier;
      if (trailingPixelFloat >= 1.0) {
        trailingPixel++;
        trailingPixelFloat -= 1.0;
      }
    }
  }

  // Fade effect for smoother transitions
  fadeToBlackBy(gleds, NUM_LEDS_GREEN_STRIP, (256 / PIXEL_SPACING) * 4);

  // Progress the sliding window animation based on speed multiplier
  pixelShift += animationSpeedMultiplier;
  if (pixelShift >= PIXEL_SPACING) {
    pixelShift -= PIXEL_SPACING;
  }

  FastLED.show();
}

void animationRedGlow() {
  unsigned long now = millis();

  // Fixed: Only update if enough time HAS passed (was inverted)
  if (now - lastFrameTime < frameRateMs) {
    return;
  }
  lastFrameTime = now;

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

  // Frame rate control for consistent timing
  if (now - lastFrameTime < frameRateMs) {
    return;
  }
  lastFrameTime = now;

  // Clear green strip in default mode
  fill_solid(gleds, NUM_LEDS_GREEN_STRIP, CRGB::Black);

  // Rainbow effect on red strip
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
    lastFrameTime = now - frameRateMs;
    pixelShift = 0;
    leadingPixel = 0;
    leadingPixelFloat = 0;
    trailingPixel = 0;
    trailingPixelFloat = 0;
    trailingPixelStart = false;
    animationActive = true;
    fill_solid(rleds, NUM_LEDS_RED_STRIP, CRGB::Black);
    FastLED.show();
  } else if (failureTriggered && currentMode != FAILURE) {
    // Trigger FAILURE animation
    currentMode = FAILURE;
    animationStart = now;
    lastFrameTime = now - frameRateMs;
    pixelShift = 0;
    leadingPixel = 0;
    leadingPixelFloat = 0;
    trailingPixel = 0;
    trailingPixelFloat = 0;
    trailingPixelStart = false;
    animationActive = true;
    fill_solid(gleds, NUM_LEDS_GREEN_STRIP, CRGB::Black);
    FastLED.show();
  }

  // Return to IDLE if animation duration has expired (regardless of pin state)
  if ((currentMode == SUCCESS || currentMode == FAILURE) && (now - animationStart >= ANIMATION_DURATION_MS)) {
    currentMode = IDLE;
    animationActive = false;
    lastFrameTime = now - frameRateMs;

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
