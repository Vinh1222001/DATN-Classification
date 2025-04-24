#include "controller.hpp"

Controller::Controller()
    : BaseModule(
          "CONTROLLER",
          CONTROLLER_TASK_PRIORITY,
          CONTROLLER_TASK_DELAY,
          CONTROLLER_TASK_STACK_DEPTH_LEVEL,
          CONTROLLER_TASK_PINNED_CORE_ID)
{
  this->state = RobotState::INIT;
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
  ESP_LOGI(this->NAME, "Current State: %d", this->state);
  switch (this->state)
  {
  case RobotState::INIT:
    this->init();
    break;

  case RobotState::SETUP:
    // IS_NULL(this->communicate);
    IS_NULL(this->camera);
    IS_NULL(this->webServer);

    this->setup();
    break;

  case RobotState::READY:
    IS_NULL(this->communicate);
    this->ready();
    break;

  case RobotState::START_SERVER:
    IS_NULL(this->webServer);
    this->startServer();
    break;

  case RobotState::START_CAMERA:
    IS_NULL(this->camera);
    this->startCamera();
    break;

  case RobotState::WAITING:
    // IS_NULL(this->communicate);
    IS_NULL(this->camera);
    this->waiting();
    break;

  case RobotState::CLASSIFY:
    IS_NULL(this->camera)
    this->classify();
    break;

  case RobotState::RESPONSE:
    IS_NULL(this->camera);
    IS_NULL(this->communicate)
    this->response();
    break;

  default:
    this->idle();
    break;
  }
}

bool Controller::init()
{
  ESP_LOGI(this->NAME, "Initialize components...");

  // uint8_t mac[6] = {0x48, 0xe7, 0x29, 0x99, 0x32, 0x04};
  // this->communicate = new Communicate(mac);
  // if (this->communicate == nullptr)
  // {
  //   ESP_LOGI(this->NAME, "Failed to create Communicate");
  //   return false;
  // }
  // ESP_LOGI(this->NAME, "Created Communicate");

  this->camera = new Camera();
  if (this->camera == nullptr || !this->camera->available())
  {
    ESP_LOGI(this->NAME, "Failed to create camera");
    return false;
  }
  ESP_LOGI(this->NAME, "Created camera");

  this->webServer = new RWebServer(camera);
  if (this->webServer == nullptr)
  {
    ESP_LOGI(this->NAME, "Failed to create Web Server");
    return false;
  }
  ESP_LOGI(this->NAME, "Created Web Server");

  ESP_LOGI(this->NAME, "Initialized all components...");

  this->setState(RobotState::SETUP);
  delay(2000);

  return true;
}

bool Controller::setup()
{
  ESP_LOGI(this->NAME, "Set up all components...");

  this->camera->stopClassifying();
  ESP_LOGI(this->NAME, "Creating Camera's task...");
  this->camera->createTask();
  delay(1000);

  ESP_LOGI(this->NAME, "Creating Web Server's task...");
  this->webServer->createTask();
  delay(1000);

  ESP_LOGI(this->NAME, "All component have been set up");
  // this->setState(RobotState::READY);
  this->setState(RobotState::START_CAMERA);
  delay(1000);
  return true;
}

bool Controller::ready()
{
  CommunicateResponse response = this->communicate->getResponse();

  if (response.header.compareTo("PING") == 0)
  {
    ESP_LOGI(
        this->NAME,
        "Connection is Ok! Received message: %s",
        response.content[0].c_str());

    std::vector<String> msg;
    msg.push_back("OK");
    this->communicate->send("RESPONSE", msg);
    this->setState(RobotState::START_SERVER);
    delay(1000);
    return true;
  }
  ESP_LOGE(
      this->NAME,
      "Connection Failed! Received message: %s",
      response.content[0].c_str());

  delay(1000);
  return false;
}

bool Controller::startServer()
{
  ESP_LOGI(this->NAME, "Running Web Server's task...");
  this->webServer->run();

  this->setState(RobotState::IDLE);
  delay(1000);
  return true;
}

bool Controller::startCamera()
{
  ESP_LOGI(this->NAME, "Running Camera's task...");
  this->camera->run();

  this->setState(RobotState::START_SERVER);
  delay(1000);

  return true;
}

bool Controller::waiting()
{
  // CommunicateResponse response = this->communicate->getResponse();

  // if (response.header.compareTo("PING") == 0)
  // {
  //   std::vector<String> data;
  //   data.push_back("OK");
  //   this->communicate->send("RESPONSE", data);
  //   ESP_LOGI(this->NAME, "Connected to Movement");
  //   return true;
  // }

  // if (response.header.compareTo("CLASSIFY") == 0)
  // {
  //   ESP_LOGI(this->NAME, "Starting to Classify...");
  //   this->camera->startClassifying();
  //   this->setState(RobotState::CLASSIFY);
  //   return true;
  // }

  this->camera->startClassifying();
  this->setState(RobotState::CLASSIFY);

  return false;
}

bool Controller::classify()
{
  // CommunicateResponse response = this->communicate->getResponse();
  // if (response.header.compareTo("STOP_CLASSIFY") == 0)
  // {
  //   ESP_LOGI(this->NAME, "Stop Classify, Return to Waiting state");
  //   this->camera->stopClassifying();
  //   this->setState(RobotState::WAITING);
  //   delay(1000);
  //   return true;
  // }

  if (!this->camera->getIsClassifying())
  {
    ESP_LOGI(this->NAME, "Stopped Classify, Switch to Response state");
    this->setState(RobotState::RESPONSE);
    delay(1000);
    return true;
  }
  ESP_LOGI(this->NAME, "Classifying...");
  return false;
}

bool Controller::response()
{
  // std::vector<String> conclude;
  String object = this->camera->getConclude();
  // conclude.push_back(object);
  ESP_LOGI(this->NAME, "Sending message with content: %s", object.c_str());
  // this->communicate->send("OBJECT", conclude);
  delay(2000);
  this->setState(RobotState::WAITING);
  delay(2000);
  return true;
}

bool Controller::idle()
{
  ESP_LOGI(this->NAME, "Idle...");
  delay(1000);
  return true;
}
