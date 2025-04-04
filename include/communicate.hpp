#pragma once
#ifndef COMMUNICATE_HPP
#define COMMUNICATE_HPP

#include <Arduino.h>
#include <WiFi.h>
#include "esp_now.h"
#include "base_module.hpp"

struct Message
{
    char *content;
};

class Communicate : public BaseModule
{
private:
    const uint8_t _PARTNER_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    Message _receiveMessage;
    Message _sendMessage;

    void onSendData(const uint8_t *mac_addr, esp_now_send_status_t status);
    void onReceiveData(const uint8_t *mac, const uint8_t *incomingData, int len);

    void taskFn() override;

public:
    Communicate();
    ~Communicate();
};

#endif
