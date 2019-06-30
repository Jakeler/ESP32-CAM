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

#include "main_server.h"

QueueHandle_t ledEventQueue;

void setup() { 
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  led.init();

  ledEventQueue = xQueueCreate(8, sizeof(int));
  sendEvent(LED_Event::startup);
  xTaskCreatePinnedToCore(ledTask, "LED Task", 16000, NULL, 17, NULL, 1);

  cam.initFS();
  cam.init();

  initWifi();
  mqtt.connect();
  initServer();
  sendEvent(LED_Event::win);

  // Setup Flash
  pinMode(FLASH_PIN, OUTPUT);
  // Setup Button
  pinMode(BTN_PIN, INPUT_PULLUP);
}

void loop() {
  server.handleClient();

  mqtt.handle();


  if (digitalRead(BTN_PIN) == LOW) {
    digitalWrite(FLASH_PIN, 1);

    sendEvent(LED_Event::capture);
    mqtt.publishScore();
    cam.capture();

    digitalWrite(FLASH_PIN, 0);

    cam.saveCurrentImage();
    cam.clearFb();
  }

  delay(100);
}

void sendEvent(LED_Event evt) {
  xQueueSend(ledEventQueue, &evt, 0);
}

void ledTask(void *params) {
  LED_Event event;
  while(1) {
    // Serial.printf("Running LEDs on core %d\n", xPortGetCoreID());
    if(xQueueReceive(ledEventQueue, &event, 0) != pdPASS) {
      led.ranking((float)mqtt.currentScore/(float)mqtt.maxScore);
      delay(100);
      continue; // Skip loop iteration is no new event
    }

    switch (event) {
      case LED_Event::startup:
        led.startup();
        break;
      case LED_Event::error:
        led.pulse(0, 255, 3, true);
        break;
      case LED_Event::capture:
        led.showCapture();
        break;
      case LED_Event::win:
        led.showWin();
        break;
      
      default:
        delay(100);
        break;
    }    
  }
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

/**
 * Connect to specified AP
 */
void initWifi() {
  Serial.print("Starting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

/**
 * REST API Server
 */
void initServer() {
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
}