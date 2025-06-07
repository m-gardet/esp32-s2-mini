idf_component_register(SRCS "tusb_serial_device_main.c" "command.c"
                       PRIV_REQUIRES nvs_flash esp_driver_gpio
                       INCLUDE_DIRS .)
