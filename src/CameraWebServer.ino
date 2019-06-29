#include <Arduino.h>

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"

#include "camera_config.h"

#include <SPIFFS.h>
#include <FS.h>

#include <WiFi.h>
#include <PubSubClient.h>
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

#include "WebServer.h"
WebServer server(80);

#include "constants.h"
#include "pins.h"

#include "led.h"
LED led;



camera_fb_t *fb = NULL;
uint8_t *_jpg_buf = NULL;
size_t _jpg_buf_len = 0;
esp_err_t res = ESP_OK;

uint16_t currentScore = 0;
uint16_t maxScore = 1;


void initCamera() {
  Serial.println("Starting camera");
  while (esp_camera_init(&config) != ESP_OK) {
    Serial.print(".");
    delay(200);
  }
  Serial.println("Camera initialized");
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
  led.pulse(0, 255, 3, true);
}



void serveImage() {
  led.showCapture();
  
  capture();
  if(saveCurrentImage()) {
    server.send_P(200, "image/jpeg", (char*)_jpg_buf, _jpg_buf_len);
  } else {
    server.send(418, "tex/plain", "Not enough free flash!");
  }
  clearFb();

  publishScore();
}

void publishScore() {
  currentScore++;
  // Reduce unnecessary traffic
  if(currentScore < maxScore) return;

  led.showWin();

  char topic[32];
  sprintf(topic, "cup/%s/imageCount", CUP_ID);
  char value[7];
  sprintf(value, "%d", currentScore);

  mqttClient.publish(topic, value);
}

uint16_t getNextImgId() {
  uint16_t id = 0;
  String path;
  do {
    path = getImgPath(String(id));
    id++;
  }  while(SPIFFS.exists(path));

  return id-1;
}

bool saveCurrentImage() {
  if (getFreeFlash() < _jpg_buf_len) {
    Serial.println(_jpg_buf_len);
    return false;
  }

  String path = getImgPath(String(getNextImgId()));

  Serial.println("Writing to " + path);
  File imageFile = SPIFFS.open(path, FILE_WRITE);
  while(imageFile.write(_jpg_buf, _jpg_buf_len) == 0) {
    Serial.print(".");
    delay(10);
  }
  Serial.println("Wrote: "+String(imageFile.size()) + " / " + String(_jpg_buf_len));
  imageFile.close();
  return true;
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

String getImgPath(String id) {
  return "/img/" + id + ".jpg";
}

void serveOldImg() {
  if(!server.hasArg("id")) {
    server.send(400, "text/plain", "id parameter missing!");
    return;
  }
  String path = getImgPath(server.arg("id"));
  if(!SPIFFS.exists(path)) {
    server.send(404, "text/plain", "File not existing!");
    return;
  }
  File imageFile = SPIFFS.open(path, FILE_READ);
  // uint32_t buf_len = imageFile.size();
  // char imageBuffer[buf_len];
  // imageFile.readBytes(imageBuffer, buf_len);
  // server.send_P(200, "image/jpeg", imageBuffer, buf_len);
  server.streamFile(imageFile, "image/jpeg");
  imageFile.close();
}
void serveSpace() {;
  server.send(200, "text/plain", String(getFreeFlash()));
}
void serveWipe() {
  String res = SPIFFS.format()? "Wipe successful!" : "Wipe FAILED";
  server.send(200, "text/plain", res);
}
void serveDelete() {
  if(!server.hasArg("id")) {
    server.send(400, "text/plain", "id parameter missing!");
    return;
  }
  String path = getImgPath(server.arg("id"));
  if(SPIFFS.remove(path)) {
    server.send(200, "text/plain", "OK");
  } else {
    server.send(404, "text/plain", "File not existing!");
  }
}

void serveImgCount() {
  uint16_t count = getNextImgId();
  server.send(200, "text/plain", String(count));
}

size_t getFreeFlash() {
  Serial.println(SPIFFS.totalBytes());
  Serial.println(SPIFFS.usedBytes());
  return SPIFFS.totalBytes() - SPIFFS.usedBytes() - 1000;
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  Serial.println("Received on: " + String(topic));
  uint16_t incoming = atoi((char*)payload);
  // Atoi returns 0 on error, so no problem
  if (incoming > maxScore) {
    maxScore = incoming;
    Serial.printf("Increased max to %d\n", incoming);
  }
  
}

void connectMQTT() {
  mqttClient.setServer(mqttServer, mqttPort);
  while (!mqttClient.connected()) {
    mqttClient.connect(CUP_ID);
    Serial.print(".");
    delay(500);
  }
  Serial.println("MQTT connected"); 
  mqttClient.setCallback(mqttCallback);
  mqttClient.subscribe("cup/+/imageCount");
}

void setup() { 
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  led.init();
  led.startup(2, 100);

  // Spiffs: format on fail = true
  if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
  }  
 
  // Camera init
  initCamera();

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  WiFi.onEvent(lostConnection, WiFiEvent_t::SYSTEM_EVENT_ETH_DISCONNECTED);

  // MQTT connection
  connectMQTT();

  // REST API Server
  server.on("/", serveWelcome);
  server.on("/pic", serveImage);
  server.on("/storage/img", serveOldImg);
  server.on("/storage/space", serveSpace);
  server.on("/storage/img_count", serveImgCount);
  server.on("/storage/wipe", serveWipe);
  server.on("/storage/delete", serveDelete);
  server.on("/flash/on", serveFlashOn);
  server.on("/flash/off", serveFlashOff);
  server.begin();

  Serial.print("Server Ready! Go to: http://");
  Serial.println(WiFi.localIP());

  // Setup Flash
  pinMode(FLASH_PIN, OUTPUT);

  // Setup Button
  pinMode(BTN_PIN, INPUT_PULLUP);
}

void loop() {
  server.handleClient();

  mqttClient.loop();
  if (!mqttClient.connected()) {
    Serial.println("Lost MQTT connection... trying reconnect");
    connectMQTT();
  }

  if (digitalRead(BTN_PIN) == LOW) {
    // digitalWrite(FLASH_PIN, 1);
    publishScore();
    led.showCapture();
    capture();
    // digitalWrite(FLASH_PIN, 0);

    saveCurrentImage();
    clearFb();
  }
  
  led.ranking();
  delay(1000);
}

