#pragma once

#include "esp_err.h"

// Inicializa el puerto UART
void uart_driver_init(void);

// Envía una cadena por UART
void uart_send_string(const char *str);