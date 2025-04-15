#pragma once
#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

// #include <Arduino.h>
#include "base_module.hpp"
#include "camera.hpp"
#include "classification.hpp"
#include "web_server.hpp"

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

    Camera *camera;
    Classification *classifier;
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
