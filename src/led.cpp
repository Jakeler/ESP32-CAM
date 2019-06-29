#include <Arduino.h>

#define FASTLED_INTERNAL // disable pragma messages
#include <FastLED.h>

#include "led.h"
#include "pins.h"

CRGB leds[LED_COUNT];

void LED::init() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  FastLED.showColor(CRGB::Red);
}

void LED::showRing() {
  startup(3, 200);
  pauseRing(500);
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
void LED::ranking() {
  int currentScore = 0; int maxScore = 3; // TODO Paramter
  float percentage = (float)currentScore/(float)maxScore;

  uint8_t lit = percentage*LED_COUNT;
  float remain = percentage*float(LED_COUNT) - lit;
  uint8_t steps = 5;

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
void LED::startup(uint16_t runs, uint32_t wait) {
  CRGB color = CRGB::Blue;

  for(size_t offset = 0; offset < LED_COUNT*runs; offset++)
  {
    FastLED.clear();
    for(size_t i = 0; i < LED_COUNT; i+=4)
    {
      leds[(i+offset) % LED_COUNT] = color;
      leds[(i+offset+1) % LED_COUNT] = color;
    }
    FastLED.show();
    delay(wait);
  }
}