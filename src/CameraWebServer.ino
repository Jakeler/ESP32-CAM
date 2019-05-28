#include "Arduino.h"
#include <WiFi.h>

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "camera_config.h"

#define FASTLED_INTERNAL // disable pragma messages
#include <FastLED.h>


#include "WebServer.h"
WebServer server(80);


//Replace with your network credentials
const char* ssid = "TestNetz";
const char* password = "6vigCGAU";


#define LED_PIN   2
#define LED_COUNT 18
// Declare our NeoPixel strip object:
CRGB leds[LED_COUNT];

camera_fb_t * fb = NULL;
uint8_t * _jpg_buf = NULL;
size_t _jpg_buf_len = 0;
esp_err_t res = ESP_OK;


void initCamera() {
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}
void capture() {
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    res = ESP_FAIL;
  } else {
    if(fb->width > 400){
      if(fb->format != PIXFORMAT_JPEG){
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;
        if(!jpeg_converted){
          Serial.println("JPEG compression failed");
          res = ESP_FAIL;
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }
  }
}
void clearFb() {
  if(fb){
    esp_camera_fb_return(fb);
    fb = NULL;
    _jpg_buf = NULL;
  } else if(_jpg_buf){
    free(_jpg_buf);
    _jpg_buf = NULL;
  }
  //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));

}

void lostConnection(WiFiEvent_t event, WiFiEventInfo_t info) {
  pulse(0, 255, 3, true);
}
void showRing() {
  startup(3, 200);
  pauseRing(500);
  ranking(0.5);
}
void showCapture() {
  pauseRing(300);
  pulse(0, 0, 0, true);
  pauseRing(300);
  ranking(0.5);
}
void showWin() {
  for(float i = 0.1; i<1.0; i+= 0.01) {
    ranking(i);
    delay(30);
  }
  pulse(255/3, 255, 3, true);
  pulse(255/3, 255, 3, false);
}
void pauseRing(int time) {
  FastLED.clear(true);
  delay(time);
}
void ranking(float percentage) {
  uint8_t lit = percentage*LED_COUNT;
  float remain = percentage*float(LED_COUNT) - lit;
  uint8_t steps = 5;

  FastLED.clear();
  // Red to Green (hue = 85)
  fill_rainbow(leds, lit, 0, steps);
  leds[lit] = CHSV(lit*steps, 255, remain*255);
  FastLED.show();
}
void pulse(uint8_t hue, uint8_t sat, uint32_t wait, bool powerdown) {
  uint8_t passes = powerdown? 2 : 1;

  for(size_t i = 0; i < 256*passes; i++)
  {
    FastLED.showColor(CHSV(hue, sat, i <= 255? i : 255 - i));
    //delay(wait);
  }
  
}
void startup(uint16_t runs, uint32_t wait) {
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


void serveImage() {
  showCapture();
  capture();
  server.send_P(200, "image/jpeg", (char*)_jpg_buf, _jpg_buf_len);
  clearFb();
}

void serveWelcome() {
  server.send(200, "text/plain", "Go to /pic.jpg to get an image!");
}

void serveFlashOn() {
  digitalWrite(FLASH_PIN, 1);
  server.send(200, "text/plain", "OK");
}
void serveFlashOff() {
  digitalWrite(FLASH_PIN, 0);
  server.send(200, "text/plain", "OK");
}
  


void setup() { 
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  FastLED.showColor(CRGB::Red);
  
  // Camera init
  initCamera();

  FastLED.clear(true);

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  WiFi.onEvent(lostConnection, WiFiEvent_t::SYSTEM_EVENT_ETH_DISCONNECTED);
  
  // Start streaming web server
  //startCameraServer();
  server.on("/pic.jpg", serveImage);
  server.on("/", serveWelcome);
  server.on("/flash/on", serveFlashOn);
  server.on("/flash/off", serveFlashOff);
  server.on("/led/start", showRing);
  server.on("/led/capture", showCapture);
  server.on("/led/win", showWin);
  server.begin();
  Serial.print("Server Ready! Go to: http://");
  Serial.print(WiFi.localIP());

  // Enable Flash
  pinMode(FLASH_PIN, OUTPUT);
}

void loop() {
  server.handleClient();
  delay(1);
}

