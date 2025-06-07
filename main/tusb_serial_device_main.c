/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include "esp_log.h"
#include "esp_task.h"
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

#define APP_TASK_STACK_SIZE (1024*10)

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
} cdc_message_t;

typedef cdc_message_t app_message_t;

typedef struct
{
  uint8_t  buffer[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
  size_t  len;
} usb_message_t;

static usb_message_t usb_msg = { .len = 0 };

void new_data(int itf, uint8_t* buf, size_t len)
{
  app_message_t app_msg =
  {
    .buf_len = len,
    .itf = itf,
  };
  memcpy(app_msg.buf, buf, len);
  xQueueSend(app_queue, &app_msg, 0);
}

static void app_task(void *arg)
{
  app_message_t msg;
  usb_msg.len=0;
  bool verbose=false;
  xQueueReset(app_queue);
  load_config(verbose);

  while(1)
  {
    if(xQueueReceive(app_queue, &msg, portMAX_DELAY))
    {
      if(msg.buf_len)
      {
        #if defined(CONFIG_USB_UART_ECHO)
        tinyusb_cdcacm_write_queue(msg.itf, msg.buf, msg.buf_len);
        //esp_err_t err = tinyusb_cdcacm_write_flush(msg.itf, 0);
        esp_err_t err = tinyusb_cdcacm_write_flush(msg.itf, pdMS_TO_TICKS(msg.buf_len*4));

        if(err != ESP_OK)
        {
          ESP_LOGE(TAG, "ECHO CDC ACM write flush error: %s", esp_err_to_name(err));
        }

        #endif

        for(size_t i=0; i< msg.buf_len ; i++)
        {
          usb_msg.buffer[usb_msg.len] = msg.buf[i];

          if(usb_msg.buffer[usb_msg.len] == '\r')
          {
            usb_msg.buffer[usb_msg.len] = '\0';
            cmd_parsing(usb_msg.buffer, usb_msg.len);
            usb_msg.len=0;
          }
          else
          {
            if(usb_msg.len < CONFIG_TINYUSB_CDC_RX_BUFSIZE)
              usb_msg.len++;
          }
        }
      }
    }
  }
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
  xTaskCreate(app_task, "app_task", APP_TASK_STACK_SIZE, NULL, (ESP_TASK_PRIO_MIN + 1), NULL);

  while(1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
