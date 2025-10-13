#pragma once
#include "driver/i2c.h"
#include "esp_err.h"

// i2c master parameters
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          10000 // modificacion para low-speed 10000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_TIMEOUT_MS              1000

esp_err_t i2c_master_init(void);