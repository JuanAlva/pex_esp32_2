#pragma once
#include "driver/gpio.h"
#include "esp_err.h"

// Define los pines aquí (puedes ajustarlos por proyecto)
#define INPUT1_GPIO 26
#define INPUT2_GPIO 33
#define INPUT3_GPIO 39

void digital_inputs_init(void);
void digital_inputs_start_task(void);
int digital_input_get_state(int index);
