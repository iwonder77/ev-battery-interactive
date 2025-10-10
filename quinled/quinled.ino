/* 
* ----------------------------------------------
* PROJECT NAME: EV Battery Interactive
* File name: ev-battery-interactive.ino
* Description: LED logic for the QuinLED Dig Uno boards that drive the 12V WS2815 LED strips used in the EV battery interactive
* 
* Original Author: Mike Heaton
* Modified By: Isai Sanchez
* Changes:
*   - added comments to explain LED pixel logic
* Date: 9-30-25
* Board Used: QuinLED Dig-Uno (ESP32 based Module)
* Notes:
*   - LED color pattern must be changed between green or red to match driver corresponding to
*     correct/incorrect battery placement scenario
* ----------------------------------------------
*/

#include <FastLED.h>

#define NUM_LEDS 18
#define DATA_PIN 2
#define PIXEL_SPACING 4  // spacing between lit pixels
#define BRIGHTNESS 15    // brightness of LEDs as a percentage (%)
#define TIMEOUT 10000    // how long the animation runs before resetting (ms)

CRGB leds[NUM_LEDS];  // array to set apart a block of memory to store and manipulate LED data

int pixelShift = 0;     // controls shifting of lit pixels (sliding window)
int leadingPixel = 0;   // tracks the "head" of blackout progression
int trailingPixel = 0;  // tracks the "tail" of blackout progression
bool trailingPixelStart = false;

unsigned long startTime;  // records when the current animation cycle started

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness((255 * BRIGHTNESS) / 100);
  startTime = millis();
}

void loop() {
  // --- sliding window logic ---
  // 1. every frame we check which pixels fall exactly on the spacing boundary relative to pixelShift
  // 2. those pixels are lit
  // 3. pixelShift increments each frame (with wraparound) so the lit pixels appear to slide down the
  // strip as the modulo selects new indices
  for (int i = 0; i < NUM_LEDS; i++) {
    if (((i - pixelShift) % PIXEL_SPACING) == 0) {
      // NOTE: change color here to Red or Green depending on which driver you are uploading to
      leds[i] = CRGB::Red;
    }
  }

  // blackout the LEDs progressively from the head (leadingPixel forward)
  if (leadingPixel <= NUM_LEDS) {
    fill_solid(leds + leadingPixel++, (NUM_LEDS - leadingPixel) + 1, CRGB::Black);
  }

  // after timeout, begin the trailing blackout sequence
  if (millis() - startTime > TIMEOUT) {
    if (trailingPixelStart == false && pixelShift == 0) {
      trailingPixelStart = true;
    }

    if (trailingPixel <= NUM_LEDS) {
      fill_solid(leds, trailingPixel++, CRGB::Black);
    } else {
      // instead of hanging forever, reset all counters and restart animation
      pixelShift = 0;
      leadingPixel = 0;
      trailingPixel = 0;
      trailingPixelStart = false;
      FastLED.clear(true);   // clear LEDs immediately
      startTime = millis();  // restart cycle timer
    }
  }

  // apply fading to create a trailing effect behind lit pixels
  fadeToBlackBy(leds, NUM_LEDS, (256 / PIXEL_SPACING) * 4);
  // increment pixelShift to slide the window
  pixelShift = (pixelShift + 1) % PIXEL_SPACING;
  // update strip
  FastLED.show();

  delay(33);  // animation speed controlled here
}
