#include "camera.hpp"
#include <Object-detection-ESP32_inferencing.h>
// Constructor: Khởi tạo cấu hình camera theo model đã định nghĩa (ví dụ: AI THINKER)
Camera::Camera()
    : BaseModule(
          "CAMERA",
          CAMERA_TASK_PRIORITY,
          0,
          CAMERA_TASK_STACK_DEPTH_LEVEL,
          CAMERA_TASK_PINNED_CORE_ID),
      isInitialized(false),
      debugNn(false)
{
  // Thiết lập cấu hình camera dựa vào các macro định nghĩa từ file camera_pins.h
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;

  // Sử dụng 20MHz xclk, LEDC timer/chanel cho I2S
  config.xclk_freq_hz = 20000000;
  config.ledc_timer = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;

  // Sử dụng định dạng JPEG, độ phân giải QVGA (320x240)
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  this->snapshotBuffer = SetUtils::initMutexData<uint8_t *>(nullptr);
  this->isClassifying = SetUtils::initMutexData<bool>(false);

  if (psramFound())
  {
    ESP_LOGI(this->NAME, "PSRAM is enabled and detected!");
    ESP_LOGI(this->NAME, "Free PSRAM: %d bytes\n", ESP.getFreePsram());
  }
  else
  {
    ESP_LOGE(this->NAME, "PSRAM NOT FOUND! Check board config or hardware.");
  }

  while (!this->init())
  {
    ei_printf("Failed to initialize Camera!\r\n");
    ei_printf("Retry to initialize Camera...\n");
    delay(1000);
  }

  ei_printf("Camera initialized\r\n");
  this->isInitialized = true;

  ei_printf("\nStarting continious inference in 2 seconds...\n");
  ei_sleep(2000);
}

Camera::~Camera()
{
  if (isInitialized)
  {
    deinit();
  }
}

bool Camera::init()
{
  if (isInitialized)
    return true;

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  esp_err_t err = esp_camera_init(&this->config);
  if (err != ESP_OK)
  {
    ESP_LOGE(this->NAME, "Camera init failed with error 0x%x\n", err);
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  // Nếu cảm biến là OV3660, điều chỉnh một số thông số
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, 0);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#elif defined(CAMERA_MODEL_ESP_EYE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_awb_gain(s, 1);
#endif

  return true;
}

void Camera::deinit()
{
  esp_err_t err = esp_camera_deinit();
  if (err != ESP_OK)
  {
    ESP_LOGE(this->NAME, "Camera deinit failed");
    return;
  }
  SetUtils::setMutexData<boolean>(this->isClassifying, false);
  this->isInitialized = false;
}

bool Camera::capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf)
{
  ESP_LOGI(this->NAME, "Capture Processing...");
  bool doResize = false;
  if (!this->isInitialized)
  {
    ESP_LOGE(this->NAME, "ERR: Camera is not initialized");
    return false;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    ESP_LOGE(this->NAME, "Camera capture failed");
    return false;
  }

  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, out_buf);

  esp_camera_fb_return(fb);

  if (!converted)
  {
    ESP_LOGE(this->NAME, "Conversion failed");
    return false;
  }

  if ((img_width != CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != CAMERA_RAW_FRAME_BUFFER_ROWS))
  {
    doResize = true;
  }

  if (doResize)
  {
    ei::image::processing::crop_and_interpolate_rgb888(
        out_buf,
        CAMERA_RAW_FRAME_BUFFER_COLS,
        CAMERA_RAW_FRAME_BUFFER_ROWS,
        out_buf,
        img_width,
        img_height);
  }

  return true;
}

int Camera::getData(size_t offset, size_t length, float *out_ptr)
{
  if (this->snapshotBuffer.value == nullptr)
  {
    ESP_LOGE(this->NAME, "No snapshot buffer available");
    return -1;
  }
  // Mỗi pixel có 3 byte (RGB)
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;
  while (pixels_left != 0)
  {
    // Swap từ định dạng BGR sang RGB nếu cần, theo thông báo về bug của esp32-camera.
    out_ptr[out_ptr_ix] = (this->snapshotBuffer.value[pixel_ix + 2] << 16) + (this->snapshotBuffer.value[pixel_ix + 1] << 8) + this->snapshotBuffer.value[pixel_ix];
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  return 0;
}

void Camera::taskFn()
{
  if (ei_sleep(CAMERA_TASK_DELAY) != EI_IMPULSE_OK)
  {
    return;
  }

  if (!this->getIsClassifying())
  {
    ESP_LOGI(this->NAME, "Don't need to Classify");
    delay(3000);
    return;
  }

  if (xSemaphoreTake(this->snapshotBuffer.xMutex, portMAX_DELAY) == pdTRUE)
  {
    this->snapshotBuffer.value = (uint8_t *)malloc(CAMERA_RAW_FRAME_BUFFER_COLS * CAMERA_RAW_FRAME_BUFFER_ROWS * CAMERA_FRAME_BYTE_SIZE);
    if (this->snapshotBuffer.value == nullptr)
    {
      ESP_LOGE(this->NAME, "Allocation failed");
      free(this->snapshotBuffer.value);
      xSemaphoreGive(this->snapshotBuffer.xMutex);
      return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = [&](size_t offset, size_t length, float *out_ptr)
    {
      return this->getData(offset, length, out_ptr);
    };

    if (!this->capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, this->snapshotBuffer.value))
    {
      ESP_LOGE(this->NAME, "Failed to capture image");
      free(this->snapshotBuffer.value);
      xSemaphoreGive(this->snapshotBuffer.xMutex);
      return;
    }

    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, this->debugNn);
    if (err != EI_IMPULSE_OK)
    {
      ei_printf("ERR: Failed to run classifier (%d)\n", err);
      free(this->snapshotBuffer.value);
      xSemaphoreGive(this->snapshotBuffer.xMutex);
      return;
    }

    ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
              result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    if (result.bounding_boxes_count < 1)
    {
      free(this->snapshotBuffer.value);
      xSemaphoreGive(this->snapshotBuffer.xMutex);
      return;
    }
    ei_printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++)
    {
      ObjectData bb = result.bounding_boxes[i];

      if (bb.value == 0)
        continue;
      ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
                bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
      if (strcmp(bb.label, "hiep_si") == 0)
      {
        if (bb.value < 0.6)
        {
          bb.value = bb.value + (float)HIEP_SI_OBJECT_DRIFF;
        }
      }

      if (strcmp(bb.label, "vit_vang") == 0)
      {
        if (bb.value < 0.6)
        {
          bb.value = bb.value - (float)VIT_VANG_OBJECT_DRIFF;
        }
      }

      if (strcmp(bb.label, "jerry") == 0)
      {
        if (bb.value < 0.6)
        {
          bb.value = bb.value + (float)JERRY_OBJECT_DRIFF;
        }
      }

      this->samples.push_back(bb);
    }
#else
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
    {
      ei_printf("  %s: %.5f\r\n", ei_classifier_inferencing_categories[i], result.classification[i].value);
    }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY
    ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
#endif

    free(this->snapshotBuffer.value);

    xSemaphoreGive(this->snapshotBuffer.xMutex);

    if (this->samples.size() > OBJECT_SAMPLE_COUNTS)
    {
      this->stopClassifying();
    }
  }
}

bool Camera::getJpg(uint8_t **jpgBuf, size_t *jpgLen)
{
  bool success = false;

  // if (xSemaphoreTake(this->snapshotBuffer.xMutex, portMAX_DELAY) == pdTRUE)
  // {
  //   if (this->snapshotBuffer.value)
  //   {
  //     // giả sử snapshot_fb là frame RGB565 hoặc GRAYSCALE (không phải JPEG)
  //     camera_fb_t fake_fb = {
  //         .buf = this->snapshotBuffer.value,
  //         .len = snapshotLen,
  //         .width = width,
  //         .height = height,
  //         .format = format};

  //     success = frame2jpg(&fake_fb, 80, jpgBuf, jpgLen);
  //   }
  //   xSemaphoreGive(this->snapshotBuffer.xMutex);
  // }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    return false;
  }
  success = frame2jpg(fb, 80, jpgBuf, jpgLen);
  esp_camera_fb_return(fb);

  return success;
}

bool Camera::available()
{
  return this->isInitialized;
}

bool Camera::getIsClassifying()
{
  bool status = false;
  GetUtils::getMutexData(
      this->isClassifying,
      [&](bool value)
      {
        status = value;
      });
  return status;
}

void Camera::startClassifying()
{
  SetUtils::setMutexData(this->isClassifying, true);
}

void Camera::stopClassifying()
{
  SetUtils::setMutexData(this->isClassifying, false);
}

String Camera::getConclude()
{
  std::vector<ObjectData> uniqueObjects = Camera::getUniqueObjects(this->samples);

  ObjectData conclude;
  for (uint32_t i = 0; i < uniqueObjects.size(); i++)
  {
    ESP_LOGI(this->NAME, "Object: %s (%.3f)", uniqueObjects[i].label, uniqueObjects[i].value);
    if (i <= 0)
    {
      conclude = uniqueObjects[i];
    }
    else
    {
      if (conclude.value < uniqueObjects[i].value)
      {
        conclude = uniqueObjects[i];
      }
    }
    ESP_LOGI(this->NAME, "Conclude: %s (%.3f)", conclude.label, conclude.value);
  }

  if (conclude.value > 0.5)
  {
    char buffer[100];
    sprintf(
        buffer,
        "%s,%.2f,%u,%u,%u,%u",
        conclude.label,
        conclude.value,
        conclude.x,
        conclude.y,
        conclude.width,
        conclude.height);
    return String(buffer);
  }

  return String("NOT_FOUND");
}

std::vector<ObjectData> Camera::getUniqueObjects(
    const std::vector<ObjectData> &samples)
{
  struct Accumulator
  {
    ObjectData box;
    float totalValue;
    int count;
  };

  std::map<std::string, Accumulator> labelToAccumulator;

  for (const auto &box : samples)
  {
    std::string labelStr = std::string(box.label);

    if (labelToAccumulator.find(labelStr) == labelToAccumulator.end())
    {
      labelToAccumulator[labelStr] = {
          .box = box,
          .totalValue = box.value,
          .count = 1};
    }
    else
    {
      labelToAccumulator[labelStr].totalValue += box.value;
      labelToAccumulator[labelStr].count += 1;
    }
  }

  // Tạo vector kết quả với giá trị trung bình
  std::vector<ObjectData> uniqueObjects;

  for (auto &[label, acc] : labelToAccumulator)
  {
    ObjectData averagedBox = acc.box;
    averagedBox.value = acc.totalValue / acc.count;

    uniqueObjects.push_back(averagedBox);
  }

  return uniqueObjects;
}

void Camera::clearSamples()
{
  this->samples.clear();
}
