#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>

#include "digital_inputs.h"
#include "ds18b20_driver.h"
#include "i2c_driver.h"   // Librería personalizada donde defines ads_read_channel y read_mlx90614
#include "mlx90614_driver.h"
#include "ads1115_driver.h"

float temp1, temp2, temp3;
float v0, v1, v2, v3;

void app_main()
{
    ESP_ERROR_CHECK(i2c_master_init());
    
    start_mlx_tasks(); 

     // ====== MLX90614 ======
    static mlx90614_t mlx_sensors[] = {
        { .addr = 0x5A, .temperature_var = &temp1, .tag = "MLX_1" },
        { .addr = 0x5B, .temperature_var = &temp2, .tag = "MLX_2" },
        { .addr = 0x5C, .temperature_var = &temp3, .tag = "MLX_3" }
    };

    for (int i = 0; i < 3; i++) {
        xTaskCreate(mlx90614_task, mlx_sensors[i].tag, 4096, &mlx_sensors[i], 5, NULL);
    }

        // ====== ADS1115 ======
    // static ads1115_channel_t ads_channels[] = {
    //     { .channel = 0, .voltage_var = &v0, .tag = "ADS_CH0" },
    //     { .channel = 1, .voltage_var = &v1, .tag = "ADS_CH1" },
    //     { .channel = 2, .voltage_var = &v2, .tag = "ADS_CH2" },
    //     { .channel = 3, .voltage_var = &v3, .tag = "ADS_CH3" }
    // };

    // for (int i = 0; i < 4; i++) {
    //     xTaskCreate(ads1115_task, ads_channels[i].tag, 4096, &ads_channels[i], 5, NULL);
    // }

    

    // Inicializar entradas digitales
    digital_inputs_init();
    digital_inputs_start_task();
    
    ds18b20_sensor_init();
    ds18b20_sensor_start_all();
    
    while (1) {
        int s1 = digital_input_get_state(0);
        int s2 = digital_input_get_state(1);
        int s3 = digital_input_get_state(2);

        ESP_LOGI("MAIN", "Entradas digitales: [%d, %d, %d]", s1, s2, s3);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}