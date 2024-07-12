#include "esp_camera.h"
#include <WiFi.h>
#include "esp_transport_tcp.h"
#include <stdio.h>

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
  config.frame_size = FRAMESIZE_HD;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

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
  while (true) {
    int64_t fr_start = esp_timer_get_time();

    while (true) {
      fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Unable to read frame buffer");
      } else {
        break;
      }
    }

    if (send_frame(transport, fb) == -1) {
      break;
    }

    esp_camera_fb_return(fb);

    frame_flow_server_feedback_t feedback = recv_feedback(transport, read_buffer);
    printf("%d\n", feedback.roi_x);

    int64_t ms_elapsed = (esp_timer_get_time() - fr_start) / 1000;
  }

  disconnect_from_server(transport);
}

void loop() {
  // put your main code here, to run repeatedly:
}
