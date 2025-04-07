#pragma once
#ifndef COMMUNICATE_HPP
#define COMMUNICATE_HPP

#pragma once
#include "base_module.hpp"
#include <esp_now.h>
#include <WiFi.h>
#include "esp_log.h"

struct Message
{
    int id;
    float value;
};

class Communicate : public BaseModule
{
public:
    Communicate();
    ~Communicate();
    bool begin(const uint8_t *peerAddress);
    bool send(const uint8_t *data, size_t len);

private:
    static Communicate *instance; // Static pointer to the current instance
    uint8_t peerMac[6];

    // Static callbacks
    static void onDataSentStatic(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecvStatic(const uint8_t *mac, const uint8_t *incomingData, int len);

    // Instance methods to call from static
    void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

    void taskFn() override;
};

#endif
