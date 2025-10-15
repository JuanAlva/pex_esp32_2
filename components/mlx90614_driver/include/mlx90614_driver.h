#pragma once

#include <stdint.h>

typedef struct {
    uint8_t addr;
    float *temperature_var;
    const char *tag;
} mlx90614_t;

float read_mlx90614(uint8_t addr);
// void mlx90614_task(void *pvParameter);
