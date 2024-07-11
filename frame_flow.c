#include "esp_camera.h"
#include "esp_transport_tcp.h"

/**
 * @brief      Connect to a server and return the transport object.
 *
 * @param[in]  addr            The server IP address
 * @param[in]  port            The server port
 *
 */
esp_transport_handle_t connect_to_server(char* addr, int port) {
  esp_transport_handle_t t = esp_transport_tcp_init();
  if (t == NULL) {
    return NULL;
  }
  if (esp_transport_connect(t, addr, port, 1000) == -1) {
    return NULL;
  }
  return t;
}

/**
 * @brief      Close the connection to a server.
 *
 * @param[in]  t   The transport handle
 *
 */
void disconnect_from_server(esp_transport_handle_t t) {
  esp_transport_close(t);
}

/**
 * @brief      Send a frame from the camera buffer.
 *
 * @param[in]  t   The transport handle
 * @param[in]  fb  The camera frame buffer handle
 *
 */
int send_frame(esp_transport_handle_t t, camera_fb_t* fb) {
  // Send the header
  if (esp_transport_write(t, (char *) &fb->len, 4, 1000) == -1) {
    return -1;
  }

  // Send the frame
  if (esp_transport_write(t, (char *) fb->buf, fb->len, 1000) == -1) {
    return -1;
  }

  return 0;
}