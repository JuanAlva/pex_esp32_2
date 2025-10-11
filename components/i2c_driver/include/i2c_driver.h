#pragma once
#include <stdint.h>

esp_err_t i2c_master_init(void);

uint16_t ads_read_channel(uint8_t channel);

uint16_t read_mlx90614(uint8_t addr, uint8_t reg);

void task_i2c(void *pvParameter);

void i2c_ads_start_task(void);