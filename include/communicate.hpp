#pragma once
#ifndef COMMUNICATE_HPP
#define COMMUNICATE_HPP

#include "vector"
#include "base_module.hpp"
#include "types.hpp"
#include "utils/set.hpp"
#include <esp_now.h>
#include <WiFi.h>
#include "esp_log.h"

class Communicate : public BaseModule
{
public:
    Communicate();
    ~Communicate();
    bool begin();
    bool send(const std::vector<String> &data);

private:
    static Communicate *instance; // Static pointer to the current instance
    const uint8_t peerMac[6] = {0x48, 0xe7, 0x29, 0x99, 0x32, 0x04};

    // Static callbacks
    static void onDataSentStatic(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecvStatic(const uint8_t *mac, const uint8_t *incomingData, int len);

    // Instance methods to call from static
    void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

    void taskFn() override;
};

#endif
