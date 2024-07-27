#include <stdio.h>

#include "esp_camera.h"
#include "esp_transport_tcp.h"

typedef struct frame_flow_server_feedback {
  unsigned int roi_x;    // Region of Interest x index
} frame_flow_server_feedback_t;

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
int send_frame(esp_transport_handle_t t, uint8_t * buf, size_t len) {
  // Send the header
  if (esp_transport_write(t, (char *) &len, 4, 1000) == -1) {
    return -1;
  }

  // Send the frame
  if (esp_transport_write(t, (char *) buf, len, 1000) == -1) {
    return -1;
  }

  return 0;
}

frame_flow_server_feedback_t recv_feedback(esp_transport_handle_t t, char* rx_buffer) {
  esp_transport_read(t, rx_buffer, 4, 1000);
  unsigned int number = *(unsigned int *) rx_buffer;
  frame_flow_server_feedback_t feedback = { .roi_x = number };
  return feedback;
}

/**
 * Implementation copied from: https://simplyexplained.com/blog/esp32-cam-cropping-images-on-device/
 */
void crop_png(camera_fb_t *fb, unsigned short cropLeft, unsigned short cropRight, unsigned short cropTop, unsigned short cropBottom)
{
    unsigned int maxTopIndex = cropTop * fb->width * 2;
    unsigned int minBottomIndex = ((fb->width*fb->height) - (cropBottom * fb->width)) * 2;
    unsigned short maxX = fb->width - cropRight; // In pixels
    unsigned short newWidth = fb->width - cropLeft - cropRight;
    unsigned short newHeight = fb->height - cropTop - cropBottom;
    size_t newJpgSize = newWidth * newHeight *2;

    unsigned int writeIndex = 0;
    // Loop over all bytes
    for(int i = 0; i < fb->len; i+=2){
        // Calculate current X, Y pixel position
        int x = (i/2) % fb->width;

        // Crop from the top
        if(i < maxTopIndex){ continue; }

        // Crop from the bottom
        if(i > minBottomIndex){ continue; }

        // Crop from the left
        if(x <= cropLeft){ continue; }

        // Crop from the right
        if(x > maxX){ continue; }

        // If we get here, keep the pixels
        fb->buf[writeIndex++] = fb->buf[i];
        fb->buf[writeIndex++] = fb->buf[i+1];
    }

    // Set the new dimensions of the framebuffer for further use.
    fb->width = newWidth;
    fb->height = newHeight;
    fb->len = newJpgSize;
}