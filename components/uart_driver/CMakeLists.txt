idf_component_register(SRCS "uart_driver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver" digital_inputs mlx90614_driver ads1115_driver ds18b20_driver)