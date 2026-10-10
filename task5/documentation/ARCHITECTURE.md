# Architecture

## 1. Overview

The firmware is a **layered, interrupt-assisted super-loop**. There is one foreground loop (the
application in `main()`) and one background context (the USART1 receive interrupt). There is no RTOS and no
scheduler; the application blocks on UART input and performs all work in response to a received byte.

## 2. Block diagram

```
  Host PC terminal (115200 8N1)
          │ USB-C
  ┌───────▼──────────────────┐
  │ Onboard WCH-Link         │   USB <-> UART bridge + programmer
  └───────┬──────────▲───────┘
          │ PD6 (RX) │ PD5 (TX)                                   LED + R to GND
==========│==========│=========================== CH32V003F4U6 ==========▲=======
          │          │                                                   │ PC6
  ┌───────▼──────────┴─────────────────┐   ┌───────────────────┐  ┌──────┴──────┐
  │ HARDWARE   USART1                  │   │ PFIC (interrupt   │  │ GPIOC       │
  └───────┬──────────▲─────────────────┘   │ controller)       │  └──────▲──────┘
          │          │                     └─────────▲─────────┘         │
  ┌───────▼──────────┴───────────────────────────────┴───────────────────┴──────┐
  │ VENDOR SDK (WCH NoneOS): startup, SystemInit, RCC_*, GPIO_*, USART_*, NVIC_*│
  └───────┬──────────▲──────────────────────────────────────────────────▲───────┘
          │ RXNE IRQ │ TXE / TC wait + data write                       │
  ┌───────▼──────────┴─────────────────────────┐   ┌────────────────────┴───────┐
  │ UART DRIVER  lib/uart-lib.c                │   │ LED DRIVER  lib/led-lib.c  │
  │                                            │   │                            │
  │  USART1_IRQHandler            (producer)   │   │  led_state (shadow, 0/1)   │
  │        │                                   │   │                            │
  │        ▼                                   │   │  led_init  led_on  led_off │
  │  rx_buf[64]  rx_head  rx_tail              │   │  led_toggle  led_get       │
  │        │                                   │   │                            │
  │        ▼                                   │   └────────────▲───────────────┘
  │  uart_try_read / uart_read_byte (consumer) │                │
  │  uart_write_byte / _string / _uint / _hex8 │                │
  │  uart_init  uart_available                 │                │
  └─────────────────────▲──────────────────────┘                │
                        │ uart_* API                            │ led_* API
  ┌─────────────────────┴───────────────────────────────────────┴──────────────┐
  │ APPLICATION  app/main.c                                                    │
  │   init -> banner -> loop { read byte -> log prefix -> dispatch -> respond }│
  │   state: rx_count (uint32_t); LED state is owned by the LED driver         │
  └────────────────────────────────────────────────────────────────────────────┘
```

## 3. Layer responsibilities

| Layer | Files | Owns | Must not |
|---|---|---|---|
| Application | `app/main.c` | Command semantics, log format, `rx_count` | Include `ch32v00x.h`, touch registers or SDK peripheral functions |
| Drivers | `lib/uart-lib.*`, `lib/led-lib.*` | Pin assignment, peripheral configuration, ISR, RX buffer, LED shadow state | Know about commands or text formats used by the application |
| Vendor SDK | PlatformIO `noneos-sdk` framework | Register definitions, clock tree setup, startup code, vector table | – (third-party, unmodified) |
| Hardware | CH32V003 | USART1, GPIOC/GPIOD, interrupt controller | – |

The dependency direction is strictly downward. The application includes only `uart-lib.h` and
`led-lib.h`; those headers include only `<stdint.h>`, so SDK types never leak into application code.

### Scheduler / event system

None is used. The only asynchronous event source is the USART1 RXNE interrupt, and its only job is to
enqueue the byte. All decisions are made in the foreground loop. This is sufficient because the application
has a single input (UART) and no periodic activity.

## 4. Data flow

### 4.1 Receive path (host → application)

```
Host key press
  → WCH-Link → PD6 → USART1 shift register → USART1 data register (RXNE = 1)
  → interrupt → USART1_IRQHandler
        next = (rx_head + 1) & 63
        if next != rx_tail: rx_buf[rx_head] = byte; rx_head = next
        else:              byte dropped (buffer full)
  → main(): uart_read_byte() → uart_try_read()
        byte = rx_buf[rx_tail]; rx_tail = (rx_tail + 1) & 63
  → command dispatch
```

### 4.2 Transmit path (application → host)

```
main(): uart_write_string("...") / uart_write_uint() / uart_write_hex8()
  → uart_write_byte() for each character
        busy-wait until TXE = 1, then write USART1 data register
  → (uart_write_string only) busy-wait until TC = 1
  → PD5 → WCH-Link → host terminal
```

TX is not buffered and not interrupt-driven: the caller blocks until every byte has been handed to the
peripheral. Ordering of output is therefore exactly the program order.

### 4.3 LED path

```
main(): led_toggle() → led_on()/led_off() → GPIO_WriteBit(GPIOC, Pin 6) + led_state update
main(): led_get()    → returns led_state (no hardware access)
```

## 5. Control flow

```
Reset
  └─► SDK startup: stack, .data/.bss init, SystemInit() (clock), global IRQ enable
        └─► main()
              ├─ led_init()          PC6 output, LED off
              ├─ uart_init(115200)   PD5/PD6, USART1 8N1, RX IRQ on, buffer empty
              ├─ print banner + help
              └─ loop forever
                    ├─ uart_read_byte()   (spins until ISR has queued a byte)
                    ├─ rx_count++ and print "[RX #n] 'c' 0xNN -> [TX] "
                    └─ '1' → toggle | '?' → status + help | other → echo

  Asynchronously, at any point in the loop (including inside TX busy-waits):
  USART1 RXNE ─► USART1_IRQHandler ─► enqueue byte ─► return to interrupted code
```

## 6. Concurrency model

The RX ring buffer is a **single-producer / single-consumer** queue:

| Variable | Written by | Read by |
|---|---|---|
| `rx_buf[]` | ISR only | main only |
| `rx_head` | ISR only | ISR, main |
| `rx_tail` | main only | ISR, main |

No interrupt disabling is needed because:

1. Each index has exactly one writer.
2. Indices are 8-bit and are read/written with single load/store instructions, so a reader never sees a
   half-updated value.
3. All three are `volatile`, so the compiler keeps the order "store data, then publish `rx_head`" and
   re-reads the indices on every loop iteration.
4. The ISR stores the byte **before** advancing `rx_head`, so the consumer can never read a slot that has not
   been written yet; the consumer reads the byte **before** advancing `rx_tail`, so the producer can never
   overwrite a slot that is still being read.

## 7. Resource map

| Resource | Owner | Configuration |
|---|---|---|
| PC6 | LED driver | Output push-pull |
| PD5 | UART driver | AF push-pull (USART1_TX) |
| PD6 | UART driver | Floating input (USART1_RX) |
| USART1 | UART driver | 8N1, RX + TX, RXNE interrupt |
| `USART1_IRQn` | UART driver | Preemption 1, sub-priority 1, group 1 |
| RAM | UART driver: 64 B buffer + 2 B indices; LED driver: 1 B; application: `rx_count` on stack | – |

## 8. Design decisions and rationale

| Decision | Alternative considered | Reason for the choice |
|---|---|---|
| Interrupt-driven RX with a ring buffer | Polling the RXNE flag from `main()` | The USART holds only one received byte. The application spends ~3–13 ms transmitting a response to each byte, during which 35–150 further bytes could arrive at 115200 baud. Polling would overrun after the second byte; the ISR captures every byte as it arrives. |
| Blocking, unbuffered TX | Interrupt-driven TX ring buffer | The application has nothing else to do while it transmits, so blocking costs nothing functionally, needs no extra RAM (2 KB total) and guarantees output order. A TX buffer would also need overflow handling. |
| Power-of-two buffer size with `& (N-1)` wrap | `% N` modulo | The RV32EC core has no hardware divide; a modulo would call a software routine inside the ISR. The mask is a single AND instruction, keeping the ISR short. |
| One empty slot to mark "full" | Separate element counter | A shared counter would be written by both ISR and main, requiring interrupts to be disabled around updates. The empty-slot scheme keeps one writer per variable. |
| Software shadow for LED state | Reading the output data register | Keeps `led_get()` free of hardware access and independent of SDK read functions; acceptable because the driver is the sole owner of PC6. |
| Application free of SDK/register calls | Calling SDK functions directly in `main.c` | Enforces the library boundary: the application can be ported by replacing only `lib/`, and the drivers can be reused without the application. |
| WCH NoneOS SDK underneath the drivers | Direct register writes | Register-level details (bit positions, baud divider calculation from the actual clock) are handled by vendor-maintained code; the drivers stay short and readable. |
| No `printf` | `printf` with a retargeted `_write` | `uart_write_uint` / `uart_write_hex8` cover the application's needs at a fraction of the flash cost on a 16 KB device (whole image is 2.9 KB). |
| Super-loop without scheduler | Cooperative scheduler or timer tick | Single input, no periodic tasks, no deadlines other than "don't lose RX bytes", which the ISR already guarantees within the buffer limit. |

## 9. Known architectural limits

- **RX overflow is silent.** If more than 63 bytes are pending, new bytes are discarded and nothing records
  that it happened. See [APPLICATION_GUIDE.md](APPLICATION_GUIDE.md#6-edge-cases-and-how-they-are-handled)
  for the resulting burst limit.
- **No receive error handling.** Framing, noise and parity errors are not inspected; a corrupted byte is
  delivered like any other.
- **Hard-coded pins and peripheral.** Using a remapped USART1 pin set or a different LED pin requires
  editing the driver source.
- **Busy-waiting.** The CPU never sleeps, which is acceptable for a USB-powered demo but not for a
  battery-powered product.
