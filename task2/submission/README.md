# Task 2 – Board Bring-Up: UART + GPIO on the VSDSquadron Mini

## What was implemented

Minimal bare-metal firmware for the VSDSquadron Mini (CH32V003F4U6, RISC-V RV32EC, WCH NoneOS SDK) that:

1. Boots after flashing and prints a startup banner over UART (board name, firmware version, UART and GPIO setup).
2. Configures one physical GPIO pin (**PC6**) as a push-pull output and toggles it every 500 ms, driving an LED.
3. Prints one numbered log line per toggle (counter, elapsed time, pin level), so the UART output grows continuously.

The firmware is split into an application and two small driver layers. `main.c` contains no register access.
It only calls the GPIO and UART APIs plus the SDK clock and delay helpers (`SystemCoreClockUpdate`, `Delay_Init`, `Delay_Ms`).

| File | Content |
|---|---|
| [main.c](main.c) | Application logic only: banner, counter, toggle loop |
| [gpio.h](gpio.h) / [gpio.c](gpio.c) | GPIO API layer and the datasheet-based pin definition |
| [uart.h](uart.h) / [uart.c](uart.c) | UART API layer (USART1) |
| [platformio.ini](platformio.ini) | Build configuration for this folder |

### GPIO API

| Function | Description |
|---|---|
| `gpio_init()` | Enable the GPIOC clock and configure PC6 as a 50 MHz push-pull output, initially LOW |
| `gpio_set()` | Drive PC6 HIGH (LED on) |
| `gpio_clear()` | Drive PC6 LOW (LED off) |
| `gpio_toggle()` | Invert the current output level |
| `gpio_get()` | Return the level currently driven on PC6 (1 = HIGH) |

### UART API

| Function | Description |
|---|---|
| `uart_init(baud)` | Configure PD5 (TX) / PD6 (RX) and USART1, 8N1 |
| `uart_write_char(c)` | Blocking transmit of one character |
| `uart_write_string(s)` | Transmit a string and wait for transmission complete |
| `uart_write_uint(v)` | Transmit an unsigned decimal number |

## GPIO pin chosen and why

| Item | Value | Source |
|---|---|---|
| Physical pin label (silkscreen / header) | **PC6**, header **J4 pin 7** (net `Nano_PC6`) | VSDSquadron Mini schematic `SquadronMini_2A` Rev V3 |
| Firmware GPIO | **Port C, pin 6**: `GPIOC` + `GPIO_Pin_6` (`RCC_APB2Periph_GPIOC`) | CH32V003 datasheet, pin **13** of CH32V003F4U6 (QFN20): `PC6/MOSI/T1CH1CH3N/UCTS/SDA_` |
| Mode | Push-pull output, active-high LED (PC6 → resistor → LED → GND) | |

Why PC6:
- It is a plain GPIO broken out on the header and is not used by UART, programming or reset.
- The onboard user LED (L1) is on **PD6**, which is also the **USART1 RX** pin. Toggling it would interfere with UART,
  so PC6 with an external LED was chosen instead.
- PD1 (SWIO, programming), PD5/PD6 (UART) and PD7 (NRST) are avoided on purpose.

The CH32V003 names its GPIOs by port and pin (PA1–PA2, PC0–PC7, PD0–PD7) rather than by a single number.
The firmware therefore uses the SDK constants `GPIOC` / `GPIO_Pin_6`, which correspond directly to the datasheet name **PC6**.

## UART message description

USART1, **115200 baud, 8N1**, TX = PD5, RX = PD6. The board's onboard WCH-Link bridges it to USB as `/dev/ttyACM0` (Linux) or `COMx` (Windows).

After reset:

```
=== Task 2: board bring-up ===
Board:    VSDSquadron Mini (CH32V003F4U6)
Firmware: v1.0.0
UART:     USART1 115200 8N1 (TX=PD5, RX=PD6)
GPIO:     PC6 (port C, pin 6) push-pull output, toggled every 500 ms
[1] t=500 ms  PC6=HIGH (LED ON)
[2] t=1000 ms  PC6=LOW  (LED OFF)
[3] t=1500 ms  PC6=HIGH (LED ON)
...
```

Each line shows an increasing counter, the elapsed time (counter × 500 ms) and the level read back from the pin,
so every UART line can be matched to the LED state.

## How to build and flash

Requirements: PlatformIO (VS Code extension or CLI) with the `ch32v` platform. Connect the board over USB-C.

```bash
cd vsdsquadron-mini-core/task2/submission
pio run                       # build
pio run -t upload             # flash through the onboard WCH-Link
pio device monitor -b 115200  # view UART output, then press the reset button (SW3)
```

Wiring for the GPIO demo: PC6 (J4 pin 7) → 1 kΩ resistor → LED anode, LED cathode → GND.
A multimeter or logic analyser on PC6 shows the same toggling.
