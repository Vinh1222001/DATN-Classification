#include <Arduino.h>
#include "main.hpp"

void setup()
{
    ESP_LOGI("SET UP", "Set up Serial...");
    Serial.begin(CONFIG_MONITOR_BAUD);
    while (!Serial)
    {
        ESP_LOGE("SET UP", "Serial is not found!\n");
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

    ESP_LOGI("SET UP", "Initializing...\n");

    Communicate *communicate = new Communicate();

    const uint8_t peerMac[6] = {0x48, 0xe7, 0x29, 0x99, 0x32, 0x04};
    if (communicate->begin(peerMac))
    {
        ESP_LOGI("ESP32 B", "ESP-NOW Init   ialized Successfully");
        Message msg = {1, 3.14f};

        // Gửi message đến ESP32 B
        communicate->send(reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
    }
    else
    {
        ESP_LOGE("ESP32 B", "Failed to initialize ESP-NOW");
    }
}

void loop() {}