#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "uart_driver.h"

#include "digital_inputs.h"
#include "ds18b20_driver.h"
#include "i2c_driver.h"   // Librería personalizada ads_read_channel y read_mlx90614
#include "mlx90614_driver.h"
#include "ads1115_driver.h"
#include "uart_driver.h"

// ====== Configuración UART ======
#define UART_PORT       UART_NUM_2      // Puedes usar UART0 o UART1
#define UART_TX_PIN     17
#define UART_RX_PIN     16
#define UART_BAUD_RATE  115200

static const char *TAG = "UART_DRIVER";

void uart_driver_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Configurar el UART
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 2048, 0, 0, NULL, 0));

    ESP_LOGI(TAG, "UART inicializado en TX=%d, RX=%d, Baudrate=%d",
             UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
}

// ====== Enviar cadena ======
void uart_send_string(const char *str)
{
    if (str == NULL) return;
    uart_write_bytes(UART_PORT, str, strlen(str));
}
