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
  ESP_LOGI(this->NAME, "Initialize components...");

  this->communicate = new Communicate();
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
  String response = this->communicate->getReceiveMsg();

  if (response.compareTo("PING") == 0)
  {
    ESP_LOGI(this->NAME, "Connection is Ok! Received message: %s", response.c_str());
    std::vector<String> msg;
    msg.push_back("OK");
    this->communicate->send(msg);
    this->setState(START);
    delay(1000);
    return true;
  }
  ESP_LOGE(this->NAME, "Connection Failed! Received message: %s", response.c_str());
  delay(1000);
  return false;
}

bool Controller::start()
{
  if (this->camera == nullptr)
  {
    ESP_LOGE(this->NAME, "Camera is not initialized");
    return false;
  }
  ESP_LOGI(this->NAME, "Running Camera's task...");
  this->camera->run();
  delay(2000);

  if (this->webServer == nullptr)
  {
    ESP_LOGE(this->NAME, "Web Server is not initialized");
    return false;
  }
  ESP_LOGI(this->NAME, "Running Web Server's task...");
  this->webServer->run();

  // if (this->classifier == nullptr)
  // {
  //   ESP_LOGE(this->NAME, "Classifier is not initialized");
  //   return false;
  // }
  // ESP_LOGI(this->NAME, "Creating Classifier's task...");
  // this->classifier->run();

  this->setState(WAITING);
  delay(2000);

  return true;
}

bool Controller::waiting()
{
  String response = this->communicate->getReceiveMsg();
  if (response.compareTo("CLASSIFY") == 0)
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
  String response = this->communicate->getReceiveMsg();
  if (response.compareTo("STOP_CLASSIFY") == 0)
  {
    ESP_LOGI(this->NAME, "Stop Classify, Return to Waiting state");
    this->camera->stopClassifying();
    this->setState(WAITING);
    delay(1000);
    return true;
  }

  ESP_LOGI(this->NAME, "Message: %s, Continue Classifying...", response.c_str());
  return false;
}

bool Controller::response()
{
  return true;
}

bool Controller::idle()
{
  return true;
}
