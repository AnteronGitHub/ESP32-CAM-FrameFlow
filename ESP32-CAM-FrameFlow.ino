#include "esp_camera.h"
#include <WiFi.h>
#include "esp_transport_tcp.h"
#include <stdio.h>
#include <inttypes.h>

#include "camera_pins.h"
#include "frame_flow.c"

#include "config.h"

void setup() {
  Serial.begin(115200);

  // Camera initialization
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 3;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Wi-Fi initialization
  Serial.print("Connecting to Wifi network.");
  WiFi.begin(WLAN_SSID, WLAN_PASSWORD);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected.");

  esp_transport_handle_t transport = connect_to_server(SERVER_ADDR, SERVER_PORT);

  // Stream video
  camera_fb_t* fb;
  char* read_buffer = (char*) malloc(4);
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;

  while (true) {
    int64_t fr_start = esp_timer_get_time();

    while (!(fb = esp_camera_fb_get())) {
      Serial.println("Unable to read frame buffer");
    }
    int64_t fr_captured = esp_timer_get_time();

    crop_png(fb, 200, 200, 200, 200);
    int64_t fr_cropped = esp_timer_get_time();

    fmt2jpg(fb->buf, fb->len, fb->width, fb->height, config.pixel_format, 80, &_jpg_buf, &_jpg_buf_len);
    int64_t fr_converted = esp_timer_get_time();

    if (send_frame(transport, _jpg_buf, _jpg_buf_len) == -1) {
      break;
    }
    int64_t fr_sent = esp_timer_get_time();

    esp_camera_fb_return(fb);
    free(_jpg_buf);
    _jpg_buf = NULL;

    int64_t fr_buffers_managed = esp_timer_get_time();

    frame_flow_server_feedback_t feedback = recv_feedback(transport, read_buffer);

    int64_t fr_feedback = esp_timer_get_time();

    printf("capture: %" PRIu64, (fr_captured - fr_start) / 1000);
    printf(", crop: %" PRIu64, (fr_cropped - fr_captured) / 1000);
    printf(", conversion: %" PRIu64, (fr_converted - fr_cropped) / 1000);
    printf(", transmission: %" PRIu64, (fr_sent - fr_converted) / 1000);
    printf(", buffer management: %" PRIu64, (fr_buffers_managed - fr_sent) / 1000);
    printf(", feedback received: %" PRIu64 "\n", (fr_feedback - fr_buffers_managed) / 1000);
  }

  disconnect_from_server(transport);
}

void loop() {
  // put your main code here, to run repeatedly:
}
