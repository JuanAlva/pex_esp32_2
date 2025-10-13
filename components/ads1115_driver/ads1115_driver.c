#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_driver.h"

#include "ads1115_driver.h"

static const char *TAG_ADS = "ADS1115";

float ads_ch0 = 0.0f;
float ads_ch1 = 0.0f;
float ads_ch2 = 0.0f;
float ads_ch3 = 0.0f;

typedef struct {
    uint8_t channel;
    float *target_var;
    const char *name;
} ads1115_channel_t;

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

// float ads1115_read_voltage(uint8_t channel)
// {
//     int16_t raw = ads1115_read_raw(channel);
//     return (raw * 4.096) / 32768.0;
// }

// void ads1115_task(void *pvParameter)
// {
//     ads1115_channel_t *ch = (ads1115_channel_t *)pvParameter;
//     float voltage;

//     while (1)
//     {
//         voltage = ads1115_read_voltage(ch->channel);
//         *(ch->voltage_var) = voltage;
//         ESP_LOGI(ch->tag, "Canal %d -> %.3f V", ch->channel, voltage);

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

// Tarea genérica ADS1115
void ads1115_task(void *pvParameter)
{
    ads1115_channel_t *ch = (ads1115_channel_t *) pvParameter;
    while (1)
    {
        int16_t raw = ads_read_channel(ch->channel);
        float voltage = (raw * 4.096f) / 32768.0f;
        *(ch->target_var) = voltage;
        ESP_LOGI(TAG_ADS, "%s -> %.3f V", ch->name, voltage);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void start_mlx_tasks(void)
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