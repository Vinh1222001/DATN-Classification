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

    if (!WifiUtil::initWifi(WIFI_SSID, WIFI_PASSWORD, true))
    {
        ESP_LOGE(WifiUtil::TAG, "Can't connect to wifif");
        while (true)
        {
            delay(1000);
        }
    }

    Communicate *communicate = new Communicate();

    if (communicate->begin())
    {
        ESP_LOGI("ESP32 B", "ESP-NOW Init   ialized Successfully");
        std::vector<String> stringArray = {"Xin chào", "ESP", "NOW"};
        // Gửi message đến ESP32 B
        communicate->send(stringArray);
    }
    else
    {
        ESP_LOGE("ESP32 B", "Failed to initialize ESP-NOW");
    }
}

void loop() {}