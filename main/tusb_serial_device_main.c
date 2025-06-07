/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#if (CONFIG_TINYUSB_CDC_COUNT > 1)
  #include "tusb_console.h"
#endif
#include "sdkconfig.h"
#include "command.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "main";
static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

/**
 * @brief Application Queue
 */
static QueueHandle_t app_queue;

typedef struct
{
  uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];     // Data buffer
  size_t buf_len;                                     // Number of bytes received
  uint8_t itf;                                        // Index of CDC device interface
} app_message_t;

typedef struct
{
  uint8_t  buffer[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
  size_t  len;
} usb_message_t;

static usb_message_t usb_msg = { .len = 0 };

void new_data(int itf, uint8_t* buf, size_t len)
{
  for(size_t i=0; i< len ; i++)
  {
    usb_msg.buffer[usb_msg.len] = buf[i];

    if(usb_msg.buffer[usb_msg.len] == '\r')
    {
      usb_msg.buffer[usb_msg.len] = '\0';
      app_message_t tx_msg =
      {
        .buf_len = usb_msg.len,
        .itf = itf,
      };
      memcpy(tx_msg.buf, usb_msg.buffer, usb_msg.len);
      xQueueSend(app_queue, &tx_msg, 0);
      usb_msg.len=0;
    }
    else
    {
      if(usb_msg.len < CONFIG_TINYUSB_CDC_RX_BUFSIZE)
        usb_msg.len++;
    }
  }

  #if defined(CONFIG_USB_UART_ECHO)
  tinyusb_cdcacm_write_queue(itf, buf, len);
  esp_err_t err = tinyusb_cdcacm_write_flush(itf, 0);

  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "CDC ACM write flush error: %s", esp_err_to_name(err));
  }

  #endif
}

/**
 * @brief CDC device RX callback
 *
 * CDC device signals, that new data were received
 *
 * @param[in] itf   CDC device index
 * @param[in] event CDC event type
 */
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
  /* initialization */
  size_t rx_size = 0;
  /* read */
  esp_err_t ret = tinyusb_cdcacm_read(itf, rx_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);

  if(ret == ESP_OK)
  {
    new_data(itf, rx_buf, rx_size);
  }
  else
  {
    ESP_LOGE(TAG, "Read Error");
  }
}

/**
 * @brief CDC device line change callback
 *
 * CDC device signals, that the DTR, RTS states changed
 *
 * @param[in] itf   CDC device index
 * @param[in] event CDC event type
 */
void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
  int dtr = event->line_state_changed_data.dtr;
  int rts = event->line_state_changed_data.rts;
  ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}

void app_main(void)
{
  // Create FreeRTOS primitives
  app_queue = xQueueCreate(5, sizeof(app_message_t));
  assert(app_queue);
  app_message_t msg;
  ESP_LOGI(TAG, "USB initialization");
  const tinyusb_config_t tusb_cfg =
  {
    .device_descriptor = NULL,
    .string_descriptor = NULL,
    .external_phy = false,
    #if (TUD_OPT_HIGH_SPEED)
    .fs_configuration_descriptor = NULL,
    .hs_configuration_descriptor = NULL,
    .qualifier_descriptor = NULL,
    #else
    .configuration_descriptor = NULL,
    #endif // TUD_OPT_HIGH_SPEED
  };
  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
  tinyusb_config_cdcacm_t acm_cfg =
  {
    .usb_dev = TINYUSB_USBDEV_0,
    .cdc_port = TINYUSB_CDC_ACM_0,
    .rx_unread_buf_sz = 64,
    .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
    .callback_rx_wanted_char = NULL,
    .callback_line_state_changed = NULL,
    .callback_line_coding_changed = NULL
  };
  ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
  /* the second way to register a callback */
  ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
                    TINYUSB_CDC_ACM_0,
                    CDC_EVENT_LINE_STATE_CHANGED,
                    &tinyusb_cdc_line_state_changed_callback));
  #if (CONFIG_TINYUSB_CDC_COUNT > 1)
  tinyusb_config_cdcacm_t acm_cfg_console = { 0 };
  acm_cfg_console.cdc_port = TINYUSB_CDC_ACM_1;
  ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg_console));
  /*
  ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
                      TINYUSB_CDC_ACM_1,
                      CDC_EVENT_LINE_STATE_CHANGED,
                      &tinyusb_cdc_line_state_changed_callback));
  */
  esp_tusb_init_console(TINYUSB_CDC_ACM_1); // log to usb
  #endif
  ESP_LOGI(TAG, "USB initialization DONE");
  esp_err_t err;
  // Initialize NVS
  err = nvs_flash_init();

  if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }

  ESP_ERROR_CHECK(err);
  load_config();

  while(1)
  {
    if(xQueueReceive(app_queue, &msg, portMAX_DELAY))
    {
      if(msg.buf_len)
      {
        /* Print received data*/
        //ESP_LOGI(TAG, "Data from channel %d:", msg.itf);
        //ESP_LOG_BUFFER_HEXDUMP(TAG, msg.buf, msg.buf_len, ESP_LOG_INFO);
        cmd_parsing(msg.buf, msg.buf_len);
      }
    }
  }
}
