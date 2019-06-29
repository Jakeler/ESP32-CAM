#include <Arduino.h>

#include <SPIFFS.h>
#include <FS.h>

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"

#include "camera_config.h"

#include "camera.h"

camera_fb_t *fb = NULL;
uint8_t *_jpg_buf = NULL;
size_t _jpg_buf_len = 0;
esp_err_t res = ESP_OK;


void CAM::init() {
  Serial.println("Starting camera");
  while (esp_camera_init(&config) != ESP_OK) {
    Serial.print(".");
    delay(200);
  }
  Serial.println("Camera initialized");
}

void CAM::initFS() {
  Serial.println("Starting FS");
  // Spiffs: format on fail = true
  while(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      delay(1000);
  }  
}

void CAM::capture() {
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
void CAM::clearFb() {
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

uint16_t CAM::getNextImgId() {
  uint16_t id = 0;
  String path;
  do {
    path = getImgPath(String(id));
    id++;
  }  while(SPIFFS.exists(path));

  return id-1;
}

bool CAM::saveCurrentImage() {
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

String CAM::getImgPath(String id) {
  return "/img/" + id + ".jpg";
}

size_t CAM::getFreeFlash() {
  return SPIFFS.totalBytes() - SPIFFS.usedBytes() - 1000;
}

bool CAM::wipeFS() {
  return SPIFFS.format();
}

bool CAM::deleteImg(String id) {
  String path = getImgPath(id);
  return SPIFFS.remove(path);
}

bool CAM::checkImgExists(String id) {
  return SPIFFS.exists(getImgPath(id));
}

File CAM::getImgFile(String id) {
  return SPIFFS.open(getImgPath(id), FILE_READ);
}