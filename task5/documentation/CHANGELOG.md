# Changelog

All notable changes to the VSDSquadron Mini UART Command Console are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/): MAJOR for incompatible API changes, MINOR for backward-compatible
features, PATCH for fixes and documentation.

## v1.0.1 – 2026-10-10 – Documentation

Firmware binary unchanged.

### Added
- `task5/documentation/` documentation set: README, API reference, architecture, application guide,
  demo guide and this changelog.
- Documented the calculated RX burst limit (~65 bytes) and the silent-drop behaviour on buffer overflow.

## v1.0.0 – 2026-10-05 – Initial Release (Task 3)

### Added
- UART driver library (`lib/uart-lib.c/.h`) for USART1 on PD5/PD6, 8N1, runtime-selectable baud rate.
- Interrupt-driven reception (`USART1_IRQHandler`) into a 64-byte single-producer/single-consumer ring
  buffer (`UART_RX_BUF_SIZE`).
- Receive API: `uart_available()`, non-blocking `uart_try_read()`, blocking `uart_read_byte()`.
- Transmit helpers `uart_write_hex8()` and TX-complete wait at the end of `uart_write_string()`.
- GPIO driver for the LED on PC6 (`lib/led-lib.c/.h`): `led_init/on/off/toggle/get`.
- UART command interface application (`app/main.c`): per-byte RX log, `1` toggles LED, `?` prints status
  and help, all other bytes are echoed.
- PlatformIO configuration for the `app/` + `lib/` layout.

### Changed (compared with v0.1.0)
- `uart_write_char(char)` renamed to `uart_write_byte(uint8_t)`.
- `gpio_*` API renamed to `led_*`; pin constants moved out of the public header so `ch32v00x.h` is no
  longer exposed to the application.
- Source tree split into `app/` (application) and `lib/` (drivers).

## v0.1.0 – 2026-10-05 – Board bring-up (Task 2)

### Added
- Polled UART transmit on USART1: `uart_init`, `uart_write_char`, `uart_write_string`, `uart_write_uint`.
- GPIO helper for PC6: `gpio_init`, `gpio_set`, `gpio_clear`, `gpio_toggle`, `gpio_get`.

## Known limitations (open)

- RX buffer overflow drops bytes without an error flag.
- USART framing/noise errors are not reported.
- `uart_read_byte()` has no timeout variant.
- Pins and USART instance are hard-coded in the driver.
- Timer and PWM drivers are not part of this project (see Task 4).
