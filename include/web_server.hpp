#pragma once
#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "base_module.hpp"
#include "camera.hpp"
#include <WebServer.h>

#define RWEB_SERVER_ON_STREAM_DELAY 20

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
