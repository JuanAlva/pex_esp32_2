#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>

#include "digital_inputs.h"
#include "ds18b20_driver.h"
#include "i2c_driver.h"   // Librería personalizada ads_read_channel y read_mlx90614
#include "mlx90614_driver.h"
#include "ads1115_driver.h"
#include "uart_driver.h"

static const char *TAG = "MAIN";

// ====== Variables globales ======
float temp1 = 0, temp2 = 0, temp3 = 0;
float v0 = 0, v1 = 0, v2 = 0, v3 = 0;

// Semáforo global para proteger el bus I2C
SemaphoreHandle_t i2c_mutex = NULL;

// ====== MLX90614 con protección I2C ======
void mlx90614_task_safe(void *pvParameter)
{
    mlx90614_t *sensor = (mlx90614_t *)pvParameter;

    while (1)
    {
        // Tomar control del bus
        if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            *(sensor->temperature_var) =  read_mlx90614(sensor->addr);
            xSemaphoreGive(i2c_mutex); // Liberar bus
        }

        //ESP_LOGI(sensor->tag, "Temp = %.2f °C", *(sensor->temperature_var));
        vTaskDelay(pdMS_TO_TICKS(500)); // tiempo entre lecturas
    }
}

// ====== ADS1115 con protección I2C ======
void ads1115_task_safe(void *pvParameter)
{
    while (1)
    {
        if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            v0 = ads_read_channel(0);
            vTaskDelay(pdMS_TO_TICKS(20));
            v1 = ads_read_channel(1);
            vTaskDelay(pdMS_TO_TICKS(20));
            v2 = ads_read_channel(2);
            vTaskDelay(pdMS_TO_TICKS(20));
            v3 = ads_read_channel(3);
            xSemaphoreGive(i2c_mutex);
        }

        //ESP_LOGI("ADS1115", "AIN0=%.3f | AIN1=%.3f | AIN2=%.3f | AIN3=%.3f", v0, v1, v2, v3);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ====== Tarea que envía los datos en formato JSON por UART ======
void uart_json_task(void *pvParameter)
{
    char json_buffer[256];

    while (1)
    {
        int s1 = digital_input_get_state(0);
        int s2 = digital_input_get_state(1);
        int s3 = digital_input_get_state(2);

        // Crear cadena JSON
        snprintf(json_buffer, sizeof(json_buffer),
                 "{\"temp1\":%.2f,\"temp2\":%.2f,\"temp3\":%.2f,"
                 "\"v0\":%.3f,\"v1\":%.3f,\"v2\":%.3f,\"v3\":%.3f,"
                 "\"inputs\":[%d,%d,%d]}",
                 temp1, temp2, temp3, v0, v1, v2, v3, s1, s2, s3);

        // Enviar JSON por UART
        uart_send_string(json_buffer);
        uart_send_string("\n");  // Importante: cada JSON en una línea

        vTaskDelay(pdMS_TO_TICKS(1000)); // Periodo de envío
    }
}

void app_main()
{
    ESP_ERROR_CHECK(i2c_master_init());
    // Crear semáforo mutex
    i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL)
    {
        ESP_LOGE(TAG, "Error al crear el semáforo I2C");
        return;
    }

    // ====== UART ======
    uart_driver_init();  // Inicialización
    //xTaskCreate(uart_send_task, "UART_SEND", 4096, NULL, 4, NULL);

    // ====== MLX90614 ======
    static mlx90614_t mlx_sensors[] = {
        {.addr = 0x5A, .temperature_var = &temp1, .tag = "MLX_1"},
        {.addr = 0x5B, .temperature_var = &temp2, .tag = "MLX_2"},
        {.addr = 0x5C, .temperature_var = &temp3, .tag = "MLX_3"}
    };

    for (int i = 0; i < 3; i++)
    {
        xTaskCreate(mlx90614_task_safe, mlx_sensors[i].tag, 4096, &mlx_sensors[i], 5, NULL);
    }

    // ====== ADS1115 ======
    xTaskCreate(ads1115_task_safe, "ADS_TASK", 4096, NULL, 5, NULL);

    // ====== Entradas digitales ======
    digital_inputs_init();
    digital_inputs_start_task();

    // ====== DS18B20 ======
    ds18b20_sensor_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    ds18b20_sensor_start_all();
    
    // ====== Envío JSON UART ======
    xTaskCreate(uart_json_task, "UART_JSON", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "Sistema iniciado correctamente");
}