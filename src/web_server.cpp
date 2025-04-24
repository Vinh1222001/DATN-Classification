#include "web_server.hpp"

RWebServer::RWebServer(Camera *cam)
    : BaseModule(
          "RWEB_SERVER",
          RWEB_SERVER_TASK_PRIORITY,
          1,
          RWEB_SERVER_TASK_STACK_DEPTH_LEVEL,
          RWEB_SERVER_TASK_PINNED_CORE_ID),
      camera(cam)
{
    this->server = new WebServer(80);
    this->server->on(
        "/stream",
        HTTP_GET,
        [&]()
        {
            this->onStream();
        });
    this->server->begin();
}

RWebServer::~RWebServer()
{
}

void RWebServer::taskFn()
{
    this->server->handleClient();
}

void RWebServer::onStream()
{
    WiFiClient client = this->server->client();

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

        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
        delay(RWEB_SERVER_TASK_DELAY);
    }

    client.stop();
}
