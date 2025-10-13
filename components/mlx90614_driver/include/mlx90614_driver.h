#pragma once
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_driver.h"

// #define I2C_SLAVE_ADDR 0x68
#define MLX1_ADDR       0x5A
#define MLX2_ADDR       0x5B
#define MLX3_ADDR       0x5C

#define REG_TEMP_AMB    0x06
#define REG_TEMP_OBJ    0x07

typedef struct {
    uint8_t addr;
    float *temperature_var;
    const char *tag;
} mlx90614_t;

esp_err_t mlx90614_read_temp(uint8_t addr, float *temp);
void mlx90614_task(void *pvParameter);
