#include "main.hpp"

#include <esp_camera.h>
#include <WebServer.h>
#include <Object-detection-ESP32_inferencing.h>
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

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
#define EI_CAMERA_FRAME_BYTE_SIZE 3

SemaphoreHandle_t fb_mutex;
uint8_t *shared_fb = nullptr;

class Camera
{
public:
    static camera_config_t config;
    static bool is_initialized;

    static void taskFn(void *params)
    {
        if (!init())
        {
            Serial.println("[Camera] Init failed");
            vTaskDelete(NULL);
        }
        shared_fb = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
        if (!shared_fb)
        {
            Serial.println("[Camera] Failed to allocate shared frame buffer");
            vTaskDelete(NULL);
        }

        while (true)
        {
            if (xSemaphoreTake(fb_mutex, portMAX_DELAY) == pdTRUE)
            {
                camera_fb_t *fb = esp_camera_fb_get();
                if (fb)
                {
                    fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, shared_fb);
                    esp_camera_fb_return(fb);
                }
                xSemaphoreGive(fb_mutex);
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }

    static bool init()
    {
        if (is_initialized)
            return true;
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK)
            return false;
        is_initialized = true;
        return true;
    }
};

camera_config_t Camera::config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY};
bool Camera::is_initialized = false;

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf)
{
    bool do_resize = false;

    if (!Camera::is_initialized)
    {
        ei_printf("ERR: Camera is not initialized\r\n");
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        ei_printf("Camera capture failed\n");
        return false;
    }

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);

    esp_camera_fb_return(fb);

    if (!converted)
    {
        ei_printf("Conversion failed\n");
        return false;
    }

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

    return true;
}

class Classification
{
public:
    static void taskFn(void *params)
    {
        ei::signal_t signal;
        signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
        signal.get_data = &getData;

        while (true)
        {
            uint8_t *local_buf = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
            if (!local_buf)
            {
                vTaskDelay(100 / portTICK_PERIOD_MS);
                continue;
            }

            if (xSemaphoreTake(fb_mutex, portMAX_DELAY) == pdTRUE)
            {
                memcpy(local_buf, shared_fb, EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
                xSemaphoreGive(fb_mutex);

                ei_camera_capture(EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, local_buf);

                ei_impulse_result_t result = {};
                EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
                if (err == EI_IMPULSE_OK)
                {
                    for (uint32_t i = 0; i < result.bounding_boxes_count; i++)
                    {
                        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
                        if (bb.value == 0)
                            continue;
                        ei_printf("[Classify] %s (%f) [x:%d y:%d w:%d h:%d]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
                    }
                }
            }
            free(local_buf);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }

    static int getData(size_t offset, size_t length, float *out_ptr)
    {
        size_t pixel_ix = offset * 3;
        size_t out_ix = 0;
        while (length--)
        {
            out_ptr[out_ix++] = (shared_fb[pixel_ix + 2] << 16) + (shared_fb[pixel_ix + 1] << 8) + shared_fb[pixel_ix];
            pixel_ix += 3;
        }
        return 0;
    }
};

class RWebServer
{
public:
    static WebServer server;
    static WiFiClient lastClient;

    static void taskFn(void *params)
    {
        server.on("/stream", HTTP_GET, []()
                  {
      WiFiClient client = server.client();
      if (!client)
        return;

      if (lastClient.connected())
        lastClient.stop();

      lastClient = client;

      String response = "HTTP/1.1 200 OK\r\n";
      response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
      client.print(response);

      while (client.connected())
      {
        uint8_t *local_buf = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
        if (local_buf && xSemaphoreTake(fb_mutex, portMAX_DELAY) == pdTRUE)
        {
          memcpy(local_buf, shared_fb, EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
          xSemaphoreGive(fb_mutex);

          camera_fb_t fake_fb;
          fake_fb.buf = local_buf;
          fake_fb.len = EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE;
          fake_fb.width = EI_CAMERA_RAW_FRAME_BUFFER_COLS;
          fake_fb.height = EI_CAMERA_RAW_FRAME_BUFFER_ROWS;
          fake_fb.format = PIXFORMAT_RGB888;

          client.print("--frame\r\n");
          client.print("Content-Type: image/jpeg\r\n\r\n");
          client.write(fake_fb.buf, fake_fb.len);
          client.print("\r\n");
        }
        free(local_buf);
        delay(100);
      }
      client.stop(); });
        server.begin();
        while (true)
        {
            server.handleClient();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
};

WebServer RWebServer::server(80);
WiFiClient RWebServer::lastClient;

const char *TAG = "SET_UP";
IPAddress localIP(192, 168, 2, 200);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);

    while (!Serial)
    {
        ESP_LOGE(TAG, "Serial is not found!\n");
        delay(10);
    }

    Serial.setDebugOutput(true);

    esp_log_level_set("*", ESP_LOG_VERBOSE);
    ESP_LOGD("EXAMPLE", "This doesn't show");

    log_v("Verbose");
    log_d("Debug");
    log_i("Info");
    log_w("Warning");
    log_e("Error");

    ESP_LOGI(TAG, "Initializing...\n");

    if (psramFound())
    {
        ESP_LOGI(TAG, "PSRAM is enabled and detected!");
        ESP_LOGI(TAG, "Free PSRAM: %d bytes", ESP.getFreePsram());
    }
    else
    {
        ESP_LOGE(TAG, "PSRAM NOT FOUND! Check board config or hardware.");
    }

    WifiUtil::initWifi(
        WIFI_SSID,
        WIFI_PASSWORD,
        true,
        localIP,
        gateway,
        subnet,
        primaryDNS,
        secondaryDNS);

    fb_mutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(Camera::taskFn, "Camera", 8192, NULL, 1, NULL, 1);
    delay(1000); // Delay to ensure Camera task is running first
    xTaskCreatePinnedToCore(Classification::taskFn, "Classification", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(RWebServer::taskFn, "RWebServer", 8192, NULL, 1, NULL, 1);
}

void loop() {}
