#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "i2c_driver.h"

// i2c master parameters
#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          10000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_TIMEOUT_MS              1000
#define DELAY_MS_I2C                1000

// #define I2C_SLAVE_ADDR 0x68
#define MLX1_ADDR           0x5A
#define MLX2_ADDR           0x5B
#define MLX3_ADDR           0x5C
#define REG_TEMP_OBJ        0x07
#define REG_TEMP_AMB        0x06

// direcciones y registros de ads
#define ADS_ADDR            0x48
#define ADS_REG_CONVERSION  0x00
#define ADS_REG_CONFIG      0x01

// configuracion de ads, bits de registro de config ads
#define ADS_OS              0x01       // 15
#define ADS_MUX             0x00      // 14:12
#define ADS_PGA             0x01      // 11:9
#define ADS_MODE            0x01     // 8
#define ADS_DR              0x01       // 7:5 // 16 SPS
#define ADS_COMP_DES        0x03 // 4.3.2.1:0

// ==================== TAGS PARA LOG ====================
static const char *TAG_I2C     = "I2C_TASK";

// ==================== VARIABLES GLOBALES ====================
float temp_mlx1 = 0.0f;
float temp_mlx2 = 0.0f;
float temp_mlx3 = 0.0f;

float ads_ch0 = 0.0f;
float ads_ch1 = 0.0f;
float ads_ch2 = 0.0f;
float ads_ch3 = 0.0f;

// ================= ESTRUCTURAS PLANTILLAS ===================
typedef struct {
    uint8_t addr;
    uint8_t reg;
    float *target_var;
    const char *name;
} mlx90614_sensor_t;

typedef struct {
    uint8_t channel;
    float *target_var;
    const char *name;
} ads1115_channel_t;

// ==================== INICIALIZACIÓN I2C ====================
esp_err_t i2c_master_init(void)
{
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .master.clk_speed = I2C_MASTER_FREQ_HZ, // importante verificar velocidad max en ds de componente
        .clk_flags = 0,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, i2c_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
    ESP_LOGI(TAG_I2C, "I2C inicializado correctamente");
    return ESP_OK;
}

// ==================== FUNCIONES BASE ====================

// Lectura de MLX90614 (raw)
uint16_t read_mlx90614(uint8_t addr, uint8_t reg)
{
    uint8_t data[3];
    i2c_master_write_read_device(I2C_NUM_0,
                                 addr,
                                 &reg,
                                 1,
                                 data,
                                 3,
                                 pdMS_TO_TICKS(I2C_TIMEOUT_MS));

    // combina los dos primeros bytes en un valor de 16 bits
    uint16_t raw = (data[1] << 8) | data[0];
    return raw;
}

// Lectura de canal ADS1115
uint16_t ads_read_channel(uint8_t channel)
{
    /**
     * Configuracion base:
     * - OS = 1 (start single conversion)
     * - MUX = canal (AIN0, AIN1, etc)
     * - PGA = +- 4.096 V
     * - MODE = single-shot
     * - DR = 128 SPS
     * - Comparator disabled
     */
    uint16_t config = 0;

    // Bit 15 -> OS = 1 (iniciar conversión en single-shot)
    config |= (1 << 15);
    // Bits 14-12 -> MUX (entrada)
    config |= (ADS_MUX & 0x07) << 12;
    // Bits 11-9 -> PGA
    config |= (ADS_PGA & 0x07) << 9;
    // Bit 8 -> MODE
    config |= (ADS_MODE & 0x01) << 8;
    // Bits 7-5 -> DR
    config |= (ADS_DR & 0x07) << 5;
    // Bits 4-0 -> deshabilitar comparador (COMP_QUE = 11)
    config |= ADS_COMP_DES;

    // Ajustar MUX segun canal
    switch (channel)
    {
    case 0:
        config |= (0x04 << 12);
        break; // AIN0 respecto a GND
    case 1:
        config |= (0x05 << 12);
        break; // AIN1
    case 2:
        config |= (0x06 << 12);
        break; // AIN2
    case 3:
        config |= (0x07 << 12);
        break; // AIN3
    }

    uint8_t write_buf[3];
    write_buf[0] = ADS_REG_CONFIG; // revisar por que
    write_buf[1] = (config >> 8) & 0xFF;
    write_buf[2] = config & 0xFF;

    // Escribir configuracion
    i2c_master_write_to_device(I2C_MASTER_NUM,
                               ADS_ADDR,
                               write_buf,
                               3,
                               I2C_TIMEOUT_MS / portTICK_PERIOD_MS);

    vTaskDelay(pdMS_TO_TICKS(10)); // timepo para conversion

    // Leer resultado
    uint8_t reg = ADS_REG_CONVERSION;
    uint8_t data[2];
    i2c_master_write_read_device(I2C_MASTER_NUM,
                                 ADS_ADDR,
                                 &reg,
                                 1,
                                 data,
                                 2,
                                 I2C_TIMEOUT_MS / portTICK_PERIOD_MS);

    int16_t result = (data[0] << 8) | data[1];
    return result;
}

// ==================== FUNCIONES PLANTILLA ====================

// Tarea genérica MLX90614
void mlx90614_task(void *pvParameter)
{
    mlx90614_sensor_t *sensor = (mlx90614_sensor_t *) pvParameter;
    while (1)
    {
        uint16_t raw = read_mlx90614_raw(sensor->addr, sensor->reg);
        float temp = (raw * 0.02) - 273.15;
        *(sensor->target_var) = temp;
        ESP_LOGI("MLX90614", "%s -> %.2f °C", sensor->name, temp);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Tarea genérica ADS1115
void ads1115_task(void *pvParameter)
{
    ads1115_channel_t *ch = (ads1115_channel_t *) pvParameter;
    while (1)
    {
        int16_t raw = ads_read_channel(ch->channel);
        float voltage = (raw * 4.096f) / 32768.0f;
        *(ch->target_var) = voltage;
        ESP_LOGI("ADS1115", "%s -> %.3f V", ch->name, voltage);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ==================== CREACIÓN DE TAREAS ====================
void start_mlx_tasks(void)
{
    static mlx90614_sensor_t mlx1 = { .addr = MLX1_ADDR, .reg = REG_TEMP_OBJ, .target_var = &temp_mlx1, .name = "MLX1 (Motor)" };
    static mlx90614_sensor_t mlx2 = { .addr = MLX2_ADDR, .reg = REG_TEMP_OBJ, .target_var = &temp_mlx2, .name = "MLX2 (Tanque)" };
    static mlx90614_sensor_t mlx3 = { .addr = MLX3_ADDR, .reg = REG_TEMP_OBJ, .target_var = &temp_mlx3, .name = "MLX3 (Ambiente)" };

    xTaskCreate(mlx90614_task, "MLX1_Task", 4096, &mlx1, 5, NULL);
    xTaskCreate(mlx90614_task, "MLX2_Task", 4096, &mlx2, 5, NULL);
    xTaskCreate(mlx90614_task, "MLX3_Task", 4096, &mlx3, 5, NULL);
}

void start_ads_tasks(void)
{
    static ads1115_channel_t ch0 = { .channel = 0, .target_var = &ads_ch0, .name = "AIN0" };
    static ads1115_channel_t ch1 = { .channel = 1, .target_var = &ads_ch1, .name = "AIN1" };
    static ads1115_channel_t ch2 = { .channel = 2, .target_var = &ads_ch2, .name = "AIN2" };
    static ads1115_channel_t ch3 = { .channel = 3, .target_var = &ads_ch3, .name = "AIN3" };

    xTaskCreate(ads1115_task, "ADS_AIN0", 4096, &ch0, 5, NULL);
    xTaskCreate(ads1115_task, "ADS_AIN1", 4096, &ch1, 5, NULL);
    xTaskCreate(ads1115_task, "ADS_AIN2", 4096, &ch2, 5, NULL);
    xTaskCreate(ads1115_task, "ADS_AIN3", 4096, &ch3, 5, NULL);
}
