# Application Guide

This document explains what the application in
[app/main.c](../../task3/submission/app/main.c) does with the drivers. Driver internals are covered in
[API_REFERENCE.md](API_REFERENCE.md) and [ARCHITECTURE.md](ARCHITECTURE.md).

## 1. Purpose

The application is a **byte-level command console**. Every byte received over UART is:

1. counted,
2. reported back to the host with its sequence number, a printable representation and its hex value, and
3. interpreted as a command (`1`, `?`) or, if it is not a command, echoed back.

The verbose log line makes the behaviour of the UART link observable: a reviewer can see exactly which byte
values arrived (including invisible ones such as CR/LF) and in which order.

## 2. Application state

| State variable | Type | Owner | Initial value | Changed by |
|---|---|---|---|---|
| `rx_count` | `uint32_t` (local in `main`) | Application | 0 | Incremented once per received byte |
| LED state | `uint8_t` (inside LED driver) | LED driver | OFF (`led_init`) | `1` command |

There are no other modes: the command interpretation does not depend on previous input (no line buffer, no
multi-character commands).

### 2.1 Command set

| Byte received | Hex | Action | Response after the log prefix |
|---|---|---|---|
| `1` | 0x31 | `led_toggle()` | `LED toggled: ON` or `LED toggled: OFF` |
| `?` | 0x3F | Report status, then print help | `status` + status line + help text |
| any other byte | – | Echo the byte | `echo: ` followed by the **raw** byte |

### 2.2 Log prefix format

```
[RX #<n>] <repr> 0x<HH> -> [TX] <response>
```

| Field | Meaning |
|---|---|
| `<n>` | Value of `rx_count` after incrementing (first byte after reset = 1) |
| `<repr>` | `'c'` for printable ASCII 0x20–0x7E, `<CR>` for 0x0D, `<LF>` for 0x0A, `<?>` for any other byte |
| `<HH>` | Two-digit upper-case hex value of the byte |

## 3. State machine

### 3.1 Main loop

```
          ┌───────────┐
 reset ──►│   INIT    │ led_init(); uart_init(115200)
          └─────┬─────┘
                ▼
          ┌───────────┐
          │  BANNER   │ title, UART settings, help text
          └─────┬─────┘
                ▼
          ┌───────────┐  buffer empty
   ┌─────►│  WAIT_RX  │◄──────────┐
   │      └─────┬─────┘───────────┘
   │            │ byte available (uart_read_byte returns)
   │            ▼
   │      ┌───────────┐
   │      │  LOG_RX   │ rx_count++; print "[RX #n] repr 0xHH -> [TX] "
   │      └─────┬─────┘
   │            ▼
   │      ┌───────────┐
   │      │ DISPATCH  │
   │      └─┬───┬───┬─┘
   │   '1'  │   │'?'│ other
   │        ▼   ▼   ▼
   │   TOGGLE STATUS ECHO
   │        │   │   │
   └────────┴───┴───┘
```

### 3.2 LED sub-state

```
          '1'
   ┌─────┐ ──► ┌────┐
   │ OFF │     │ ON │
   └─────┘ ◄── └────┘
     ▲     '1'
     │
   reset / led_init()
```

Any byte other than `1` leaves the LED state unchanged.

## 4. Timing behaviour

All numbers assume 115200 baud, 8N1 (10 bits per character → **86.8 µs per character**).

| Phase | Duration | Notes |
|---|---|---|
| Boot banner + help | ≈ 160 characters ≈ 14 ms | Sent once after reset |
| Waiting for input | Unbounded | CPU busy-polls the ring buffer indices; no sleep |
| ISR per received byte | A few µs | Runs at every received byte, also while the main loop is transmitting |
| Response to a normal byte (echo) | 34–43 characters ≈ 3.0–3.7 ms | Length grows with the digits of `rx_count` |
| Response to `1` | ≈ 42 characters ≈ 3.6 ms | |
| Response to `?` | ≈ 150 characters ≈ 13 ms | Status line + full help text |

**LED switching latency.** `led_toggle()` is called **after** the log prefix (≈ 25 characters) has been
handed to the USART, so the LED changes ≈ 2.2 ms after the `1` byte was received, and before the
`LED toggled: …` text is sent. The text therefore always describes a change that has already happened.

**Throughput.** Because every input byte produces 34–150 output bytes over a link of equal speed, the
sustained processing rate is roughly 75–330 input bytes per second. Interactive typing (≤ 10 bytes/s) is
far below that limit; pasted text is not (see 6.2).

## 5. How the drivers are orchestrated

| Step | Driver call(s) | Why in this order |
|---|---|---|
| 1 | `led_init()` | Puts PC6 into a defined LOW state as early as possible, so the LED does not float during start-up |
| 2 | `uart_init(BAUD_RATE)` | After this the ISR starts collecting bytes; anything typed during the banner is buffered, not lost |
| 3 | `uart_write_string()` × 3 + `print_help()` | Banner; `uart_write_string` waits for TC so the text is fully on the wire before the loop starts |
| 4 | `uart_read_byte()` | Blocks until the ISR has queued a byte |
| 5 | `uart_write_string/uint/hex8/byte` | Log prefix |
| 6 | `led_toggle()` + `led_get()` (cmd `1`) | `led_get()` reads the new state to report it |
| 6 | `led_get()` + `uart_write_uint()` (cmd `?`) | Status report uses driver state, not a copy held by the application |
| 6 | `uart_write_byte(c)` (other) | Echo |

The application never touches hardware directly: it does not include `ch32v00x.h`.

## 6. Edge cases and how they are handled

### 6.1 Input content

| Case | Behaviour | Rationale |
|---|---|---|
| Unknown / "invalid" command byte | Echoed back; no error message | Every byte value has a defined action, so there is no invalid input. Echo provides visible confirmation that the byte arrived. |
| CR (0x0D) / LF (0x0A) | Logged as `<CR>` / `<LF>`, then echoed raw | Terminals send different line endings (CR, LF or CR+LF). Showing them explicitly explains extra log lines when the user presses Enter. The raw echoed LF causes a blank line in the terminal – expected. |
| Non-printable bytes (0x00–0x1F except CR/LF, 0x7F–0xFF) | Logged as `<?>` plus hex value, then echoed raw | The log field must not emit control characters into the terminal; the hex value still identifies the byte exactly. The echo field deliberately returns the original byte unchanged. |
| Repeated commands in one burst (`11`) | Each byte processed in order (LED toggles twice) | Byte-at-a-time semantics; demonstrated in the Task-3 evidence log (`RX #18` / `RX #19`). |
| Multi-character words (`led on`) | Treated as individual bytes | No line parser by design; keeps the application stateless. |

### 6.2 Input rate (buffer overflow)

- The ring buffer holds **63 unread bytes**. While the application spends ~3 ms answering one byte, a
  continuous stream delivers ~35 more.
- Calculated limit for a single burst of echo-type characters (e.g. pasting text): about **65 bytes**
  (63 buffered + the byte being processed + roughly one more consumed during the burst). This figure is
  derived from the timing table above, not measured.
- Bytes beyond the limit are **dropped silently** by the ISR. They do not appear in the log and do not
  increment `rx_count`. The reviewer can detect loss by comparing the number of bytes sent with the
  `bytes received` value from `?`.
- After a burst, the console resumes normally once the buffer drains; no reset is needed.

### 6.3 Timeouts and long idle periods

- `uart_read_byte()` has **no timeout**. The application is purely reactive, so waiting indefinitely is the
  intended behaviour; there is no work that could be starved.
- No watchdog is enabled, so the indefinite wait cannot cause a reset.

### 6.4 Counter overflow

- `rx_count` is `uint32_t` and wraps to 0 after 4 294 967 295 bytes. At the maximum sustained processing
  rate (~330 bytes/s) this takes about 150 days of continuous input. Wrap-around is harmless: the counter
  is only used for display.

### 6.5 Start-up

- Bytes received between `uart_init()` and the end of the banner are buffered and processed after the
  banner, in order.
- If the terminal is opened **after** the board has booted, the banner is not visible (it was sent to a
  closed port). Pressing RESET re-sends it; the console otherwise works without the banner.

### 6.6 Not handled (out of scope for v1.0.0)

| Case | Consequence |
|---|---|
| UART framing / noise errors (wrong baud on the host) | Corrupted bytes are delivered and echoed; the user sees garbage characters |
| LED wiring fault | Log reports the commanded state; the driver cannot detect that the LED did not light |
| RX overflow reporting | Only detectable indirectly via `bytes received` |
