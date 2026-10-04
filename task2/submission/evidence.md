# Evidence – Task 2 (Board Bring-Up)

## 1. UART evidence

<!-- TODO: add a screenshot (uart_output.png) and a short video (uart_output.mp4) of the terminal after reset. -->

![UART output after reset](uart_output.png)

Video: [uart_output.mp4](uart_output.mp4)

Captured log (115200 8N1, 10+ lines):

```
TODO: paste the real terminal output here (banner + at least 10 counter lines)
```

## 2. GPIO evidence

<!-- TODO: add a photo and a short video showing the board, the wiring on PC6 and the LED blinking. -->

![Board with LED on PC6](gpio_board.jpg)

Video: [gpio_blink.mp4](gpio_blink.mp4)

| Item | Value |
|---|---|
| Physical pin label | **PC6** (header J4, pin 7, net `Nano_PC6`) |
| Firmware GPIO | **GPIOC, GPIO_Pin_6** (CH32V003F4U6 pin 13, `PC6`) |
| Output | Push-pull, toggled every 500 ms; HIGH = LED on |

## 3. Short explanation – how correct behavior was verified

<!-- TODO: confirm each item on the board, then tick it. -->

- [ ] Firmware flashed with `pio run -t upload` and the board boots after reset (banner appears).
- [ ] Banner shows the board name and firmware version `v1.0.0`.
- [ ] The counter increases by 1 on every line, and the time increases by 500 ms per line.
- [ ] The LED on PC6 changes state at the same moment a new UART line appears.
- [ ] `PC6=HIGH (LED ON)` lines match the LED being lit; `PC6=LOW (LED OFF)` lines match it being dark.
- [ ] Disconnecting the LED wire from J4 pin 7 stops the blinking, so the toggling comes from that exact pin.

## 4. Build

`pio run` in `task2/submission/`:

```
Compiling .../src/gpio.o
Compiling .../src/main.o
Compiling .../src/uart.o
RAM:   [==        ]  15.4% (used 316 bytes from 2048 bytes)
Flash: [==        ]  15.0% (used 2460 bytes from 16384 bytes)
========================= [SUCCESS] Took 2.58 seconds =========================
```
