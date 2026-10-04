# Evidence – Task 3 (UART Driver Library)

## 1. UART evidence

<!-- TODO: add a screenshot of the terminal (e.g. uart_log.png) and paste the captured log (10+ lines). -->

![UART terminal output](uart_log.png)

Captured log (115200 8N1):

```
TODO: paste real terminal output here (boot banner + at least 10 RX/TX lines),
e.g. after typing: hello, 1, 1, ?
```

## 2. Hardware evidence

<!-- TODO: add a photo or video showing the board, the LED on PC6 and the terminal. -->

| What | File |
|---|---|
| Board with LED on PC6 OFF, after an odd number of `1` presses → ON | TODO |
| Video: typing `1` toggles the LED while the terminal shows the log | TODO |

## 3. Explanation

### How the application uses the library

- `app/main.c` contains no register access and no SDK peripheral calls. It only calls the UART library
  (`uart_init`, `uart_read_byte`, `uart_write_byte`, `uart_write_string`, `uart_write_uint`,
  `uart_write_hex8`) and the LED helper (`led_init`, `led_toggle`, `led_get`).
- `uart_init(115200)` configures PD5/PD6, USART1 (8N1) and the RX interrupt.
- Each received byte is stored in the library's ring buffer by `USART1_IRQHandler`. The application
  fetches it with `uart_read_byte()`, logs it as `[RX #n] 'c' 0xNN`, and then:
  - `1` → `led_toggle()` and logs the new LED state,
  - `?` → prints status and help,
  - anything else → echoes the byte back with `uart_write_byte()`.

### What was verified on hardware

<!-- TODO: tick each item after checking it on the board. -->

- [ ] Boot banner and help text appear at 115200 8N1
- [ ] Typed characters are received and echoed back (`[RX #n] ... -> [TX] echo: ...`)
- [ ] Sending `1` toggles the LED on PC6 and the log shows `LED toggled: ON/OFF`
- [ ] Sending `?` reports the correct LED state and byte count
- [ ] Pasting a long line at once is fully received (no lost characters, thanks to interrupt RX buffering)

## 4. Build

Built with `pio run` from `task3/submission/`:

```
Compiling .../lib/led-lib.o
Compiling .../lib/uart-lib.o
Compiling .../src/main.o
RAM:   [==        ]  18.2% (used 372 bytes from 2048 bytes)
Flash: [==        ]  17.7% (used 2900 bytes from 16384 bytes)
========================= [SUCCESS] Took 2.38 seconds =========================
```
