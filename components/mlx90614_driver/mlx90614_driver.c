#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "i2c_driver.h"

#include "mlx90614_driver.h"

static const char *TAG = "MLX90614";

// #define I2C_SLAVE_ADDR 0x68
#define MLX1_ADDR 0x5A
#define MLX2_ADDR 0x5B
#define MLX3_ADDR 0x5C

#define REG_TEMP_AMB 0x06 // Registro temperatura objeto 1
#define REG_TEMP_OBJ 0x07

#define MLX90614_SCALE 0.02f  // Factor de conversión (0.02 °K por bit)
#define MLX90614_OFFSET 273.15f // Para convertir K a °C

float read_mlx90614(uint8_t addr)
{
    uint8_t reg = REG_TEMP_OBJ;
    uint8_t data[3] = {0};

    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM,
                                                 addr,
                                                 &reg,
                                                 1,
                                                 data,
                                                 3,
                                                 pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Error I2C addr=0x%02X", addr);
        return -1000.0f; // Valor de error
    }

    uint16_t raw = ((uint16_t)data[1] << 8) | data[0];
    float tempC = (raw * MLX90614_SCALE) - MLX90614_OFFSET;
    return tempC;
}

void mlx90614_task(void *pvParameter)
{
    mlx90614_t *sensor = (mlx90614_t *)pvParameter;
    float temp;

    while (1)
    {
        float temp = read_mlx90614(sensor->addr);
        *(sensor->temperature_var) = temp;

        ESP_LOGI(sensor->tag, "Temp = %.2f °C", temp);
        vTaskDelay(pdMS_TO_TICKS(500));  // Lectura cada 0.5 s
    }
}
