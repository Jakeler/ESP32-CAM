#include <Arduino.h>

#include <WiFi.h>
#include "WebServer.h"
WebServer server(80);

#include "constants.h"
#include "pins.h"

#include "led.h"
LED led;
#include "camera.h"
CAM cam;
#include "mqtt.h"
MQTT mqtt;


void lostConnection(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WIFI disconnected!");
  led.pulse(0, 255, 3, true);
}



void serveWelcome() {
  server.send(200, "text/plain", "ESP-CAM running, please use an endpoint in /storage or /flash");
}

void serveFlashOn() {
  digitalWrite(FLASH_PIN, 1);
  server.send(200, "text/plain", "OK");
}
void serveFlashOff() {
  digitalWrite(FLASH_PIN, 0);
  server.send(200, "text/plain", "OK");
}

void serveImg() {
  if(!server.hasArg("id")) {
    server.send(400, "text/plain", "id parameter missing!");
    return;
  }

  String id = server.arg("id");
  if(!cam.checkImgExists(id)) {
    server.send(404, "text/plain", "File not existing!");
    return;
  }
  File imageFile = cam.getImgFile(id);
  server.streamFile(imageFile, "image/jpeg");
  imageFile.close();
}
void serveSpace() {;
  server.send(200, "text/plain", String(cam.getFreeFlash()));
}
void serveWipe() {
  String res = cam.wipeFS()? "Wipe successful!" : "Wipe FAILED";
  server.send(200, "text/plain", res);
}
void serveDelete() {
  if(!server.hasArg("id")) {
    server.send(400, "text/plain", "id parameter missing!");
    return;
  }
  
  if(!cam.deleteImg(server.arg("id"))) {
    server.send(200, "text/plain", "OK");
  } else {
    server.send(404, "text/plain", "Could not delete file");
  }
}

void serveImgCount() {
  uint16_t count = cam.getNextImgId();
  server.send(200, "text/plain", String(count));
}


void setup() { 
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  led.init();
  led.startup(2, 100);

  cam.initFS();
 
  cam.init();

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  WiFi.onEvent(lostConnection, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

  // MQTT connection
  mqtt.connect();

  // REST API Server
  server.on("/", serveWelcome);
  server.on("/storage/img", serveImg);
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

  mqtt.handle();


  if (digitalRead(BTN_PIN) == LOW) {
    // digitalWrite(FLASH_PIN, 1);
    mqtt.publishScore();
    led.showCapture();
    cam.capture();
    // digitalWrite(FLASH_PIN, 0);

    cam.saveCurrentImage();
    cam.clearFb();
  }
  
  led.ranking();
  delay(1000);
}

