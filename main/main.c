#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>

#include "ds18b20_driver.h"
#include "i2c_driver.h"   // Librería personalizada donde defines ads_read_channel y read_mlx90614
#include "digital_inputs.h"

void app_main()
{
    ESP_ERROR_CHECK(i2c_master_init());
    i2c_ads_start_task(); 
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