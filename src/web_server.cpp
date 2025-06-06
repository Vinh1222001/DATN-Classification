#include "web_server.hpp"

RWebServer::RWebServer(Camera *cam)
    : BaseModule(
          "RWEB_SERVER",
          RWEB_SERVER_TASK_PRIORITY,
          RWEB_SERVER_TASK_DELAY,
          RWEB_SERVER_TASK_STACK_DEPTH_LEVEL,
          RWEB_SERVER_TASK_PINNED_CORE_ID),
      camera(cam)
{
    this->server = new WebServer(80);
    this->server->on(
        "/stream",
        HTTP_GET,
        [this]()
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

    while (client.connected())
    {
        if (this->camera == nullptr)
        {
            ESP_LOGE(this->NAME, "Camera is still null");
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            continue;
        }

        client.print("--frame\r\n");
        client.print("Content-Type: image/jpeg\r\n\r\n");
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
        delay(RWEB_SERVER_TASK_DELAY);
    }

    client.stop();
    vTaskDelete(NULL);
}
