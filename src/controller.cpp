#include "controller.hpp"

Controller::Controller()
    : BaseModule("CONTROLLER")
{
  this->state = INIT;
}

Controller::~Controller() {}

bool Controller::setState(RobotState state, bool extCondition)
{
  if (extCondition)
  {
    this->state = state;
    return true;
  }
  return false;
}

void Controller::taskFn()
{
  this->stateMachine();
}

void Controller::stateMachine()
{
  switch (this->state)
  {
  case INIT:
    this->init();
    break;

  case SETUP:
    this->setup();
    break;

  case READY:
    this->ready();
    break;

  case START:
    this->start();
    break;

  case WAITING:
    this->waiting();
    break;

  case CLASSIFY:
    this->classify();
    break;

  case RESPONSE:
    this->response();
    break;

  default:
    this->idle();
    break;
  }
}

bool Controller::init()
{
  return true;
}

bool Controller::setup()
{
  return true;
}

bool Controller::ready()
{
  return true;
}

bool Controller::start()
{
  return true;
}

bool Controller::waiting()
{
  return true;
}

bool Controller::classify()
{
  return true;
}

bool Controller::response()
{
  return true;
}

bool Controller::idle()
{
  return true;
}
