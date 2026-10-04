# Task 3 – UART Driver Library + Echo/Command Application (VSDSquadron Mini)

**Library implemented: Option-2 – UART driver library** (`lib/uart-lib.c` / `lib/uart-lib.h`).

A reusable USART1 driver for the CH32V003F4U6 on the VSDSquadron Mini with initialization, blocking
transmit, polled receive and interrupt-driven RX into a ring buffer. A demo application uses it to echo every
received character back to the PC, toggle an LED when `1` is received, and log every RX/TX event over UART.

## Folder contents

| Path | Purpose |
|---|---|
| [lib/uart-lib.c](lib/uart-lib.c) / [lib/uart-lib.h](lib/uart-lib.h) | **UART driver library** (the Task-3 deliverable) |
| [lib/led-lib.c](lib/led-lib.c) / [lib/led-lib.h](lib/led-lib.h) | Small LED helper for the demo, so `main.c` has no direct hardware access |
| [app/main.c](app/main.c) | Demo application – uses only `uart_*` and `led_*` functions |
| [platformio.ini](platformio.ini) | Build configuration for this folder layout |

## UART library API

| Function | Description |
|---|---|
| `void uart_init(uint32_t baud)` | Enables GPIOD/USART1 clocks, configures PD5 (TX, AF push-pull) and PD6 (RX, floating input), sets 8N1 at `baud`, enables the RXNE interrupt and NVIC, enables USART1 |
| `void uart_write_byte(uint8_t c)` | **Blocking transmit** of one byte (waits for TXE) |
| `void uart_write_string(const char *s)` | Blocking transmit of a C string; waits for transmission complete (TC) |
| `void uart_write_uint(uint32_t v)` | Transmit an unsigned number in decimal |
| `void uart_write_hex8(uint8_t v)` | Transmit a byte as two hex digits |
| `uint8_t uart_available(void)` | Returns `1` if at least one received byte is buffered |
| `uint8_t uart_try_read(uint8_t *c)` | **Non-blocking receive**: returns `1` and stores a byte, or `0` if none |
| `uint8_t uart_read_byte(void)` | **Blocking receive**: waits for and returns the next byte |

RX design: `USART1_IRQHandler` stores each received byte in a 64-byte ring buffer (`UART_RX_BUF_SIZE`).
The CH32V003 USART has a single-byte receive register and no FIFO, so buffering in the interrupt prevents
bytes from being lost (overrun) while the application is busy transmitting log lines.

LED helper API (demo support only): `led_init()`, `led_on()`, `led_off()`, `led_toggle()`, `led_get()` –
LED on **PC6**, push-pull, active-high.

## Demo application

[app/main.c](app/main.c) calls only library functions:

```c
led_init();
uart_init(115200);
while (1) {
    uint8_t c = uart_read_byte();       // RX
    ...                                 // log "[RX #n] 'c' 0xNN -> [TX] ..."
    if (c == '1') led_toggle();         // command
    else uart_write_byte(c);            // echo
}
```

Behaviour for every received byte:

| Received | Action | Example log line |
|---|---|---|
| `1` | Toggle the LED on PC6 | `[RX #5] '1' 0x31 -> [TX] LED toggled: ON` |
| `?` | Print status (LED state, byte count) and help | `[RX #6] '?' 0x3F -> [TX] status` |
| anything else | Echo the byte back | `[RX #1] 'h' 0x68 -> [TX] echo: h` |

Control characters are shown as `<CR>`, `<LF>` or `<?>` plus their hex value.

## Build and flash

Requirements: [PlatformIO](https://platformio.org/) (VS Code extension or CLI) with the `ch32v` platform;
board connected by USB-C (the onboard WCH-Link programs the chip and provides the USB-serial port).

```bash
cd vsdsquadron-mini-core/task3/submission
pio run                       # build
pio run -t upload             # flash via onboard WCH-Link
pio device monitor -b 115200  # open serial terminal
```

## UART configuration

| Setting | Value |
|---|---|
| Peripheral | USART1 – TX = **PD5**, RX = **PD6** |
| Baud rate | **115200** |
| Frame | 8 data bits, no parity, 1 stop bit (8N1), no flow control |
| Port | USB-serial of the onboard WCH-Link: `/dev/ttyACM0` (Linux) or `COMx` (Windows) |

## How to demo

1. Flash the firmware and open a terminal at 115200 8N1; reset the board. It prints:
   ```
   === Task3: UART driver library demo ===
   USART1 115200 8N1, TX=PD5 RX=PD6
   commands:
     1      toggle LED (PC6)
     ?      show status
     other  echoed back
   ```
2. Type `hello` – each character is received and echoed back with a log line.
3. Type `1` – the LED on PC6 toggles; type `1` again to toggle it back.
4. Type `?` – status shows the LED state and total bytes received.
