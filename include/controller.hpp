#pragma once
#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "base_module.hpp"
#include "camera.hpp"
#include "web_server.hpp"
#include "communicate.hpp"

enum RobotState
{
    INIT = 100,
    SETUP,
    READY,
    START,
    WAITING,
    CLASSIFY,
    RESPONSE,
    IDLE
};

class Controller : public BaseModule
{
private:
    RobotState state;

    Communicate *communicate;
    Camera *camera;
    RWebServer *webServer;

    void taskFn() override;

    bool init();
    bool setup();
    bool ready();
    bool start();
    bool waiting();
    bool classify();
    bool response();
    bool idle();

    void stateMachine();

public:
    Controller();
    ~Controller();

    bool setState(RobotState state, bool extCondition = true);
};

#endif
