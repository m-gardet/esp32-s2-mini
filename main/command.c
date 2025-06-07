/*
 * command.c
 *
 *  Created on: 28 août 2016
 *      Author: mick
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"


#define UNUSED(x) ((void)(x))
static const char *TAG = "command";
#define VERSION "1.0.1"
#define END_OF_LINE "\n\r"
#define OS_WAITE_TIMEOUT 1000 // in ms
#define STORAGE_NAMESPACE "storage"

#define GPIO_NOPULL   0x00000000U
#define GPIO_PULLUP   0x00000001U
#define GPIO_PULLDOWN   0x00000002U

//#define GPIO_MODE_INPUT   0x00000000U
#define GPIO_MODE_OUTPUT_PP   GPIO_MODE_INPUT_OUTPUT
#undef GPIO_MODE_OUTPUT_OD
#define GPIO_MODE_OUTPUT_OD   GPIO_MODE_INPUT_OUTPUT_OD
#define GPIO_MODE_ANALOG   0x00000055U

#define GPIO_PIN_RESET  0x00000000U
#define GPIO_PIN_SET  0x00000001U

typedef struct
{
  const uint8_t name[5];
  uint8_t mode;
  uint8_t pull;
  uint8_t state;
} gpio_st;

typedef struct
{
  const uint8_t name[5];
  uint32_t channel;
} adc_st;

typedef struct
{
  uint32_t pin;
} GPIO_TypeDef;

#define NUMBER_OF_GPIO 41
#define INTEGRITY_NAME "ZZZZ"
typedef struct
{
  char sys_name[50];
  gpio_st gpio[NUMBER_OF_GPIO];
  char integrity_name[5];
} config_st;


#if defined(CONFIG_IDF_TARGET_ESP32S2)

const char RESERVED_PIN[][5] =
{
  //PINs    FUNCTIONs     LABELs
  /*
   "IO00",  // SW2
   "IO01",  // ADC1_CH0
   "IO02",  // ADC1_CH1
   "IO03",  // ADC1_CH2
   "IO04",  // ADC1_CH3
   "IO05",  // ADC1_CH4
   "IO06",  // ADC1_CH5
   "IO07",  // ADC1_CH6
   "IO08",  // ADC1_CH7
   "IO09",  // ADC1_CH8
   "IO10",  // ADC1_CH9
   "IO11",  // ADC2_CH0
   "IO12",  // ADC2_CH1
   "IO13",  // ADC2_CH2
   "IO14",  // ADC2_CH3
   "IO15",  // LED
   "IO16",  // ADC2_CH5
   "IO17",  // ADC2_CH6
   "IO18",  // ADC2_CH7
   */
  "IO19",  // USB D-
  "IO20",  // USB D+
  "IO21",  // not on connector
  "IO22",  //
  "IO23",  //
  "IO24",  //
  "IO25",  //
  "IO26",  //
  "IO27",  //
  "IO28",  //
  "IO29",  //
  "IO30",  //
  "IO31",  //
  "IO32",  //
  /*
  "IO33",  //
  "IO34",  //
  "IO35",  //
  "IO36",  //
  "IO37",  //
  "IO38",  //
  "IO39",  //
  "IO40",  //
  */
  "IO41",  // not on connector
  "IO42",  // not on connector
  "IO43",  // not on connector
  "IO44",  // not on connector
  "IO45",  // not on connector

  // end
  "\0"
};

adc_st adc_chanel [6] =
{
  // end
  { "\0", 0 }

};

const config_st default_config =
{
  {
    "IO Demo"
  },
  {
    { "IO00", GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_PIN_RESET },
    { "IO01", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO02", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO03", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO04", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO05", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO06", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO07", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO09", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO09", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO10", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO11", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO12", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO13", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO14", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO15", GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO16", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO17", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO18", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    //
    { "IO33", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO34", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO35", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO36", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO37", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO38", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO39", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    { "IO40", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET },
    // end
    { "\0", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET }
  },
  {INTEGRITY_NAME}
};

#else

const char RESERVED_PIN[][1] =
{
  //PINs    FUNCTIONs     LABELs
  // end
  "\0"
};

adc_st adc_chanel [1] =
{
  // end
  { "\0", 0 }

};

const config_st default_config =
{
  {
    "IO Demo"
  },
  {
    // end
    { "\0", GPIO_MODE_INPUT, GPIO_NOPULL, GPIO_PIN_RESET }
  },
  {INTEGRITY_NAME}
};

#endif

config_st config;


#define CMD_OK "*OK* "
#define CMD_KO "*KO* "

#define HELP_NAME "HELP"
#define HELP_NAME_LEN (sizeof(HELP_NAME)-1)

#define SET_SYS_NAME_NAME "SET_NAME"
#define SET_SYS_NAME_LEN (sizeof(SET_SYS_NAME_NAME)-1)

#define GET_SYS_NAME_NAME "GET_NAME"
#define GET_SYS_NAME_LEN (sizeof(GET_SYS_NAME_NAME)-1)

#define UNKNOWN_TOTAL_LEN  0xFFFF

#define GET_UID_NAME "GET_UID"
#define GET_UID_NAME_LEN (sizeof(GET_UID_NAME)-1)

#define SAVE_CONFIG_NAME "CONFIG_SAVE"
#define SAVE_CONFIG_NAME_LEN (sizeof(SAVE_CONFIG_NAME)-1)

#define DUMP_CONFIG_NAME "CONFIG_DUMP"
#define DUMP_CONFIG_NAME_LEN (sizeof(DUMP_CONFIG_NAME)-1)

#define LOAD_CONFIG_NAME "CONFIG_LOAD"
#define LOAD_CONFIG_NAME_LEN (sizeof(LOAD_CONFIG_NAME)-1)

#define DEFAULT_CONFIG_NAME "CONFIG_DEFAULT"
#define DEFAULT_CONFIG_NAME_LEN (sizeof(DEFAULT_CONFIG_NAME)-1)

#define GPIO_INIT_NAME "INIT"
#define GPIO_INIT_NAME_LEN (sizeof(GPIO_INIT_NAME)-1)

#define GPIO_NAME_LEN 4

#define GPIO_MODE_LEN     3
#define MODE_INPUT        "INP"
#define MODE_OUTPUT_PP    "OPP"
#define MODE_OUTPUT_OD    "OOD"
#define MODE_INPUT_ADC    "ADC"

#define GPIO_PULL_LEN  2
#define NOPULL        "NO"
#define PULLUP        "UP"
#define PULLDOWN      "DO"

#define GPIO_INIT_TOTAL_LEN (GPIO_INIT_NAME_LEN+1 +GPIO_NAME_LEN+1  + GPIO_MODE_LEN+1 + GPIO_PULL_LEN)

#define GPIO_SET_NAME "SET"
#define GPIO_SET_NAME_LEN (sizeof(GPIO_SET_NAME)-1)
#define GPIO_VALUE_LEN 1
#define GPIO_SET_TOTAL_LEN (GPIO_SET_NAME_LEN+1 +GPIO_NAME_LEN+1  + GPIO_VALUE_LEN)

#define GPIO_GET_NAME "GET"
#define GPIO_GET_NAME_LEN (sizeof(GPIO_GET_NAME)-1)
#define GPIO_GET_TOTAL_LEN (GPIO_GET_NAME_LEN+1 +GPIO_NAME_LEN)


typedef int (*parsing_function)(uint8_t* cmd, uint16_t len);

typedef struct
{
  uint16_t len_total;
  uint16_t len_name;
  const char * name;
  parsing_function func;
} cmd_st;

int help(uint8_t* cmd, uint16_t len);
int cmd_uid(uint8_t* cmd, uint16_t len);
int cmd_set_sys_nanme(uint8_t* cmd, uint16_t len);
int cmd_get_sys_nanme(uint8_t* cmd, uint16_t len);
int cmd_save_config(uint8_t* cmd, uint16_t len);
int cmd_dump_config(uint8_t* cmd, uint16_t len);
int cmd_load_config(uint8_t* cmd, uint16_t len);
int cmd_default_config(uint8_t* cmd, uint16_t len);
int cmd_init(uint8_t* cmd, uint16_t len);
int cmd_set(uint8_t* cmd, uint16_t len);
int cmd_get(uint8_t* cmd, uint16_t len);

cmd_st cmd_list[] =
{
  { HELP_NAME_LEN, HELP_NAME_LEN,  HELP_NAME, help },
  { GET_UID_NAME_LEN, GET_UID_NAME_LEN,  GET_UID_NAME, cmd_uid },
  { UNKNOWN_TOTAL_LEN, SET_SYS_NAME_LEN,  SET_SYS_NAME_NAME, cmd_set_sys_nanme  },
  { GET_SYS_NAME_LEN, GET_SYS_NAME_LEN,  GET_SYS_NAME_NAME, cmd_get_sys_nanme  },

  { SAVE_CONFIG_NAME_LEN, SAVE_CONFIG_NAME_LEN, SAVE_CONFIG_NAME, cmd_save_config },
  { DUMP_CONFIG_NAME_LEN, DUMP_CONFIG_NAME_LEN, DUMP_CONFIG_NAME, cmd_dump_config },
  { LOAD_CONFIG_NAME_LEN, LOAD_CONFIG_NAME_LEN, LOAD_CONFIG_NAME, cmd_load_config },
  { DEFAULT_CONFIG_NAME_LEN, DEFAULT_CONFIG_NAME_LEN, DEFAULT_CONFIG_NAME, cmd_default_config },

  { GPIO_INIT_TOTAL_LEN, GPIO_INIT_NAME_LEN,  GPIO_INIT_NAME, cmd_init },
  { GPIO_SET_TOTAL_LEN, GPIO_SET_NAME_LEN,  GPIO_SET_NAME, cmd_set  },
  { GPIO_GET_TOTAL_LEN, GPIO_GET_NAME_LEN,  GPIO_GET_NAME, cmd_get  },

  // end of list
  {0, 0, "", NULL }
};


int usb_printf(char* buffer, ...);

int usb_printf(char* buffer, ...)
{
  char tubOutputBuffer[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
  va_list argument;
  int len = 0;
  //Translate arguments into a character buffer
  va_start(argument, buffer);
  len = vsnprintf(tubOutputBuffer, sizeof(tubOutputBuffer), buffer, argument);
  va_end(argument);

  if(len>0)
  {
    tinyusb_cdcacm_write_queue(TINYUSB_USBDEV_0, tubOutputBuffer, len);
    //esp_err_t err = tinyusb_cdcacm_write_flush(TINYUSB_USBDEV_0, 0);
    esp_err_t err = tinyusb_cdcacm_write_flush(TINYUSB_USBDEV_0, pdMS_TO_TICKS(len*4));
    //vTaskDelay(pdMS_TO_TICKS(len)*11);

    if(err != ESP_OK)
    {
      ESP_LOGE(TAG, "PRINT CDC ACM write flush error: %s", esp_err_to_name(err));
    }
  }

  return len;
}

int help(uint8_t* cmd, uint16_t len)
{
  UNUSED(cmd);
  UNUSED(len);
  usb_printf("\n\r");
  usb_printf("----------------------help----------------------\n\r");
  usb_printf("APP VERSION %s\n\r", VERSION);
  usb_printf("HELP (this message)\n\r");
  usb_printf("--------------system configuration--------------\n\r");
  usb_printf("%s XXXXXX (len max %d) \n\r", SET_SYS_NAME_NAME, SET_SYS_NAME_LEN+sizeof(config.sys_name));
  usb_printf("%s (%d)\n\r", GET_SYS_NAME_NAME, GET_SYS_NAME_LEN);
  usb_printf("%s (%d)\n\r", GET_UID_NAME, GET_UID_NAME_LEN);
  usb_printf("%s (%d)\n\r", SAVE_CONFIG_NAME, SAVE_CONFIG_NAME_LEN);
  usb_printf("%s (%d)\n\r", DUMP_CONFIG_NAME, DUMP_CONFIG_NAME_LEN);
  usb_printf("%s (%d)\n\r", LOAD_CONFIG_NAME, LOAD_CONFIG_NAME_LEN);
  usb_printf("%s (%d)\n\r", DEFAULT_CONFIG_NAME, DEFAULT_CONFIG_NAME_LEN);
  usb_printf("---------------gpio configuration---------------\n\r");
  usb_printf("gpio configuration\n\r");
  usb_printf("%s NAME MODE PULL\n\r", GPIO_INIT_NAME);
  usb_printf("  NAME (%d bytes)\n\r", GPIO_NAME_LEN);
  usb_printf("        IO01, IO15, IO33, ...\n\r");
  usb_printf("  MODE (%d bytes) \n\r", GPIO_MODE_LEN);
  usb_printf("        %s   (Input Floating)\n\r", MODE_INPUT);
  usb_printf("        %s   (Input ADC IO01, IO02)\n\r", MODE_INPUT_ADC);
  usb_printf("        %s   (Output Push Pull Mode)\n\r", MODE_OUTPUT_PP);
  usb_printf("        %s   (Output Open Drain Mode)\n\r", MODE_OUTPUT_OD);
  usb_printf("  PULL (%d bytes) \n\r", GPIO_PULL_LEN);
  usb_printf("        %s   (No Pull-up or Pull-down)\n\r", NOPULL);
  usb_printf("        %s   (Pull-up)\n\r", PULLUP);
  usb_printf("        %s   (Pull-down)\n\r", PULLDOWN);
  usb_printf("sample\n\r");
  usb_printf("INIT IO02 OPP NO (%d bytes)\n\r", GPIO_INIT_NAME_LEN);
  usb_printf("-----------------gpio read/write----------------\n\r");
  usb_printf("GET NAME\n\r");
  usb_printf("%s NAME VALUE (%d bytes)\n\r", GPIO_SET_NAME, GPIO_SET_TOTAL_LEN);
  /*
    usb_printf("--------------------adc  read-------------------\n\r");
    usb_printf("ADC NUMBER ( 00 to 01)\n\r");
    usb_printf("\n\r");
  */
  return 0;
}

void cmd_parsing(uint8_t* cmd, size_t len)
{
  if(len >= HELP_NAME_LEN)
  {
    uint16_t index = 0;

    while(cmd_list[index].len_total != 0)
    {
      if((len ==  cmd_list[index].len_total  ||  cmd_list[index].len_total == UNKNOWN_TOTAL_LEN)
         && (strncmp((const char *)cmd, cmd_list[index].name, cmd_list[index].len_name) == 0))
      {
        cmd_list[index].func(cmd, len);
        return;
      }

      index++;
    }
  }

  usb_printf(CMD_KO "Invalid command: %s (%lu bytes) try HELP\n\r", cmd, len);
}

int gpio_write_pin(GPIO_TypeDef* GPIOx, uint32_t pin, uint8_t state)
{
  return gpio_set_level(pin, state);
}

int gpio_read_pin(GPIO_TypeDef* GPIOx, uint32_t pin)
{
  return gpio_get_level(pin);
}

int gpio_init(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t mode, uint32_t pull)
{
  //zero-initialize the config structure.
  gpio_config_t io_conf = {};
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = mode;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1)<<pin));

  if(pull == GPIO_PULLUP)
  {
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
  }
  else if(pull == GPIO_PULLDOWN)
  {
    //enable pull-down mode
    io_conf.pull_down_en = 1;
    //disable pull-up mode
    io_conf.pull_up_en = 1;
  }
  else  // GPIO_NOPULL
  {
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
  }

  //configure GPIO with the given settings
  return gpio_config(&io_conf);
}

int get_port_and_pin(const uint8_t name[GPIO_NAME_LEN], GPIO_TypeDef **port, uint32_t *pin)
{
  int number=-1;

  if(name[0] != 'I' && name[0] != 'O')
  {
    usb_printf(CMD_KO "Invalid format: %c%c != IOXX\n\r", name[0], name[1]);
    return -1;
  }

  number = (name[2]-'0')*10 + (name[3]-'0');

  if(number > (NUMBER_OF_GPIO-1))
  {
    usb_printf(CMD_KO "Invalid format: IO00 to IO%d\n\r", (NUMBER_OF_GPIO-1));
    return -1;
  }

  *pin = number;
  return 0;
}

int is_reserved(uint8_t name[GPIO_NAME_LEN])
{
  return 0;
  int r=0;

  while(RESERVED_PIN[r][0] != '\0')
  {
    if(strncmp((const char *)name, &RESERVED_PIN[r][0], GPIO_NAME_LEN) == 0)
    {
      usb_printf(CMD_KO "%s is an reserver pin\n\r", RESERVED_PIN[r]);
      return -1;
    }

    r++;
  }

  return 0;
}


int get_mode(uint8_t modeTxt[GPIO_MODE_LEN], uint32_t *mode)
{
  if(strncmp((const char *)modeTxt, MODE_INPUT, GPIO_MODE_LEN) == 0)
  {
    *mode = GPIO_MODE_INPUT;
    return 0;
  }
  else if(strncmp((const char *)modeTxt, MODE_OUTPUT_PP, GPIO_MODE_LEN) == 0)
  {
    *mode = GPIO_MODE_OUTPUT_PP;
    return 0;
  }
  else if(strncmp((const char *)modeTxt, MODE_OUTPUT_OD, GPIO_MODE_LEN) == 0)
  {
    *mode = GPIO_MODE_OUTPUT_OD;
    return 0;
  }
  else if(strncmp((const char *)modeTxt, MODE_INPUT_ADC, GPIO_MODE_LEN) == 0)
  {
    *mode = GPIO_MODE_ANALOG;
    return 0;
  }

  usb_printf(CMD_KO "unknown mode : %s\n\r", modeTxt);
  return -1;
}

int get_pull(uint8_t pullTxt[GPIO_PULL_LEN], uint32_t *pull)
{
  if(strncmp((const char *)pullTxt, NOPULL, GPIO_PULL_LEN) == 0)
  {
    *pull = GPIO_NOPULL;
    return 0;
  }
  else if(strncmp((const char *)pullTxt, PULLUP, GPIO_PULL_LEN) == 0)
  {
    *pull = GPIO_PULLUP;
    return 0;
  }
  else if(strncmp((const char *)pullTxt, PULLDOWN, GPIO_PULL_LEN) == 0)
  {
    *pull = GPIO_PULLDOWN;
    return 0;
  }

  usb_printf(CMD_KO "unknown pull : %s\n\r", pullTxt);
  return -1;
}


int cmd_uid(uint8_t* cmd, uint16_t len)
{
  UNUSED(cmd);
  UNUSED(len);
  unsigned char mac_base[6] = {0};
  esp_read_mac(mac_base, ESP_MAC_EFUSE_FACTORY);
  char txt[6*2+2];
  int i=0;

  for(; i< sizeof(mac_base) ; i++)
  {
    sprintf(&txt[i*2], "%02X", mac_base[i]);
  }

  txt[i*2]= '\0';
  usb_printf(CMD_OK "value :%s\n\r", txt);
  return 0;
}

int get_config_index(const uint8_t name[4])
{
  uint16_t i=0;

  while(config.gpio[i].name[0] != '\0')
  {
    if(strncmp((const char *)&config.gpio[i].name[0], (const char *)name, 4) == 0)
    {
      return i;
    }

    i++;
  }

  return -1;
}

uint32_t get_adc_channel(const uint8_t name[4])
{
  uint16_t i=0;

  while(adc_chanel[i].name[0] != '\0')
  {
    if(strncmp((const char *)&adc_chanel[i].name[0], (const char *)name, 4) == 0)
    {
      return adc_chanel[i].channel;
    }

    i++;
  }

  return 0;
}

int update_config_value(const uint8_t name[4], uint8_t state)
{
  int index = get_config_index(name);

  if(index <0)
  {
    return -1;
  }

  config.gpio[index].state = state;
  return 0;
}

int update_config_setting(const uint8_t name[4], uint8_t mode, uint8_t pull)
{
  int index = get_config_index(name);

  if(index <0)
  {
    return -1;
  }

  config.gpio[index].mode = mode;
  config.gpio[index].pull = pull;
  return 0;
}


const char *get_mode_txt(uint32_t mode)
{
  switch(mode)
  {
    case GPIO_MODE_INPUT:
      return MODE_INPUT;

    case GPIO_MODE_ANALOG:
      return MODE_INPUT_ADC;

    case GPIO_MODE_OUTPUT_PP:
      return MODE_OUTPUT_PP;

    case GPIO_MODE_OUTPUT_OD:
      return MODE_OUTPUT_OD;

    default:
      return "UNK";
  }
}

const char *get_pull_txt(uint32_t pull)
{
  switch(pull)
  {
    case GPIO_NOPULL:
      return NOPULL;

    case GPIO_PULLUP:
      return PULLUP;

    case GPIO_PULLDOWN:
      return PULLDOWN;

    default:
      return "UNK";
  }
}

int cmd_dump_config(uint8_t* cmd, uint16_t len)
{
  UNUSED(cmd);
  UNUSED(len);
  usb_printf("CONFIG SIZE: %d\n\r", sizeof(config));
  usb_printf("SYS NAME: %s\n\r", config.sys_name);
  uint16_t i=0;

  while(config.gpio[i].name[0] != '\0')
  {
    GPIO_TypeDef *port=0;
    uint32_t pin=0 ;
    int32_t value=-1;

    if(get_port_and_pin(config.gpio[i].name, &port, &pin) ==0)
    {
      value = gpio_read_pin(port, pin);
    }

    usb_printf("%s %s %s config %d value %d\n\r",
               config.gpio[i].name,
               get_mode_txt(config.gpio[i].mode),
               get_pull_txt(config.gpio[i].pull),
               config.gpio[i].state,
               value
              );
    i++;
  }

  usb_printf(CMD_OK "\n\r");
  return 0;
}


int cmd_set(uint8_t* cmd, uint16_t len)
{
  UNUSED(len);
  uint8_t* arg= cmd;
  arg+= (GPIO_SET_NAME_LEN+1);
  GPIO_TypeDef *GPIOx=0;
  uint32_t pin=0;

  if(get_port_and_pin(arg, &GPIOx, &pin) !=0)
    return -1;

  int index = get_config_index(arg);

  if(index < 0)
  {
    usb_printf(CMD_KO " configuration index not found for cmd %s\n\r", cmd);
    return -1;
  }

  if(config.gpio[index].mode != GPIO_MODE_OUTPUT_PP && config.gpio[index].mode != GPIO_MODE_OUTPUT_OD)
  {
    usb_printf(CMD_KO " this pin is not an ouput %s => cmd %s\n\r", get_mode_txt(config.gpio[index].mode), cmd);
    return -1;
  }

  arg+= (GPIO_NAME_LEN+1);
  uint32_t value =0;

  if(arg[0] == '0')
    value = GPIO_PIN_RESET;
  else if(arg[0] == '1')
    value = GPIO_PIN_SET;
  else
  {
    usb_printf(CMD_KO "unknown value : %s\n\r", arg);
    return -1;
  }

  if(gpio_write_pin(GPIOx, pin, value) != 0)
  {
    usb_printf(CMD_KO " gpio_write_pin \n\r");
    return -1;
  }

  update_config_value(&cmd[(GPIO_SET_NAME_LEN+1)], value);
  usb_printf(CMD_OK "\n\r");
  return 0;
}

int cmd_get(uint8_t* cmd, uint16_t len)
{
  UNUSED(len);
  uint8_t* arg= cmd;
  arg+= (GPIO_GET_NAME_LEN+1);
  GPIO_TypeDef *GPIOx=0;
  uint32_t pin=0;

  if(get_port_and_pin(arg, &GPIOx, &pin) !=0)
    return -1;

  int index = get_config_index(arg);

  if(index < 0)
  {
    usb_printf(CMD_KO " configuration index not found for cmd %s\n\r", cmd);
    return -1;
  }

  if(config.gpio[index].mode == GPIO_MODE_ANALOG)
  {
    uint32_t channel = get_adc_channel(arg);

    if(channel == 0)
    {
      usb_printf(CMD_KO " adc channel not found for cmd %s\n\r", cmd);
      return -1;
    }

    int32_t value = -5; //adc_read(channel);

    if(value < 0)
    {
      usb_printf(CMD_KO " adc read error %s\n\r", cmd);
      return -1;
    }

    usb_printf(CMD_OK "value :%d\n\r", value);
  }
  else
  {
    uint32_t value = gpio_read_pin(GPIOx, pin);
    usb_printf(CMD_OK "value :%d\n\r", value);
  }

  return 0;
}

int cmd_set_sys_nanme(uint8_t* cmd, uint16_t len)
{
  uint8_t* arg= cmd;
  arg+= (SET_SYS_NAME_LEN+1);
  uint8_t l=len - (SET_SYS_NAME_LEN+1);

  if(l >= (sizeof(config.sys_name) -1))
    l = sizeof(config.sys_name) -1;

  memcpy(config.sys_name, arg, l);
  config.sys_name[l] = '\0';
  usb_printf(CMD_OK "\n\r");
  return 0;
}

int cmd_get_sys_nanme(uint8_t* cmd, uint16_t len)
{
  UNUSED(cmd);
  UNUSED(len);
  usb_printf(CMD_OK "value :%s\n\r", config.sys_name);
  return 0;
}

int load_config(bool verbose)
{
  uint16_t i=0;
  nvs_handle_t my_handle;
  config_st config_flash = { .sys_name[0] = 0xFF };
  esp_err_t err;
  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);

  if(err == ESP_OK)
  {
    size_t config_flash_size = sizeof(config_flash);
    err = nvs_get_blob(my_handle, "config", &config_flash, &config_flash_size);

    if(err == ESP_OK)
    {
      ESP_LOGI(TAG, "Reading config form flash:");

      if(config_flash_size != sizeof(config_flash))
      {
        nvs_close(my_handle);
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
        ESP_LOGE(TAG, "size error we delete everything");
      }
    }
    else
    {
      ESP_LOGW(TAG, "config data not found in flasg!");
    }

    nvs_close(my_handle);
  }
  else
  {
    ESP_LOGE(TAG, "nvs open partition error %s", esp_err_to_name(err));
  }

  if(
    config_flash.sys_name[0] != 0xFF &&
    config_flash.sys_name[0] != 0x00 &&
    memcmp(&config_flash.integrity_name[0], &default_config.integrity_name[0], sizeof(default_config.integrity_name)) == 0
  )
  {
    if(verbose)
    {
      usb_printf("use flash config\n\r");
    }

    memcpy(&config, &config_flash, sizeof(config));
  }
  else
  {
    if(verbose)
    {
      usb_printf("use default config\n\r");
    }

    memcpy(&config, &default_config, sizeof(config));
  }

  GPIO_TypeDef *port=0;
  uint32_t pin=0 ;

  while(config.gpio[i].name[0] != '\0')
  {
    if(verbose)
    {
      usb_printf("%s %s %s\n\r", config.gpio[i].name, get_mode_txt(config.gpio[i].mode), get_pull_txt(config.gpio[i].pull));
    }

    if(get_port_and_pin(config.gpio[i].name, &port, &pin) == 0)
    {
      if(gpio_init(port, pin, config.gpio[i].mode, config.gpio[i].pull) == 0)
      {
        if(config.gpio[i].mode != GPIO_MODE_INPUT)
        {
          gpio_write_pin(port, pin, config.gpio[i].state);
        }
      }
    }

    i++;
  }

  return 0;
}

int cmd_load_config(uint8_t* cmd, uint16_t len)
{
  bool verbose=true;
  load_config(verbose);
}

int cmd_save_config(uint8_t* cmd, uint16_t len)
{
  UNUSED(cmd);
  UNUSED(len);
  nvs_handle_t my_handle;
  esp_err_t err;
  // Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);

  if(err != ESP_OK)
  {
    usb_printf(CMD_KO "flash Update error NVS open %s\n\r",  esp_err_to_name(err));
    return -1;
  }

  // Write blob
  err = nvs_set_blob(my_handle, "config", (void*)&config, sizeof(config));

  if(err != ESP_OK)
  {
    usb_printf(CMD_KO "flash Update error nvs set %s\n\r",  esp_err_to_name(err));
    nvs_close(my_handle);
    return -1;
  }

  // Commit
  err = nvs_commit(my_handle);

  if(err != ESP_OK)
  {
    usb_printf(CMD_KO "flash Update error nvs_commit %s\n\r",  esp_err_to_name(err));
    nvs_close(my_handle);
    return -1;
  }

  nvs_close(my_handle);
  usb_printf(CMD_OK "\n\r");
  return 0;
}

int cmd_init(uint8_t* cmd, uint16_t len)
{
  UNUSED(len);
  uint8_t* arg= cmd;
  arg+= (GPIO_INIT_NAME_LEN+1);
  GPIO_TypeDef *GPIOx=0;
  uint32_t pin=0;

  if(get_port_and_pin(arg, &GPIOx, &pin) !=0)
    return -1;

  if(is_reserved(arg) !=0)
    return -1;

  uint32_t mode;
  arg+= (GPIO_NAME_LEN+1);

  if(get_mode(arg, &mode) != 0)
    return -1;

  uint32_t pull;
  arg+= (GPIO_MODE_LEN+1);

  if(get_pull(arg, &pull) != 0)
    return -1;

  if(gpio_init(GPIOx, pin, mode, pull) != 0)
  {
    usb_printf(CMD_KO " gpio_init \n\r");
    return-1;
  }

  update_config_setting(&cmd[(GPIO_INIT_NAME_LEN+1)], mode, pull);
  usb_printf(CMD_OK "\n\r");
  return 0;
}

int cmd_default_config(uint8_t* cmd, uint16_t len)
{
  memcpy(&config, &default_config, sizeof(config));
  return cmd_save_config(cmd, len);
}
