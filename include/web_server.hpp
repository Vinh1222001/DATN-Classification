#pragma once
#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "base_module.hpp"
#include "camera.hpp"
// #include "esp_http_server.h"
#include <WebServer.h>

class RWebServer : public BaseModule
{
private:
    Camera *camera = nullptr;
    WebServer *server = nullptr;

    void onStream();

    void taskFn() override;

public:
    RWebServer(Camera *cam);
    ~RWebServer();
};

#endif
