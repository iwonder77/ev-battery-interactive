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

#define NUM_LEDS 70
#define LED_DATA_PIN 3
#define SUCCESS_INPUT_PIN 15  // Q1 on the QuinLED Dig uno board
#define FAILURE_INPUT_PIN 12  // Q2 on the QuinLED Dig uno board
#define BRIGHTNESS 50         // brightness of LEDs as a percentage (%)

CRGB leds[NUM_LEDS];  // array to set apart a block of memory to store and manipulate LED data

enum AnimationMode { IDLE,
                     SUCCESS,
                     FAILURE };
AnimationMode currentMode = IDLE;


// --- animation timing control ---
unsigned long animationStart = 0;
const unsigned long ANIMATION_DURATION_MS = 5000;

// --- sliding window animation variables ---
const uint8_t PIXEL_SPACING = 8;  // spacing between lit pixels
int leadingPixel = 0;
int trailingPixel = 0;
uint8_t pixelShift = 0;  // controls shifting of lit pixels (sliding window)
bool animationActive = false;
bool trailingPixelStart = false;
uint8_t hue = 0;

void animationSlidingWindow(CRGB::HTMLColorCode color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (((i - pixelShift) % PIXEL_SPACING) == 0) {
      leds[i] = color;
    }
  }

  if (leadingPixel <= NUM_LEDS) {
    fill_solid(leds + leadingPixel++, (NUM_LEDS - leadingPixel) + 1, CRGB::Black);
  }

  if (millis() > ANIMATION_DURATION_MS) {
    if (trailingPixelStart == false && pixelShift == 0) {
      trailingPixelStart = true;
    }

    if (trailingPixel <= NUM_LEDS) {
      fill_solid(leds, trailingPixel++, CRGB::Black);
    }
  }

  fadeToBlackBy(leds, NUM_LEDS, (256 / PIXEL_SPACING) * 4);
  pixelShift = (pixelShift + 1) % PIXEL_SPACING;
  FastLED.show();

  delay(33);
}

void animationDefault() {
  fill_solid(leds, NUM_LEDS, CHSV(hue, 80, 180));
  hue++;
  FastLED.show();
}

void setup() {
  FastLED.addLeds<WS2811, LED_DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness((255 * BRIGHTNESS) / 100);

  pinMode(SUCCESS_INPUT_PIN, INPUT_PULLDOWN);
  pinMode(FAILURE_INPUT_PIN, INPUT_PULLDOWN);
}

void loop() {
  unsigned long now = millis();

  bool successTiggered = digitalRead(SUCCESS_INPUT_PIN);
  bool failureTriggered = digitalRead(FAILURE_INPUT_PIN);

  if (successTiggered && currentMode != SUCCESS) {
    currentMode = SUCCESS;
    animationStart = now;
    pixelShift = 0;
    animationActive = true;
  } else if (failureTriggered && currentMode != FAILURE) {
    currentMode = FAILURE;
    animationStart = now;
    pixelShift = 0;
    animationActive = true;
  } else if (!successTiggered && !failureTriggered && (now - animationStart > ANIMATION_DURATION_MS)) {
    currentMode = IDLE;
    animationActive = false;
  }
  switch (currentMode) {
    case SUCCESS:
      animationSlidingWindow(CRGB::Green);
      break;
    case FAILURE:
      animationSlidingWindow(CRGB::Red);
      break;
    case IDLE:
    default:
      animationDefault();
      break;
  }
}
