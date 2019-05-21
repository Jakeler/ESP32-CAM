#include "Arduino.h"
#include <WiFi.h>

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "camera_config.h"


#include <Adafruit_NeoPixel.h>

#include "WebServer.h"
WebServer server(80);


//Replace with your network credentials
const char* ssid = "TestNetz";
const char* password = "6vigCGAU";


#define LED_PIN   2
#define LED_COUNT 18
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint8_t currentLed = 0;

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


void serveImage() {
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

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.fill(strip.Color(255, 0, 0));
  strip.show();
  
  // Camera init
  initCamera();

  strip.fill(strip.Color(0, 0, 255));
  strip.show();

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start streaming web server
  //startCameraServer();
  server.on("/pic.jpg", serveImage);
  server.on("/", serveWelcome);
  server.on("/flash/on", serveFlashOn);
  server.on("/flash/off", serveFlashOff);
  server.begin();
  Serial.print("Server Ready! Go to: http://");
  Serial.print(WiFi.localIP());

  // Enable Flash
  pinMode(FLASH_PIN, OUTPUT);
}

void loop() {
  server.handleClient();

  delay(100);
  strip.clear();
  strip.setPixelColor(currentLed, strip.Color(0, 255, 0));
  strip.show();
  currentLed++;
  currentLed %= LED_COUNT;
}

