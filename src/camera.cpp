#include "camera.hpp"

// Constructor: Khởi tạo cấu hình camera theo model đã định nghĩa (ví dụ: AI THINKER)
Camera::Camera()
    : BaseModule("CAMERA"),
      is_initialised(false),
      snapshot_buf(nullptr)
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
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (this->init() == false)
  {
    ei_printf("Failed to initialize Camera!\r\n");
  }
  else
  {
    ei_printf("Camera initialized\r\n");
  }

  ei_printf("\nStarting continious inference in 2 seconds...\n");
  ei_sleep(2000);
}

Camera::~Camera()
{
  if (is_initialised)
  {
    deinit();
  }
  // Không cần free(snapshot_buf) vì nó được cấp phát bên ngoài (trong hàm capture)
}

bool Camera::init()
{
  if (is_initialised)
    return true;

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

  is_initialised = true;
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
  is_initialised = false;
}

// Capture ảnh từ camera, chuyển đổi về RGB888 (raw) và crop/rescale nếu cần.
// out_buf là buffer được cấp phát từ bên ngoài có kích thước đủ để chứa ảnh đã xử lý.
bool Camera::capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf)
{
  bool do_resize = false;
  if (!is_initialised)
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

  // Chuyển đổi ảnh JPEG sang định dạng RGB888 và lưu vào out_buf.
  // Nếu quá trình chuyển đổi thất bại, in lỗi.
  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, out_buf);
  // Trả lại frame buffer
  esp_camera_fb_return(fb);

  if (!converted)
  {
    ESP_LOGE(this->NAME, "Conversion failed");
    return false;
  }

  // Nếu kích thước ảnh đã capture khác với yêu cầu, cần crop và nội suy (resize)
  if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS))
  {
    do_resize = true;
  }

  if (do_resize)
  {
    ei::image::processing::crop_and_interpolate_rgb888(
        out_buf,
        EI_CAMERA_RAW_FRAME_BUFFER_COLS,
        EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
        out_buf,
        img_width,
        img_height);
  }

  // Lưu pointer buffer vào snapshot_buf để các hàm khác có thể truy cập dữ liệu ảnh đã capture.
  snapshot_buf = out_buf;

  return true;
}

// Hàm get_data: chuyển đổi dữ liệu RGB888 (đã lưu trong snapshot_buf) từ offset và length thành một mảng float.
// Giá trị trả về là số pixel được xử lý theo thứ tự (RGB) trong một số nguyên đại diện (hoặc dưới dạng float).
int Camera::get_data(size_t offset, size_t length, float *out_ptr)
{
  if (snapshot_buf == nullptr)
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
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  return 0;
}

void Camera::taskFn()
{

  if (ei_sleep(5) != EI_IMPULSE_OK)
  {
    return;
  }

  uint8_t *buffer = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
  if (buffer == nullptr)
  {
    ESP_LOGE(this->NAME, "Allocation failed");
    return;
  }

  if (this->capture(EI_CAMERA_RAW_FRAME_BUFFER_COLS, EI_CAMERA_RAW_FRAME_BUFFER_ROWS, buffer))
  {
    ESP_LOGI(this->NAME, "Capture succeeded");
    // Sau đó, bạn có thể dùng camera.get_data(offset, length, out_ptr) để lấy dữ liệu ảnh.
    // Ví dụ:
    size_t num_pixels = EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS;
    float *data = new float[num_pixels];
    this->get_data(0, num_pixels, data);
    // Xử lý data theo yêu cầu ...

    // Giải phóng bộ nhớ
    delete[] data;
  }
  free(buffer);
}