/*
 * command.h
 *
 *  Created on: 28 août 2016
 *      Author: mick
 */

#define CONFIG_USB_UART_ECHO 1

#include <stdint.h>
int load_config(void);
void cmd_parsing(uint8_t* cmd, size_t len);
