#pragma once
#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <Arduino.h>
#include "base_module.hpp"
#include "esp_camera.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"

#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#if defined(CAMERA_MODEL_ESP_EYE)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 4
#define SIOD_GPIO_NUM 18
#define SIOC_GPIO_NUM 23

#define Y9_GPIO_NUM 36
#define Y8_GPIO_NUM 37
#define Y7_GPIO_NUM 38
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 35
#define Y4_GPIO_NUM 14
#define Y3_GPIO_NUM 13
#define Y2_GPIO_NUM 34
#define VSYNC_GPIO_NUM 5
#define HREF_GPIO_NUM 27
#define PCLK_GPIO_NUM 25

#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#else
#error "Camera model not selected"
#endif

// Các định nghĩa kích thước và frame byte (được sử dụng trong Edge Impulse)
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3

class Camera : public BaseModule
{
public:
  Camera();
  ~Camera();

  // Khởi tạo camera
  bool init();

  // Giải phóng camera
  void deinit();

  // Capture ảnh, rescale và crop nếu cần.
  // out_buf phải được cấp phát đủ kích thước từ bên ngoài.
  bool capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);

  // Hàm lấy dữ liệu ảnh đã capture để xử lý (ví dụ cho inferencing)
  // Hàm này chuyển đổi dữ liệu từ buffer (RGB888) sang một chuỗi số float theo thứ tự R,G,B
  int get_data(size_t offset, size_t length, float *out_ptr);

  void taskFn() override;

private:
  bool is_initialised;
  camera_config_t config;
  // Chúng ta sẽ lưu trữ pointer của buffer ảnh đã capture trong biến thành viên snapshot_buf
  // Lưu ý: Buffer này sẽ được truyền từ bên ngoài trong hàm capture.
  uint8_t *snapshot_buf;
};

#endif // CAMERA_HPP
