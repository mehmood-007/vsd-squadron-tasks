// Task 3 demo: UART echo + command interaction, using only uart-lib and led-lib.
#include <uart-lib.h>
#include <led-lib.h>

#define BAUD_RATE 115200

static void print_help(void){
    uart_write_string(
        "commands:\r\n"
        "  1      toggle LED (PC6)\r\n"
        "  ?      show status\r\n"
        "  other  echoed back\r\n");
}

static void print_status(uint32_t rx_count){
    uart_write_string("status: LED=");
    uart_write_string(led_get() ? "ON" : "OFF");
    uart_write_string(", bytes received=");
    uart_write_uint(rx_count);
    uart_write_string("\r\n");
}

static void print_char(uint8_t c){
    if (c == '\r')               uart_write_string("<CR>");
    else if (c == '\n')          uart_write_string("<LF>");
    else if (c >= 0x20 && c < 0x7F){
        uart_write_byte('\'');
        uart_write_byte(c);
        uart_write_byte('\'');
    } else {
        uart_write_string("<?>");
    }
}

int main(void){
    uint32_t rx_count = 0;

    led_init();
    uart_init(BAUD_RATE);

    uart_write_string("\r\n=== Task3: UART driver library demo ===\r\n");
    uart_write_string("USART1 115200 8N1, TX=PD5 RX=PD6\r\n");
    print_help();

    while (1){
        uint8_t c = uart_read_byte();
        rx_count++;

        uart_write_string("[RX #");
        uart_write_uint(rx_count);
        uart_write_string("] ");
        print_char(c);
        uart_write_string(" 0x");
        uart_write_hex8(c);
        uart_write_string(" -> [TX] ");

        if (c == '1'){
            led_toggle();
            uart_write_string("LED toggled: ");
            uart_write_string(led_get() ? "ON\r\n" : "OFF\r\n");
        } else if (c == '?'){
            uart_write_string("status\r\n");
            print_status(rx_count);
            print_help();
        } else {
            uart_write_string("echo: ");
            uart_write_byte(c);
            uart_write_string("\r\n");
        }
    }
}
