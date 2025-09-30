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

#define NUM_LEDS 144
#define DATA_PIN 3
#define PIXEL_SPACING 8  // spacing between
#define BRIGHTNESS 1     // as a percentage (%)
#define TIMEOUT 30000    // how long to wait after power on

CRGB leds[NUM_LEDS];  // array to set apart a block of memory to store and manipulate LED data

int pixelShift = 0;
int leadingPixel = 0;
int trailingPixel = 0;
bool trailingPixelStart = false;

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255.0 * BRIGHTNESS);
}

void loop() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (((i - pixelShift) % PIXEL_SPACING) == 0) {
      // NOTE: change color here to Red or Green depending on which driver you are uploading to
      leds[i] = CRGB::Red;
    }
  }

  if (leadingPixel <= NUM_LEDS) {
    fill_solid(leds + leadingPixel++, (NUM_LEDS - leadingPixel) + 1, CRGB::Black);
  }

  if (millis() > TIMEOUT) {
    if (trailingPixelStart == false && pixelShift == 0) {
      trailingPixelStart = true;
    }

    if (trailingPixel <= NUM_LEDS) {
      fill_solid(leds, trailingPixel++, CRGB::Black);
    } else {
      while (true);  // hang until restarted by reed switch circuit
    }
  }

  fadeToBlackBy(leds, NUM_LEDS, (256 / PIXEL_SPACING) * 4);
  pixelShift = (pixelShift + 1) % PIXEL_SPACING;
  FastLED.show();

  delay(33);
}
