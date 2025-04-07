#include "communicate.hpp"

// ESP32 WROOM MAC: 48:e7:29:99:32:04

Communicate *Communicate::instance = nullptr;

Communicate::Communicate()
    : BaseModule("COMMUNICATE")
{
    instance = this;
}

Communicate::~Communicate() {}

bool Communicate::begin(const uint8_t *peerAddress)
{
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        ESP_LOGE(this->NAME, "ESP-NOW init failed");
        return false;
    }

    memcpy(peerMac, peerAddress, 6);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (!esp_now_is_peer_exist(peerMac))
    {
        if (esp_now_add_peer(&peerInfo) != ESP_OK)
        {
            ESP_LOGE(this->NAME, "Failed to add peer");
            return false;
        }
    }

    esp_now_register_send_cb(onDataSentStatic);
    esp_now_register_recv_cb(onDataRecvStatic);

    ESP_LOGI(this->NAME, "ESP-NOW init successful");
    return true;
}

bool Communicate::send(const uint8_t *data, size_t len)
{
    esp_err_t result = esp_now_send(peerMac, data, len);
    if (result == ESP_OK)
    {
        ESP_LOGI(this->NAME, "Sent data successfully");
        return true;
    }
    else
    {
        ESP_LOGE(this->NAME, "Failed to send data: %d", result);
        return false;
    }
}

void Communicate::onDataSentStatic(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (instance)
        instance->onDataSent(mac_addr, status);
}

void Communicate::onDataRecvStatic(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    if (instance)
        instance->onDataRecv(mac, incomingData, len);
}

void Communicate::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    ESP_LOGI(this->NAME, "Data sent to %02X:%02X:%02X:%02X:%02X:%02X - Status: %s",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5],
             status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void Communicate::onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    ESP_LOGI(this->NAME, "Received data from %02X:%02X:%02X:%02X:%02X:%02X, length: %d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len);
    // TODO: Handle incoming data here
    const Message *packet = reinterpret_cast<const Message *>(incomingData);
    ESP_LOGI(this->NAME, "Data Received: Id: %d, value: %.2f", packet->id, packet->value);

    Message response = {packet->id + 1, packet->value * 2.0f}; // Ví dụ phản hồi thay đổi giá trị
    send(reinterpret_cast<uint8_t *>(&response), sizeof(response));
}

void Communicate::taskFn() {}
