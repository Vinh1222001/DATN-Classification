#include "controller.hpp"

Controller::Controller()
    : BaseModule(
          "CONTROLLER",
          CONTROLLER_TASK_PRIORITY,
          CONTROLLER_TASK_DELAY,
          CONTROLLER_TASK_STACK_DEPTH_LEVEL,
          CONTROLLER_TASK_PINNED_CORE_ID)
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

  case START_CAMERA:
    this->startCamera();
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
  ESP_LOGI(this->NAME, "Initialize components...");

  uint8_t mac[6] = {0x48, 0xe7, 0x29, 0x99, 0x32, 0x04};
  this->communicate = new Communicate(mac);
  if (this->communicate == nullptr)
  {
    ESP_LOGI(this->NAME, "Failed to create Communicate");
    return false;
  }
  ESP_LOGI(this->NAME, "Created Communicate");

  this->camera = new Camera(communicate);
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

  this->setState(SETUP);
  delay(2000);

  return true;
}

bool Controller::setup()
{
  ESP_LOGI(this->NAME, "Set up all components...");

  if (this->camera == nullptr)
  {
    ESP_LOGE(this->NAME, "Camera is not initialized");
    return false;
  }
  this->camera->stopClassifying();
  ESP_LOGI(this->NAME, "Creating Camera's task...");
  this->camera->createTask();
  delay(2000);

  if (this->webServer == nullptr)
  {
    ESP_LOGE(this->NAME, "Web Server is not initialized");
    return false;
  }
  ESP_LOGI(this->NAME, "Creating Web Server's task...");
  this->webServer->createTask();
  delay(2000);

  ESP_LOGI(this->NAME, "All component have been set up");
  this->setState(READY);
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
    this->setState(START_SERVER);
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
  if (this->webServer == nullptr)
  {
    ESP_LOGE(this->NAME, "Web Server is not initialized");
    return false;
  }
  ESP_LOGI(this->NAME, "Running Web Server's task...");
  this->webServer->run();

  this->setState(START_CAMERA);
  delay(1000);
  return true;
}

bool Controller::startCamera()
{
  if (this->camera == nullptr)
  {
    ESP_LOGE(this->NAME, "Camera is not initialized");
    return false;
  }
  ESP_LOGI(this->NAME, "Running Camera's task...");
  this->camera->run();

  this->setState(WAITING);
  delay(1000);

  return true;
}

bool Controller::waiting()
{
  CommunicateResponse response = this->communicate->getResponse();

  if (response.header.compareTo("PING") == 0)
  {
    std::vector<String> data;
    data.push_back("OK");
    this->communicate->send("RESPONSE", data);
    ESP_LOGI(this->NAME, "Connected to Movement");
    return true;
  }

  if (response.header.compareTo("CLASSIFY") == 0)
  {
    ESP_LOGI(this->NAME, "Starting to Classify...");
    this->camera->startClassifying();
    this->setState(CLASSIFY);
    return true;
  }

  return false;
}

bool Controller::classify()
{
  // CommunicateResponse response = this->communicate->getResponse();
  // if (response.header.compareTo("STOP_CLASSIFY") == 0)
  // {
  //   ESP_LOGI(this->NAME, "Stop Classify, Return to Waiting state");
  //   this->camera->stopClassifying();
  //   this->setState(WAITING);
  //   delay(1000);
  //   return true;
  // }

  if (!this->camera->getIsClassifying())
  {
    ESP_LOGI(this->NAME, "Stopped Classify, Switch to Response state");
    this->setState(RESPONSE);
    delay(1000);
    return true;
  }
  ESP_LOGI(this->NAME, "Classifying...");
  return false;
}

bool Controller::response()
{
  if (this->camera == nullptr || this->communicate == nullptr)
  {
    ESP_LOGE(this->NAME, "Camera or communicate is null");
  }

  std::vector<String> conclude;
  conclude.push_back(this->camera->getConclude());
  this->communicate->send("OBJECT", conclude);

  this->setState(WAITING);

  return true;
}

bool Controller::idle()
{
  ESP_LOGI(this->NAME, "Idle...");
  return true;
}
