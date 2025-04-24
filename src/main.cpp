#include "main.hpp"

const char *TAG = "SET_UP";

IPAddress localIP(192, 168, 2, 200); // <--- Your desired static IP
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    ESP_LOGI(TAG, "Set up Serial...");
    Serial.begin(CONFIG_MONITOR_BAUD);
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

    while (!WifiUtil::initWifi(
        WIFI_SSID,
        WIFI_PASSWORD,
        true,
        localIP,
        gateway,
        subnet,
        primaryDNS,
        secondaryDNS))
    {
        ESP_LOGE(TAG, "Retry to connect WIFI SSID: %s, PASSWORD: %s", WIFI_SSID, WIFI_PASSWORD);
        delay(1000);
    }
    Controller *controller = new Controller();

    while (controller == nullptr)
    {
        ESP_LOGE(TAG, "Can't init Controller");
        controller = new Controller();
        delay(1000);
    }

    delay(2000);
    controller->createTask();
    delay(2000);
    controller->run();
}

void loop() {}
