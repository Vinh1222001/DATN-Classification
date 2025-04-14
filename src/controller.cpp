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
  ESP_LOGI(this->NAME, "Initialize components...");

  this->camera = new Camera();
  if (this->camera == nullptr || this->camera->available())
  {
    ESP_LOGI(this->NAME, "Failed to create camera");
    return false;
  }
  ESP_LOGI(this->NAME, "Created camera");

  this->classifier = new Classification(camera);
  if (this->classifier == nullptr)
  {
    ESP_LOGI(this->NAME, "Failed to create Classifier");
    return false;
  }
  ESP_LOGI(this->NAME, "Created Classifier");

  this->webServer = new RWebServer(camera);
  if (this->classifier == nullptr)
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
  ESP_LOGI(this->NAME, "Creating Camera's task...");
  this->camera->createTask();
  delay(2000);

  if (this->classifier == nullptr)
  {
    ESP_LOGE(this->NAME, "Classifier is not initialized");
    return false;
  }
  ESP_LOGI(this->NAME, "Creating Classifier's task...");
  this->classifier->createTask();
  delay(2000);

  ESP_LOGI(this->NAME, "All component have been set up");
  this->setState(READY);
  delay(1000);
  return true;
}

bool Controller::ready()
{
  this->setState(START);
  return true;
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
  this->webServer->startCameraServer();

  if (this->classifier == nullptr)
  {
    ESP_LOGE(this->NAME, "Classifier is not initialized");
    return false;
  }
  ESP_LOGI(this->NAME, "Creating Classifier's task...");
  this->classifier->run();
  delay(2000);

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
