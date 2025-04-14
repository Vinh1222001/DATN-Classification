#include "web_server.hpp"

RWebServer::RWebServer(Camera *cam)
    : BaseModule("RWEB_SERVER"),
      camera(cam)
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

esp_err_t RWebServer::streamHandler(httpd_req_t *req)
{
  esp_err_t res = ESP_OK;
  uint8_t *_jpg_buf = NULL;
  size_t _jpg_buf_len = 0;
  char part_buf[64]; // ⚠️ sửa lại từ char* part_buf[64] thành mảng thường

  res = httpd_resp_set_type(req, this->_STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
  {
    return res;
  }

  while (true)
  {
    // 👇 Gọi getDataJpg thay vì esp_camera_fb_get()
    if (!this->camera->getJpg(&_jpg_buf, &_jpg_buf_len))
    {
      ESP_LOGE(this->NAME, "Failed to get JPEG frame from camera buffer");
      res = ESP_FAIL;
    }

    if (res == ESP_OK)
    {
      size_t hlen = snprintf(part_buf, sizeof(part_buf), this->_STREAM_PART, _jpg_buf_len);
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

    if (_jpg_buf)
    {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    if (res != ESP_OK)
    {
      break;
    }
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

void RWebServer::taskFn() {}