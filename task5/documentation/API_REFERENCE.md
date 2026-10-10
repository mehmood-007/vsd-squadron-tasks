# API Reference

This document describes every public function exported by the two drivers of the VSDSquadron Mini UART
Command Console (v1.0.0). Functions are presented in Doxygen style; the comment blocks below can be copied
into the headers if HTML generation is desired later.

| Module | Header | Implementation |
|---|---|---|
| UART driver | [uart-lib.h](../../task3/submission/lib/uart-lib.h) | [uart-lib.c](../../task3/submission/lib/uart-lib.c) |
| LED (GPIO) driver | [led-lib.h](../../task3/submission/lib/led-lib.h) | [led-lib.c](../../task3/submission/lib/led-lib.c) |

General conventions:

- All functions are synchronous C functions with no dynamic memory allocation.
- No function reports errors through a return code; the drivers have no failure path that the hardware can
  report at runtime (see *Constraints* for each function for misuse that is **not** detected).
- Boolean results are returned as `uint8_t` with values `0` (false) or `1` (true).
- None of the functions are designed to be called from interrupt context.

---

## 1. UART driver (`uart-lib`)

### 1.1 Hardware binding and configuration

| Item | Value | Changeable? |
|---|---|---|
| Peripheral | USART1 | No (hard-coded) |
| TX pin | PD5, alternate-function push-pull | No |
| RX pin | PD6, floating input | No |
| Frame format | 8 data bits, no parity, 1 stop bit, no flow control | No |
| Baud rate | Argument of `uart_init()` | Yes, at runtime |
| RX buffer size | `UART_RX_BUF_SIZE` = 64 bytes (63 usable) | Yes, at compile time |
| Interrupt | `USART1_IRQn`, RXNE only, preemption priority 1, sub-priority 1 | No |

#### `UART_RX_BUF_SIZE`

```c
/**
 * @brief Capacity of the interrupt-fed receive ring buffer, in bytes.
 *
 * @note Must be a power of two (index wrap-around uses a bit mask, not a modulo).
 *       Must be <= 256 because head/tail indices are uint8_t.
 *       One slot is always kept empty to distinguish "full" from "empty",
 *       so the usable capacity is UART_RX_BUF_SIZE - 1.
 *       No compile-time check enforces these rules.
 */
#define UART_RX_BUF_SIZE 64
```

---

### 1.2 `uart_init`

```c
/**
 * @brief  Initialize USART1 for 8N1 operation with interrupt-driven reception.
 *
 * Enables the GPIOD and USART1 clocks, configures PD5 as TX and PD6 as RX,
 * programs the baud rate and frame format, clears the RX ring buffer,
 * enables the RXNE interrupt in USART1 and in the interrupt controller,
 * and finally enables the USART.
 *
 * @param  baud  Baud rate in bits per second (e.g. 9600, 115200).
 * @return None.
 */
void uart_init(uint32_t baud);
```

| | |
|---|---|
| **Purpose** | One-time bring-up of the serial link; must precede every other `uart_*` call. |
| **Parameters** | `baud` – desired bit rate. The SDK derives the divider from the current APB clock, so the result is correct for whatever clock `SystemInit()` configured. |
| **Returns** | Nothing. |
| **Side effects** | Sets the SDK interrupt priority grouping to `NVIC_PriorityGroup_1` (a **global** setting). Calling it again re-initializes the peripheral and **discards** any unread received bytes. |

**Constraints / notes**

- `baud` is not validated. `0` or a value the APB clock cannot divide down to accurately produces a wrong
  bit rate without any error indication.
- If another module in the system uses a different priority grouping, initialize it consistently; the last
  call to `NVIC_PriorityGroupConfig()` wins.
- Global interrupts must be enabled (the SDK startup code enables them before `main()`), otherwise no byte
  is ever received.

**Example**

```c
#include <uart-lib.h>

int main(void) {
    uart_init(115200);
    uart_write_string("ready\r\n");
    for (;;) { }
}
```

---

### 1.3 `uart_write_byte`

```c
/**
 * @brief  Transmit one byte, blocking until the transmit data register is free.
 *
 * @param  c  Byte to send (any value 0x00–0xFF; no translation is applied).
 * @return None.
 */
void uart_write_byte(uint8_t c);
```

| | |
|---|---|
| **Purpose** | Lowest-level transmit primitive used by every other write function. |
| **Parameters** | `c` – raw byte. `'\n'` is **not** expanded to `"\r\n"`. |
| **Returns** | Nothing. Returns as soon as the byte is handed to the USART (TXE flag), **not** when it has left the pin. |

**Constraints / notes**

- Blocks for up to one character time (≈ 86.8 µs at 115200 8N1) if a previous byte is still being shifted out.
- Because it returns before the stop bit is sent, do not reset the MCU, change the baud rate or enter a low
  power mode immediately after it. Use `uart_write_string()` (which waits for transmission complete) as a
  flush, e.g. `uart_write_string("")`.

**Example**

```c
uart_write_byte('A');
uart_write_byte(0x0D);   /* carriage return */
uart_write_byte(0x0A);   /* line feed */
```

---

### 1.4 `uart_write_string`

```c
/**
 * @brief  Transmit a NUL-terminated string and wait until the last bit has left the pin.
 *
 * @param  s  Pointer to a NUL-terminated string. Must not be NULL.
 * @return None.
 */
void uart_write_string(const char *s);
```

| | |
|---|---|
| **Purpose** | Send text messages; also acts as a TX **flush** because it waits for the transmission-complete (TC) flag. |
| **Parameters** | `s` – string to send; the terminating `'\0'` is not transmitted. |
| **Returns** | Nothing. On return the USART transmitter is idle. |

**Constraints / notes**

- `s == NULL` is not checked and results in undefined behaviour.
- Blocking time ≈ `strlen(s) × 86.8 µs` at 115200 baud (e.g. a 40-character line ≈ 3.5 ms).
- Line endings are sent exactly as written; use `"\r\n"` for terminal output.
- Received bytes are not lost while this function blocks, because reception is handled by the interrupt.

**Example**

```c
uart_write_string("temperature=");
uart_write_uint(23);
uart_write_string(" C\r\n");
```

---

### 1.5 `uart_write_uint`

```c
/**
 * @brief  Transmit an unsigned 32-bit integer as decimal ASCII text.
 *
 * @param  v  Value to print (0 … 4294967295).
 * @return None.
 */
void uart_write_uint(uint32_t v);
```

| | |
|---|---|
| **Purpose** | Print counters and measurements without pulling in `printf` (saves several KB of flash). |
| **Parameters** | `v` – value. `0` prints `"0"`. |
| **Returns** | Nothing. |

**Constraints / notes**

- No sign, padding, or thousands separator. For signed values, print `'-'` and the magnitude yourself.
- Uses a 10-byte stack buffer, which exactly fits the largest `uint32_t` value (10 digits).
- The CH32V003 core has no hardware divider (RV32EC); `/ 10` and `% 10` are performed by software routines.
  This is negligible next to the UART transmission time.
- Does **not** wait for transmission complete (unlike `uart_write_string`).

**Example**

```c
uart_write_string("count=");
uart_write_uint(1234u);      /* sends "1234" */
uart_write_string("\r\n");
```

---

### 1.6 `uart_write_hex8`

```c
/**
 * @brief  Transmit one byte as exactly two upper-case hexadecimal digits.
 *
 * @param  v  Byte to print.
 * @return None.
 */
void uart_write_hex8(uint8_t v);
```

| | |
|---|---|
| **Purpose** | Show raw byte values (register contents, received characters) unambiguously. |
| **Parameters** | `v` – value to print; always two digits, leading zero kept (`0x0A` → `"0A"`). |
| **Returns** | Nothing. |

**Constraints / notes**

- No `0x` prefix is added; send it explicitly if wanted.
- Does not wait for transmission complete.

**Example**

```c
uart_write_string("0x");
uart_write_hex8(0x3F);       /* sends "0x3F" */
```

---

### 1.7 `uart_available`

```c
/**
 * @brief  Check whether at least one received byte is waiting in the RX buffer.
 *
 * @return 1 if one or more bytes are buffered, 0 if the buffer is empty.
 */
uint8_t uart_available(void);
```

| | |
|---|---|
| **Purpose** | Cheap, non-blocking poll for incoming data inside a super-loop. |
| **Parameters** | None. |
| **Returns** | `1` / `0`. It is **not** a byte count. |

**Constraints / notes**

- The result can become stale immediately: the ISR may add a byte right after the check. A `0` result means
  "nothing yet", never "nothing will arrive".

**Example**

```c
if (uart_available()) {
    uint8_t c = uart_read_byte();   /* will not block */
    handle(c);
}
```

---

### 1.8 `uart_try_read`

```c
/**
 * @brief  Non-blocking read of one byte from the RX ring buffer.
 *
 * @param[out] c  Where the received byte is stored. Must not be NULL.
 *                Left unchanged when the function returns 0.
 * @return 1 if a byte was read into *c, 0 if the buffer was empty.
 */
uint8_t uart_try_read(uint8_t *c);
```

| | |
|---|---|
| **Purpose** | Preferred receive call when the caller has other work to do (e.g. a cooperative scheduler). |
| **Parameters** | `c` – output pointer. |
| **Returns** | `1` on success, `0` if no data. |

**Constraints / notes**

- Single-consumer design: call it (and `uart_read_byte`) from **one** execution context only. Two concurrent
  readers would race on the tail index.
- `c == NULL` is not checked.
- Bytes are returned in arrival order (FIFO).

**Example**

```c
uint8_t c;
while (uart_try_read(&c)) {     /* drain everything that has arrived */
    process(c);
}
do_other_work();
```

---

### 1.9 `uart_read_byte`

```c
/**
 * @brief  Blocking read: wait until a byte is available and return it.
 *
 * @return The oldest unread received byte.
 */
uint8_t uart_read_byte(void);
```

| | |
|---|---|
| **Purpose** | Simplest receive call for purely reactive applications (used by the demo app). |
| **Parameters** | None. |
| **Returns** | Received byte (0x00–0xFF). |

**Constraints / notes**

- **No timeout.** If no data ever arrives, the function never returns. Use `uart_available()` /
  `uart_try_read()` when a timeout or other background activity is required.
- While waiting, the CPU busy-polls (it does not enter a sleep mode).
- Same single-consumer rule as `uart_try_read`.

**Example**

```c
for (;;) {
    uint8_t c = uart_read_byte();
    uart_write_byte(c);          /* simple echo */
}
```

---

### 1.10 `USART1_IRQHandler` (internal – do not call)

```c
/**
 * @brief  USART1 interrupt service routine. Moves each received byte into the ring buffer.
 *
 * @note   Declared with __attribute__((interrupt("WCH-Interrupt-fast"))), which uses the
 *         QingKe hardware register-stacking feature; requires the WCH-aware GCC shipped with
 *         the PlatformIO ch32v platform.
 * @note   If the buffer is full, the new byte is discarded silently (no overflow flag).
 */
void USART1_IRQHandler(void);
```

This symbol is listed because it is linked into the vector table by name. Defining another
`USART1_IRQHandler` in the application causes a duplicate-symbol link error.

---

## 2. LED / GPIO driver (`led-lib`)

### 2.1 Hardware binding

| Item | Value |
|---|---|
| Pin | PC6 (`GPIOC`, `GPIO_Pin_6`) |
| Mode | Push-pull output, 50 MHz slew setting |
| Polarity | Active-high: logic 1 = LED on (LED + resistor from PC6 to GND) |
| State tracking | Software shadow variable (`led_state`); the pin is not read back |

---

### 2.2 `led_init`

```c
/**
 * @brief  Configure PC6 as a push-pull output and switch the LED off.
 *
 * @return None.
 */
void led_init(void);
```

| | |
|---|---|
| **Purpose** | Must be called once before any other `led_*` function. |
| **Parameters** | None. |
| **Returns** | Nothing. Post-condition: pin low, `led_get() == 0`. |

**Constraints / notes**

- Enables the GPIOC clock; harmless if already enabled by other code.
- Reconfigures PC6 unconditionally – do not use PC6 for anything else in the same build.

**Example**

```c
led_init();          /* LED is now off */
```

---

### 2.3 `led_on` / `led_off`

```c
/**
 * @brief  Drive PC6 high (LED on) and record the new state.
 * @return None.
 */
void led_on(void);

/**
 * @brief  Drive PC6 low (LED off) and record the new state.
 * @return None.
 */
void led_off(void);
```

| | |
|---|---|
| **Purpose** | Set the LED to an explicit state, independent of its previous state. |
| **Parameters** | None. |
| **Returns** | Nothing. |

**Constraints / notes**

- Calling them before `led_init()` has no visible effect on the pin (port clock and mode not set up), but the
  shadow state is still updated, so `led_get()` would then report a state the pin does not have.
- Not protected against concurrent use from an ISR and main code; the read-modify of `led_state` in
  `led_toggle` is not atomic with respect to these calls.

**Example**

```c
led_on();
/* ... */
led_off();
```

---

### 2.4 `led_toggle`

```c
/**
 * @brief  Invert the LED state (on → off, off → on).
 * @return None.
 */
void led_toggle(void);
```

| | |
|---|---|
| **Purpose** | Single call to flip the LED, used by the `1` command in the application. |
| **Parameters** | None. |
| **Returns** | Nothing. |

**Constraints / notes**

- Decides the new state from the software shadow, not from the pin. If code outside this driver writes PC6,
  the toggle direction can be wrong.

**Example**

```c
led_toggle();
uart_write_string(led_get() ? "ON\r\n" : "OFF\r\n");
```

---

### 2.5 `led_get`

```c
/**
 * @brief  Return the last state commanded through this driver.
 *
 * @return 1 if the LED was last switched on, 0 if off.
 */
uint8_t led_get(void);
```

| | |
|---|---|
| **Purpose** | Report LED status (e.g. in a status command) without touching the hardware. |
| **Parameters** | None. |
| **Returns** | `1` = on, `0` = off. |

**Constraints / notes**

- This is the **commanded** state. A wiring fault (LED reversed, open circuit) is not detected; the function
  still returns `1` after `led_on()`.

**Example**

```c
if (!led_get()) {
    led_on();
}
```

---

## 3. Integrating the drivers into another project

1. Copy `uart-lib.h/.c` and/or `led-lib.h/.c` into the target project's `lib/` folder.
2. Make the headers visible and the sources compiled. With PlatformIO and the same layout as this project:
   ```ini
   [env:vsdsquadronMini]
   platform = ch32v
   framework = noneos-sdk
   board = vsdsquadronMini
   build_flags = -I lib
   build_src_filter = +<*> +<../lib/*.c>
   ```
3. Call `led_init()` and `uart_init(baud)` once at start-up, before any other function of those modules.
4. Make sure no other code in the project uses PC6, PD5, PD6, USART1 or defines `USART1_IRQHandler`.
5. If the project uses other interrupts, choose priorities compatible with `NVIC_PriorityGroup_1`
   (set by `uart_init`).

Minimal skeleton combining both drivers:

```c
#include <uart-lib.h>
#include <led-lib.h>

int main(void) {
    led_init();
    uart_init(115200);
    uart_write_string("boot\r\n");

    for (;;) {
        uint8_t c;
        if (uart_try_read(&c) && c == 't') {
            led_toggle();
        }
        /* other periodic work here */
    }
}
```
