#include "web_server.hpp"

RWebServer::RWebServer()
    : BaseModule("RWEB_SERVER")
{
}

RWebServer::~RWebServer()
{
  if (this->streamHttpd)
  {
    httpd_stop(this->streamHttpd);
    this->streamHttpd = NULL;
  }
}

bool RWebServer::connectWifi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    ESP_LOGI(this->NAME, ".");
  }
  ESP_LOGI(this->NAME, "WiFi connected");
  ESP_LOGI(this->NAME, "Camera Stream Ready! Go to: http://%s", WiFi.localIP().toString().c_str());
  return true;
}

esp_err_t RWebServer::streamHandler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[64];

  res = httpd_resp_set_type(req, this->_STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
  {
    return res;
  }

  while (true)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      ESP_LOGE(this->NAME, "Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->width > 400)
      {
        if (fb->format != PIXFORMAT_JPEG)
        {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted)
          {
            ESP_LOGE(this->NAME, "JPEG compression failed");
            res = ESP_FAIL;
          }
        }
        else
        {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK)
    {
      size_t hlen = snprintf((char *)part_buf, 64, this->_STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, this->_STREAM_BOUNDARY, strlen(this->_STREAM_BOUNDARY));
    }
    if (fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    }
    else if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK)
    {
      break;
    }
    // Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

esp_err_t RWebServer::streamHandlerWrapper(httpd_req_t *req)
{
  return static_cast<RWebServer *>(req->user_ctx)->streamHandler(req);
}

void RWebServer::startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = RWebServer::streamHandlerWrapper,
      .user_ctx = NULL};

  if (httpd_start(&this->streamHttpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(this->streamHttpd, &index_uri);
  }
}