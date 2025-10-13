#pragma once

#include <stdint.h>

#define ADS_ADDR            0x48
#define ADS_REG_CONVERSION  0x00
#define ADS_REG_CONFIG      0x01

// configuracion de ads, bits de registro de config ads
#define ADS_OS              0x01       // 15
#define ADS_MUX             0x00      // 14:12 
#define ADS_PGA             0x01      // 11:9 // b001 4.096 v // b010 2.048 v (default)
#define ADS_MODE            0x01     // 8
#define ADS_DR              0x04       // 7:5 // b001 16 SPS // b100 128 SPS (default)
#define ADS_COMP_DES        0x03 // 4.3.2.1:0

uint16_t ads_read_channel(uint8_t channel);
//float ads1115_read_voltage(uint8_t channel);
//void ads1115_task(void *pvParameter);

void ads1115_task(void *pvParameter);
void start_mlx_tasks(void);