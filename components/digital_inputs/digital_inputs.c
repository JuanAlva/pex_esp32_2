#include "digital_inputs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "DIGITAL_INPUTS"

// Tiempo mínimo para considerar estable un cambio (en ms)
#define DEBOUNCE_TIME_MS 50

// Estructura de datos del estado interno
typedef struct {
    gpio_num_t pin;
    int stable_state;
    int last_read;
    int counter;
} input_t;

// Pines de entrada (puedes modificarlos según tus necesidades)
static input_t inputs[] = {
    {INPUT1_GPIO, 0, 0, 0},
    {INPUT2_GPIO, 0, 0, 0},
    {INPUT3_GPIO, 0, 0, 0},
};

#define NUM_INPUTS (sizeof(inputs) / sizeof(inputs[0]))

// Configura las entradas
void digital_inputs_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,     // útil si los pulsadores van a GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << INPUT1_GPIO) | (1ULL << INPUT2_GPIO) | (1ULL << INPUT3_GPIO)
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Entradas digitales configuradas");
}

// Devuelve el estado estable de una entrada
int digital_input_get_state(int index)
{
    if (index < 0 || index >= NUM_INPUTS)
        return -1;
    return inputs[index].stable_state;
}

// Tarea que lee las entradas con anti-rebote
static void read_inputs_task(void *pvParameters)
{
    const TickType_t delay = pdMS_TO_TICKS(10);
    while (1) {
        for (int i = 0; i < NUM_INPUTS; i++) {
            int current_state = gpio_get_level(inputs[i].pin);

            if (current_state != inputs[i].last_read) {
                inputs[i].counter = 0;
                inputs[i].last_read = current_state;
            } else {
                if (inputs[i].counter * 10 >= DEBOUNCE_TIME_MS) {
                    if (inputs[i].stable_state != current_state) {
                        inputs[i].stable_state = current_state;
                        ESP_LOGI(TAG, "Entrada GPIO %d cambió a: %d", inputs[i].pin, current_state);
                    }
                } else {
                    inputs[i].counter++;
                }
            }
        }
        vTaskDelay(delay);
    }
}

// Crea la tarea de lectura
void digital_inputs_start_task(void)
{
    xTaskCreate(read_inputs_task, "read_inputs_task", 2048, NULL, 5, NULL);
}
