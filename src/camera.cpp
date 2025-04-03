#include "camera.hpp"

Camera::Camera()
    : BaseModule("CAMERA")
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector

  this->config.ledc_channel = LEDC_CHANNEL_0;
  this->config.ledc_timer = LEDC_TIMER_0;
  this->config.pin_d0 = Y2_GPIO_NUM;
  this->config.pin_d1 = Y3_GPIO_NUM;
  this->config.pin_d2 = Y4_GPIO_NUM;
  this->config.pin_d3 = Y5_GPIO_NUM;
  this->config.pin_d4 = Y6_GPIO_NUM;
  this->config.pin_d5 = Y7_GPIO_NUM;
  this->config.pin_d6 = Y8_GPIO_NUM;
  this->config.pin_d7 = Y9_GPIO_NUM;
  this->config.pin_xclk = XCLK_GPIO_NUM;
  this->config.pin_pclk = PCLK_GPIO_NUM;
  this->config.pin_vsync = VSYNC_GPIO_NUM;
  this->config.pin_href = HREF_GPIO_NUM;
  this->config.pin_sccb_sda = SIOD_GPIO_NUM;
  this->config.pin_sccb_scl = SIOC_GPIO_NUM;
  this->config.pin_pwdn = PWDN_GPIO_NUM;
  this->config.pin_reset = RESET_GPIO_NUM;
  this->config.xclk_freq_hz = 24000000;
  this->config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    this->config.frame_size = FRAMESIZE_UXGA;
    this->config.jpeg_quality = 10;
    this->config.fb_count = 2;
  }
  else
  {
    this->config.frame_size = FRAMESIZE_SVGA;
    this->config.jpeg_quality = 12;
    this->config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    ESP_LOGE(this->NAME, "Camera init failed with error 0x%x", err);
    return;
  }
}