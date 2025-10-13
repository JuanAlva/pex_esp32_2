#include "mlx90614_driver.h"
#include "esp_log.h"

static const char *TAG_MLX = "MLX90614";

esp_err_t mlx90614_read_temp(uint8_t addr, float *temp)
{
    uint8_t reg = REG_TEMP_OBJ;
    uint8_t data[3];
    esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, addr, &reg, 1, data, 3, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (err != ESP_OK) return err;

    uint16_t raw = (data[1] << 8) | data[0];
    *temp = (raw * 0.02) - 273.15;
    return ESP_OK;
}

void mlx90614_task(void *pvParameter)
{
    mlx90614_t *sensor = (mlx90614_t *)pvParameter;
    float temp;

    while (1)
    {
        if (mlx90614_read_temp(sensor->addr, &temp) == ESP_OK) {
            *(sensor->temperature_var) = temp;
            ESP_LOGI(sensor->tag, "T = %.2f °C", temp);
        } else {
            ESP_LOGE(sensor->tag, "Error al leer MLX90614");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
