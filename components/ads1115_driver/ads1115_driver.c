#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_driver.h"
#include "ads1115_driver.h"

static const char *TAG = "ADS1115";

#define ADS_ADDR            0x48
#define ADS_REG_CONVERSION  0x00
#define ADS_REG_CONFIG      0x01

// configuracion de ads, bits de registro de config ads
#define ADS_OS          0x01       // 15
#define ADS_MUX         0x00      // 14:12
#define ADS_PGA         0x01      // 11:9 // b001 4.096 v // b010 2.048 v (default)
#define ADS_MODE        0x01     // 8
#define ADS_DR          0x04       // 7:5 // b001 16 SPS // b100 128 SPS (default)
#define ADS_COMP_DES    0x03 // 4.3.2.1:0

#define ADS1115_CONFIG_MUX_AIN0 0x04
#define ADS1115_CONFIG_MUX_AIN1 0x05
#define ADS1115_CONFIG_MUX_AIN2 0x06
#define ADS1115_CONFIG_MUX_AIN3 0x07

// Factor de conversión a voltaje
#define ADS1115_LSB_4_096V (4.096 / 32768.0)

static uint16_t channel_mux[] = {
    ADS1115_CONFIG_MUX_AIN0,
    ADS1115_CONFIG_MUX_AIN1,
    ADS1115_CONFIG_MUX_AIN2,
    ADS1115_CONFIG_MUX_AIN3};

float ads_read_channel(uint8_t channel)
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
    config |= (ADS_OS & 0x01) << 15;
    // Bits 14-12 -> MUX (entrada)
    config |= (channel_mux[channel] & 0x07) << 12;
    // Bits 11-9 -> PGA
    config |= (ADS_PGA & 0x07) << 9;
    // Bit 8 -> MODE
    config |= (ADS_MODE & 0x01) << 8;
    // Bits 7-5 -> DR
    config |= (ADS_DR & 0x07) << 5;
    // Bits 4-0 -> deshabilitar comparador (COMP_QUE = 11)
    config |= ADS_COMP_DES;

    uint8_t write_buf[3];
    write_buf[0] = ADS_REG_CONFIG; // revisar por que
    write_buf[1] = (config >> 8) & 0xFF;
    write_buf[2] = config & 0xFF;

    // Escribir configuración
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM,
                                               ADS_ADDR,
                                               write_buf,
                                               3,
                                               pdMS_TO_TICKS(100)); // I2C_TIMEOUT_MS / portTICK_PERIOD_MS

    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Error escribiendo config canal %d", channel);
        return -1.0f;
    }

    // Tiempo de conversión (para 128 SPS -> ~8 ms)
    vTaskDelay(pdMS_TO_TICKS(10));

    // Leer resultado
    uint8_t reg = ADS_REG_CONVERSION;
    uint8_t data[2] = {0};
    ret = i2c_master_write_read_device(I2C_MASTER_NUM,
                                       ADS_ADDR,
                                       &reg,
                                       1,
                                       data,
                                       2,
                                       I2C_TIMEOUT_MS / portTICK_PERIOD_MS);

    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "Error leyendo canal %d", channel);
        return -1.0f;
    }

    int16_t raw = ((int16_t)data[0] << 8) | data[1];
    float voltage = raw * ADS1115_LSB_4_096V;

    return voltage;
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
// void ads1115_task(void *pvParameter)
// {
//     ads1115_channel_t *ch = (ads1115_channel_t *)pvParameter;
//     while (1)
//     {
//         int16_t raw = ads_read_channel(ch->channel);
//         float voltage = (raw * 4.096f) / 32768.0f;
//         *(ch->target_var) = voltage;
//         ESP_LOGI(TAG_ADS, "%s -> %.3f V", ch->name, voltage);
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

// void start_mlx_tasks(void)
// {
//     static ads1115_channel_t ch0 = {.channel = 0, .target_var = &ads_ch0, .name = "AIN0"};
//     static ads1115_channel_t ch1 = {.channel = 1, .target_var = &ads_ch1, .name = "AIN1"};
//     static ads1115_channel_t ch2 = {.channel = 2, .target_var = &ads_ch2, .name = "AIN2"};
//     static ads1115_channel_t ch3 = {.channel = 3, .target_var = &ads_ch3, .name = "AIN3"};

//     xTaskCreate(ads1115_task, "ADS_AIN0", 4096, &ch0, 5, NULL);
//     xTaskCreate(ads1115_task, "ADS_AIN1", 4096, &ch1, 5, NULL);
//     xTaskCreate(ads1115_task, "ADS_AIN2", 4096, &ch2, 5, NULL);
//     xTaskCreate(ads1115_task, "ADS_AIN3", 4096, &ch3, 5, NULL);
// }

// ====== Tarea ejemplo de lectura ======
// La tarea con el semaforo se encuentra en app_main()
void ads1115_task(void *pvParameter)
{
    float v0, v1, v2, v3;

    while (1)
    {
        v0 = ads_read_channel(0);
        vTaskDelay(pdMS_TO_TICKS(50));
        v1 = ads_read_channel(1);
        vTaskDelay(pdMS_TO_TICKS(50));
        v2 = ads_read_channel(2);
        vTaskDelay(pdMS_TO_TICKS(50));
        v3 = ads_read_channel(3);

        ESP_LOGI(TAG, "AIN0=%.3f | AIN1=%.3f | AIN2=%.3f | AIN3=%.3f",
                 v0, v1, v2, v3);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}