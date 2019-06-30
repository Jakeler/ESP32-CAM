#include <Arduino.h>

#define FASTLED_INTERNAL // disable pragma messages
#include <FastLED.h>

#include "led.h"
#include "pins.h"

CRGB leds[LED_COUNT];

void LED::init() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  FastLED.setCorrection(LEDColorCorrection::TypicalSMD5050);
  FastLED.clear(true);
}

void LED::showCapture() {
  pauseRing(300);
  pulse(0, 0, 0, true);
  pauseRing(300);
}
void LED::showWin() {
  pulse(255/3, 255, 3, true);
  pulse(255/3, 255, 3, false);
}
void LED::pauseRing(int time) {
  FastLED.clear(true);
  delay(time);
}
void LED::ranking(float percentage) {
  uint8_t lit = percentage*LED_COUNT;
  float remain = percentage*float(LED_COUNT) - lit;
  uint8_t steps = 5;

  if(lit > 17) { // Full green for winning team
    fill_solid(leds, LED_COUNT, CRGB(0,255,0));
    FastLED.show();
    return;
  }

  FastLED.clear();
  // Red to Green (hue = 85)
  fill_rainbow(leds, lit, 0, steps);
  leds[lit] = CHSV(lit*steps, 255, remain*255);
  FastLED.show();
}
void LED::pulse(uint8_t hue, uint8_t sat, uint32_t wait, bool powerdown) {
  uint8_t passes = powerdown? 2 : 1;

  for(size_t i = 0; i < 256*passes; i++)
  {
    FastLED.showColor(CHSV(hue, sat, i <= 255? i : 255 - i));
    //delay(wait);
  }
  
}
void LED::startup() {
  CRGB color = CRGB::Blue;

  CRGB temp[9];
  fill_gradient_RGB(temp, 9, CRGB::Black, color);
  napplyGamma_video(temp, 9, 2.0);

  for (size_t start = 0; start < LED_COUNT*3; start++) {
    for (size_t i = 0; i < 9; i++) {
      leds[(start+i)%LED_COUNT] = temp[i];
    }
    FastLED.show();
    delay(50);
  }  
  
}