#include "web_server.hpp"

RWebServer::RWebServer(Camera *cam)
    : BaseModule(
          "RWEB_SERVER",
          4,
          1,
          RWEB_SERVER_TASK_STACK_DEPTH_LEVEL,
          1),
      camera(cam)
{
    this->server = new WebServer(80);
    ESP_LOGI(this->NAME, "Server uri: %s", this->server->uri().c_str());
    this->server->on(
        "/stream",
        HTTP_GET,
        [this]()
        {
            ESP_LOGI(this->NAME, "Server Callback");
            this->onStream();
        });
    if (this->camera != nullptr)
    {
        ESP_LOGI(this->NAME, "Server is starting...");
        this->server->begin();
    }
    else
    {
        ESP_LOGE(this->NAME, "Camera is NULL");
    }
}

RWebServer::~RWebServer()
{
}

void RWebServer::taskFn()
{
    ESP_LOGI(this->NAME, "taskFn");
    this->server->handleClient();
}

void RWebServer::onStream()
{
    WiFiClient client = this->server->client();
    if (!client)
    {
        ESP_LOGE(this->NAME, "Client is NULL");
        return;
    }

    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    client.print(response);
    bool isConneted = client.connected();
    ESP_LOGI(this->NAME, "Is Connected: %d", isConneted);
    while (isConneted)
    {
        ESP_LOGI(this->NAME, "Client ip: %s", client.localIP().toString().c_str());
        if (this->camera == nullptr)
        {
            ESP_LOGE(this->NAME, "Camera is still null");
            continue;
        }

        if (!this->camera->available())
        {
            ESP_LOGE(this->NAME, "Failed to init camera");
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE(this->NAME, "Failed to get frame buffer");
            continue;
        }
        ESP_LOGI(this->NAME, "Client printing...");
        client.print("--frame\r\nContent-Type: image/jpeg\r\n\r\n");
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
        delay(20);
    }

    client.stop();
}
