# VSDSquadron Mini UART Command Console – v1.0.0

## Summary

This project is a bare-metal firmware for the VSDSquadron Mini (WCH CH32V003F4U6, RISC-V) that turns the
board into a byte-oriented command console over UART. It consists of two small reusable drivers – a USART1
driver with interrupt-driven reception into a ring buffer, and a single-pin GPIO (LED) driver – plus an
application that consumes every received byte, logs it with a sequence number and hex value, and either
toggles an LED (`1`), prints a status report (`?`) or echoes the byte back. The application contains no
register or SDK peripheral access; all hardware interaction goes through the two driver APIs, so the drivers
can be dropped into another CH32V003 project unchanged. The firmware documented here is the Task-3
("Library + Application") solution located in [task3/submission](../../task3/submission).

## Target hardware

| Item | Value |
|---|---|
| Board | VSDSquadron Mini |
| MCU | WCH CH32V003F4U6 – 32-bit RISC-V (RV32EC, QingKe V2A core), 16 KB flash, 2 KB SRAM |
| Clock | Default SDK `SystemInit()` configuration (no clock changes made by this firmware) |
| Programmer / serial bridge | Onboard WCH-Link (USB-C): programs the MCU and exposes USART1 as a USB serial port |
| External parts | One LED + series resistor (330 Ω suggested) on header pin **PC6**, active-high |

## Supported drivers

| Driver | Status in v1.0.0 | Source | Scope |
|---|---|---|---|
| **UART** | Supported | [uart-lib.h](../../task3/submission/lib/uart-lib.h), [uart-lib.c](../../task3/submission/lib/uart-lib.c) | USART1, 8N1, configurable baud, blocking TX, interrupt RX into a 64-byte ring buffer, blocking and non-blocking read |
| **GPIO** (LED) | Supported | [led-lib.h](../../task3/submission/lib/led-lib.h), [led-lib.c](../../task3/submission/lib/led-lib.c) | One push-pull output on PC6: init, on, off, toggle, get |
| Timer | Not part of this release | – | The application is purely event-driven by UART input and needs no time base |
| PWM | Not part of this release | – | No analog/dimming requirement in this application |

Timer and PWM drivers exist in the separate Task-4 project and are intentionally out of scope for this
documented solution.

## Folder structure

```
vsdsquadron-mini-core/
├── task3/submission/              Firmware being documented (source of truth)
│   ├── platformio.ini             Build config: src_dir = app, compiles ../lib/*.c, -I lib
│   ├── app/
│   │   └── main.c                 Application: command dispatcher + RX/TX logger
│   └── lib/
│       ├── uart-lib.h / .c        UART driver (USART1 + RX ISR + ring buffer)
│       └── led-lib.h / .c         GPIO/LED driver (PC6)
└── task5/documentation/           This documentation set
    ├── README.md                  Entry point (this file)
    ├── API_REFERENCE.md           Every public driver function, Doxygen style
    ├── ARCHITECTURE.md            Layers, data/control flow, design rationale
    ├── APPLICATION_GUIDE.md       Application states, timing, edge cases
    ├── DEMO_GUIDE.md              Five-minute reproduction procedure for reviewers
    └── CHANGELOG.md               Version history
```

## Quick start (build + flash)

Prerequisite: [PlatformIO](https://platformio.org/) CLI or VS Code extension; the `ch32v` platform is
installed automatically on first build. Board connected over USB-C, LED wired to PC6.

```bash
cd vsdsquadron-mini-core/task3/submission   # 1. enter the firmware project
pio run                                      # 2. build
pio run -t upload                            # 3. flash through the onboard WCH-Link
pio device monitor -b 115200                 # 4. open the console (115200 8N1)
# 5. press RESET on the board, then type 1 (toggle LED) or ? (status)
```

## Where to go next

| If you want to… | Read |
|---|---|
| Call the drivers from your own code | [API_REFERENCE.md](API_REFERENCE.md) |
| Understand how the layers and the interrupt interact | [ARCHITECTURE.md](ARCHITECTURE.md) |
| Understand what the application does with each byte, and its limits | [APPLICATION_GUIDE.md](APPLICATION_GUIDE.md) |
| Reproduce and verify the demo | [DEMO_GUIDE.md](DEMO_GUIDE.md) |
| See what changed between versions | [CHANGELOG.md](CHANGELOG.md) |

## Resource usage (v1.0.0 build)

| Resource | Used | Available | Notes |
|---|---|---|---|
| Flash | 2900 B (17.7 %) | 16384 B | Includes SDK startup and peripheral library code |
| RAM | 372 B (18.2 %) | 2048 B | Of which 64 B is the UART RX ring buffer |
| Pins | PC6, PD5, PD6 | – | LED, USART1 TX, USART1 RX |
| Interrupts | `USART1_IRQn` | – | Preemption priority 1, sub-priority 1 |
