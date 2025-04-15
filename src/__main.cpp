// #include <Object-detection-ESP32_inferencing.h>
// #include "edge-impulse-sdk/dsp/image/image.hpp"

// #include "esp_camera.h"
// #include "soc/soc.h"
// #include "soc/rtc_cntl_reg.h"

// #include <WebServer.h>
// #include <WiFi.h>

// #define CAMERA_MODEL_AI_THINKER // Has PSRAM

// #if defined(CAMERA_MODEL_ESP_EYE)
// #define PWDN_GPIO_NUM -1
// #define RESET_GPIO_NUM -1
// #define XCLK_GPIO_NUM 4
// #define SIOD_GPIO_NUM 18
// #define SIOC_GPIO_NUM 23

// #define Y9_GPIO_NUM 36
// #define Y8_GPIO_NUM 37
// #define Y7_GPIO_NUM 38
// #define Y6_GPIO_NUM 39
// #define Y5_GPIO_NUM 35
// #define Y4_GPIO_NUM 14
// #define Y3_GPIO_NUM 13
// #define Y2_GPIO_NUM 34
// #define VSYNC_GPIO_NUM 5
// #define HREF_GPIO_NUM 27
// #define PCLK_GPIO_NUM 25

// #elif defined(CAMERA_MODEL_AI_THINKER)
// #define PWDN_GPIO_NUM 32
// #define RESET_GPIO_NUM -1
// #define XCLK_GPIO_NUM 0
// #define SIOD_GPIO_NUM 26
// #define SIOC_GPIO_NUM 27

// #define Y9_GPIO_NUM 35
// #define Y8_GPIO_NUM 34
// #define Y7_GPIO_NUM 39
// #define Y6_GPIO_NUM 36
// #define Y5_GPIO_NUM 21
// #define Y4_GPIO_NUM 19
// #define Y3_GPIO_NUM 18
// #define Y2_GPIO_NUM 5
// #define VSYNC_GPIO_NUM 25
// #define HREF_GPIO_NUM 23
// #define PCLK_GPIO_NUM 22

// #else
// #error "Camera model not selected"
// #endif

// /* Constant defines -------------------------------------------------------- */
// #define EI_CAMERA_RAW_FRAME_BUFFER_COLS 320
// #define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 240
// #define EI_CAMERA_FRAME_BYTE_SIZE 3

// /* Private variables ------------------------------------------------------- */
// static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
// static bool is_initialised = false;
// uint8_t *snapshot_buf; // points to the output of the capture
// SemaphoreHandle_t fb_mutex;

// static camera_config_t camera_config = {
//     .pin_pwdn = PWDN_GPIO_NUM,
//     .pin_reset = RESET_GPIO_NUM,
//     .pin_xclk = XCLK_GPIO_NUM,
//     .pin_sscb_sda = SIOD_GPIO_NUM,
//     .pin_sscb_scl = SIOC_GPIO_NUM,

//     .pin_d7 = Y9_GPIO_NUM,
//     .pin_d6 = Y8_GPIO_NUM,
//     .pin_d5 = Y7_GPIO_NUM,
//     .pin_d4 = Y6_GPIO_NUM,
//     .pin_d3 = Y5_GPIO_NUM,
//     .pin_d2 = Y4_GPIO_NUM,
//     .pin_d1 = Y3_GPIO_NUM,
//     .pin_d0 = Y2_GPIO_NUM,
//     .pin_vsync = VSYNC_GPIO_NUM,
//     .pin_href = HREF_GPIO_NUM,
//     .pin_pclk = PCLK_GPIO_NUM,

//     // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
//     .xclk_freq_hz = 20000000,
//     .ledc_timer = LEDC_TIMER_0,
//     .ledc_channel = LEDC_CHANNEL_0,

//     .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
//     .frame_size = FRAMESIZE_QVGA,   // QQVGA-UXGA Do not use sizes above QVGA when not JPEG

//     .jpeg_quality = 12, // 0-63 lower number means higher quality
//     .fb_count = 1,      // if more than one, i2s runs in continuous mode. Use only with JPEG
//     .fb_location = CAMERA_FB_IN_PSRAM,
//     .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
// };

// /* Function definitions ------------------------------------------------------- */
// bool ei_camera_init(void);
// void ei_camera_deinit(void);
// bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);

// static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);

// const char *ssid = "Tran Hung";
// const char *password = "66668888";
// IPAddress local_IP(192, 168, 2, 200); // <--- Your desired static IP
// IPAddress gateway(192, 168, 2, 1);
// IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);   // optional
// IPAddress secondaryDNS(8, 8, 4, 4); // optional

// WebServer server(80);
// WiFiClient lastClient;

// void mjpeg_stream_task(void *pvParameters)
// {
//     WiFiClient client = *((WiFiClient *)pvParameters);

//     while (client.connected())
//     {
//         if (xSemaphoreTake(fb_mutex, portMAX_DELAY) == pdTRUE)
//         {
//             camera_fb_t *fb = esp_camera_fb_get();
//             if (!fb)
//             {
//                 xSemaphoreGive(fb_mutex);
//                 continue;
//             }

//             client.print("--frame\r\n");
//             client.print("Content-Type: image/jpeg\r\n\r\n");
//             client.write(fb->buf, fb->len);
//             client.print("\r\n");

//             esp_camera_fb_return(fb);
//             xSemaphoreGive(fb_mutex);
//         }

//         delay(100);
//     }

//     client.stop();
//     vTaskDelete(NULL);
// }

// void handle_jpg_stream()
// {
//     WiFiClient client = server.client();
//     if (!client)
//         return;

//     if (lastClient.connected())
//         lastClient.stop();
//     lastClient = client;

//     String response = "HTTP/1.1 200 OK\r\n";
//     response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
//     client.print(response);

//     // Create FreeRTOS task
//     xTaskCreatePinnedToCore(
//         mjpeg_stream_task,      // task function
//         "mjpeg_stream_task",    // name
//         8192,                   // stack size
//         new WiFiClient(client), // parameters
//         1,                      // priority
//         NULL,                   // task handle
//         1                       // core (use 1 to avoid interfering with WiFi)
//     );
// }

// /**
//  * @brief      Arduino setup function
//  */
// void setup()
// {
//     WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
//     // put your setup code here, to run once:
//     Serial.begin(115200);
//     // comment out the below line to start inference immediately after upload
//     while (!Serial)
//         ;

//     if (psramFound())
//     {
//         Serial.println("✅ PSRAM is enabled and detected!");
//         Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
//     }
//     else
//     {
//         Serial.println("❌ PSRAM NOT FOUND! Check board config or hardware.");
//     }

//     fb_mutex = xSemaphoreCreateMutex();

//     if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
//     {
//         Serial.println("STA Failed to configure");
//     }

//     WiFi.begin(ssid, password);
//     Serial.print("Connecting to WiFi");
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("\nWiFi connected");
//     Serial.println(WiFi.localIP());

//     // Start Webserver
//     server.on("/stream", HTTP_GET, handle_jpg_stream);
//     server.begin();
//     Serial.println("Web server started at /stream");

//     Serial.println("Edge Impulse Inferencing Demo");
//     if (ei_camera_init() == false)
//     {
//         ei_printf("Failed to initialize Camera!\r\n");
//     }
//     else
//     {
//         ei_printf("Camera initialized\r\n");
//     }

//     ei_printf("\nStarting continious inference in 2 seconds...\n");
//     ei_sleep(2000);
// }

// /**
//  * @brief      Get data and run inferencing
//  *
//  * @param[in]  debug  Get debug info if true
//  */
// void loop()
// {
//     server.handleClient(); // Handle incoming stream requests

//     if (WiFi.status() == WL_CONNECTED)
//     {
//         if (ei_sleep(5) != EI_IMPULSE_OK)
//         {
//             return;
//         }

//         // Try to take the camera mutex before doing inference
//         if (xSemaphoreTake(fb_mutex, (TickType_t)100) == pdTRUE)
//         {
//             snapshot_buf = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

//             if (snapshot_buf == nullptr)
//             {
//                 ei_printf("ERR: Failed to allocate snapshot buffer!\n");
//                 xSemaphoreGive(fb_mutex); // Don't forget to release mutex!
//                 return;
//             }

//             ei::signal_t signal;
//             signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
//             signal.get_data = &ei_camera_get_data;

//             if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false)
//             {
//                 ei_printf("Failed to capture image\r\n");
//                 free(snapshot_buf);
//                 xSemaphoreGive(fb_mutex);
//                 return;
//             }

//             // Run the classifier
//             ei_impulse_result_t result = {0};
//             EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
//             if (err != EI_IMPULSE_OK)
//             {
//                 ei_printf("ERR: Failed to run classifier (%d)\n", err);
//                 free(snapshot_buf);
//                 xSemaphoreGive(fb_mutex);
//                 return;
//             }

//             // Print results
//             ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
//                       result.timing.dsp, result.timing.classification, result.timing.anomaly);

// #if EI_CLASSIFIER_OBJECT_DETECTION == 1
//             for (uint32_t i = 0; i < result.bounding_boxes_count; i++)
//             {
//                 ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
//                 if (bb.value == 0)
//                     continue;
//                 ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
//                           bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
//             }
// #else
//             for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++)
//             {
//                 ei_printf("  %s: %.5f\r\n", ei_classifier_inferencing_categories[i], result.classification[i].value);
//             }
// #endif

// #if EI_CLASSIFIER_HAS_ANOMALY
//             ei_printf("Anomaly prediction: %.3f\r\n", result.anomaly);
// #endif

//             free(snapshot_buf);
//             xSemaphoreGive(fb_mutex); // Release camera after you're done
//         }
//     }
// }

// /**
//  * @brief   Setup image sensor & start streaming
//  *
//  * @retval  false if initialisation failed
//  */
// bool ei_camera_init(void)
// {

//     if (is_initialised)
//         return true;

// #if defined(CAMERA_MODEL_ESP_EYE)
//     pinMode(13, INPUT_PULLUP);
//     pinMode(14, INPUT_PULLUP);
// #endif

//     // initialize the camera
//     esp_err_t err = esp_camera_init(&camera_config);
//     if (err != ESP_OK)
//     {
//         Serial.printf("Camera init failed with error 0x%x\n", err);
//         return false;
//     }

//     sensor_t *s = esp_camera_sensor_get();
//     // initial sensors are flipped vertically and colors are a bit saturated
//     if (s->id.PID == OV3660_PID)
//     {
//         s->set_vflip(s, 1);      // flip it back
//         s->set_brightness(s, 1); // up the brightness just a bit
//         s->set_saturation(s, 0); // lower the saturation
//     }

// #if defined(CAMERA_MODEL_M5STACK_WIDE)
//     s->set_vflip(s, 1);
//     s->set_hmirror(s, 1);
// #elif defined(CAMERA_MODEL_ESP_EYE)
//     s->set_vflip(s, 1);
//     s->set_hmirror(s, 1);
//     s->set_awb_gain(s, 1);
// #endif

//     is_initialised = true;
//     return true;
// }

// /**
//  * @brief      Stop streaming of sensor data
//  */
// void ei_camera_deinit(void)
// {

//     // deinitialize the camera
//     esp_err_t err = esp_camera_deinit();

//     if (err != ESP_OK)
//     {
//         ei_printf("Camera deinit failed\n");
//         return;
//     }

//     is_initialised = false;
//     return;
// }

// /**
//  * @brief      Capture, rescale and crop image
//  *
//  * @param[in]  img_width     width of output image
//  * @param[in]  img_height    height of output image
//  * @param[in]  out_buf       pointer to store output image, NULL may be used
//  *                           if ei_camera_frame_buffer is to be used for capture and resize/cropping.
//  *
//  * @retval     false if not initialised, image captured, rescaled or cropped failed
//  *
//  */
// bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf)
// {
//     bool do_resize = false;

//     if (!is_initialised)
//     {
//         ei_printf("ERR: Camera is not initialized\r\n");
//         return false;
//     }

//     camera_fb_t *fb = esp_camera_fb_get();

//     if (!fb)
//     {
//         ei_printf("Camera capture failed\n");
//         return false;
//     }

//     bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, out_buf);

//     esp_camera_fb_return(fb);

//     if (!converted)
//     {
//         ei_printf("Conversion failed\n");
//         return false;
//     }

//     if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS))
//     {
//         do_resize = true;
//     }

//     if (do_resize)
//     {
//         ei::image::processing::crop_and_interpolate_rgb888(
//             out_buf,
//             EI_CAMERA_RAW_FRAME_BUFFER_COLS,
//             EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
//             out_buf,
//             img_width,
//             img_height);
//     }

//     return true;
// }

// static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
// {
//     // we already have a RGB888 buffer, so recalculate offset into pixel index
//     size_t pixel_ix = offset * 3;
//     size_t pixels_left = length;
//     size_t out_ptr_ix = 0;

//     while (pixels_left != 0)
//     {
//         // Swap BGR to RGB here
//         // due to https://github.com/espressif/esp32-camera/issues/379
//         out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix];

//         // go to the next pixel
//         out_ptr_ix++;
//         pixel_ix += 3;
//         pixels_left--;
//     }
//     // and done!
//     return 0;
// }

// #if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
// #error "Invalid model for current sensor"
// #endif
