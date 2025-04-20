#pragma once
#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "edge-impulse-sdk/dsp/image/image.hpp"

// #include <Arduino.h>
#include "base_module.hpp"
#include "types.hpp"
#include "esp_camera.h"
#include "communicate.hpp"
#include <vector>
#include <map>

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
#define CAMERA_RAW_FRAME_BUFFER_COLS 320
#define CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define CAMERA_FRAME_BYTE_SIZE 3

#define OBJECT_SAMPLE_COUNTS 20

using SnapshotBuffer = Types::SemaphoreMutexData<uint8_t *>;
using ClassifyState = Types::SemaphoreMutexData<bool>;

class Camera : public BaseModule
{
public:
  Camera(Communicate *communicate = nullptr);
  ~Camera();

  int getData(size_t offset, size_t length, float *out_ptr);
  bool getJpg(uint8_t **jpgBuf, size_t *jpgLen);

  bool available();

  bool getIsClassifying();
  void startClassifying();
  void stopClassifying();

  String getConclude();

  static std::vector<ei_impulse_result_bounding_box_t> getUniqueObjectsWithMaxValue(
      const std::vector<ei_impulse_result_bounding_box_t> &samples);

private:
  bool isInitialized;
  bool debugNn;
  camera_config_t config;
  SnapshotBuffer snapshotBuffer;
  Communicate *communicate;

  ClassifyState isClassifying;
  std::vector<ei_impulse_result_bounding_box_t> samples;

  bool init();
  void deinit();
  void taskFn() override;

  bool capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
};

#endif
