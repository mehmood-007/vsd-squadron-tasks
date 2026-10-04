// Task 2: board bring-up demo. Application logic only; hardware access goes through gpio.h / uart.h.
#include <debug.h>
#include "gpio.h"
#include "uart.h"

#define BOARD_NAME       "VSDSquadron Mini (CH32V003F4U6)"
#define FIRMWARE_VERSION "v1.0.0"
#define UART_BAUD        115200
#define BLINK_PERIOD_MS  500

int main(void){
    uint32_t count = 0;

    SystemCoreClockUpdate();
    Delay_Init();
    uart_init(UART_BAUD);
    gpio_init();

    uart_write_string("\r\n=== Task 2: board bring-up ===\r\n");
    uart_write_string("Board:    " BOARD_NAME "\r\n");
    uart_write_string("Firmware: " FIRMWARE_VERSION "\r\n");
    uart_write_string("UART:     USART1 115200 8N1 (TX=PD5, RX=PD6)\r\n");
    uart_write_string("GPIO:     " LED_GPIO_NAME " (port C, pin 6) push-pull output, toggled every 500 ms\r\n");

    while (1){
        gpio_toggle();
        count++;

        uart_write_string("[");
        uart_write_uint(count);
        uart_write_string("] t=");
        uart_write_uint(count * BLINK_PERIOD_MS);
        uart_write_string(" ms  " LED_GPIO_NAME "=");
        uart_write_string(gpio_get() ? "HIGH (LED ON)\r\n" : "LOW  (LED OFF)\r\n");

        Delay_Ms(BLINK_PERIOD_MS);
    }
}
