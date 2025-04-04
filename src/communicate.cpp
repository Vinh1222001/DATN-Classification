#include "communicate.hpp"

Communicate::Communicate()
    : BaseModule("COMMUNICATE")
{
}

Communicate::~Communicate() {}

void Communicate::taskFn()
{
    esp_err_t result = esp_now_send(this->_PARTNER_MAC, (uint8_t *)&this->_sendMessage, sizeof(this->_sendMessage));

    if (result == ESP_OK)
    {
        ESP_LOGI(this->NAME, "Sent with success");
    }
    else
    {
        ESP_LOGI(this->NAME, "Error sending the data");
    }
}
