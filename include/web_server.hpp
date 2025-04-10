#pragma once
#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include <WiFi.h>
#include "base_module.hpp"
#include "esp_http_server.h"
#include "camera.hpp"

// const char *ssid = "Tran Hung";
// const char *password = "66668888";

#define PART_BOUNDARY "123456789000000000000987654321"

class RWebServer : public BaseModule
{
private:
  const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
  const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
  const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  httpd_handle_t streamHttpd = NULL;

  esp_err_t streamHandler(httpd_req_t *req);

  static esp_err_t streamHandlerWrapper(httpd_req_t *req);

  bool connectWifi();

  void taskFn() override;

public:
  RWebServer();
  ~RWebServer();

  void startCameraServer();
};

#endif
