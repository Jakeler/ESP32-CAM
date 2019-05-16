#include <Arduino.h>

// PIN def: https://github.com/SeeedDocument/Outsourcing/tree/master/113990580%20ESP32-CAM
#define FLASH_PIN 4 // Conflicts SD DATA1!

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(FLASH_PIN, OUTPUT);
  Serial.begin(BAUD);
}

void loop()
{
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(FLASH_PIN, HIGH);
  // wait for a second
  delay(1000);
  // turn the LED off by making the voltage LOW
  digitalWrite(FLASH_PIN, LOW);
   // wait for a second
  delay(1000);
  Serial.println("Hello World!");
}