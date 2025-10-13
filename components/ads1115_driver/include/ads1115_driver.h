#pragma once

#include <stdint.h>

typedef struct {
    uint8_t channel;
    float *target_var;
    const char *name;
} ads1115_channel_t;

float ads_read_channel(uint8_t channel);
//float ads1115_read_voltage(uint8_t channel);
//void ads1115_task(void *pvParameter);

void ads1115_task(void *pvParameter);
// void start_mlx_tasks(void);