# Demo Guide

Goal: a reviewer who has never seen the code can build, flash and verify the firmware in about five minutes.

## 1. What you need

| Item | Detail |
|---|---|
| Board | VSDSquadron Mini + USB-C data cable |
| LED | Any standard LED + ~330 Ω resistor: **PC6 → resistor → LED anode, LED cathode → GND** |
| Host | Linux, Windows or macOS with [PlatformIO](https://platformio.org/) (CLI or VS Code extension) |
| Source | This repository, folder `task3/submission` |

Linux only: your user must be allowed to open the serial port (`sudo usermod -aG dialout $USER`, then log
out and in) and to access the WCH-Link USB device (udev rules from the
[PlatformIO CH32V platform](https://github.com/Community-PIO-CH32V/platform-ch32v) documentation).

## 2. Flashing steps

```bash
cd vsdsquadron-mini-core/task3/submission
pio run                  # expect: [SUCCESS], Flash ≈ 2900 bytes, RAM ≈ 372 bytes
pio run -t upload        # expect: [SUCCESS]; the onboard WCH-Link programs the CH32V003
```

The first `pio run` downloads the `ch32v` platform and toolchain, which takes longer than later builds.

## 3. UART settings

| Setting | Value |
|---|---|
| Port | WCH-Link serial port: `/dev/ttyACM0` (Linux), `COMx` (Windows), `/dev/cu.usbmodem*` (macOS) |
| Baud rate | **115200** |
| Data / parity / stop | 8 / none / 1 (8N1) |
| Flow control | None |
| Local echo | Off (the firmware echoes) |
| Send mode | Character-by-character (do not use a "send line" box for the main test) |

Open the console:

```bash
pio device monitor -b 115200
```

`pio device monitor` sends each key immediately and has local echo off, which matches the table above.

## 4. Test procedure – exact input and expected output

Type the keys below **without pressing Enter**. Each step lists exactly what must appear.

### Step 1 – Reset

Press the RESET button on the board (the serial port stays open).

Expected output:

```
=== Task3: UART driver library demo ===
USART1 115200 8N1, TX=PD5 RX=PD6
commands:
  1      toggle LED (PC6)
  ?      show status
  other  echoed back
```

Hardware: LED is **off**.

What this proves: the clock, USART1 TX path, pin PD5 and the baud rate are correct (readable text at the
configured rate), and `led_init()` drove PC6 low.

### Step 2 – Type `hello`

Expected output (one line per key):

```
[RX #1] 'h' 0x68 -> [TX] echo: h
[RX #2] 'e' 0x65 -> [TX] echo: e
[RX #3] 'l' 0x6C -> [TX] echo: l
[RX #4] 'l' 0x6C -> [TX] echo: l
[RX #5] 'o' 0x6F -> [TX] echo: o
```

What this proves: the RX path (PD6 → USART1 → interrupt → ring buffer → application) delivers every byte,
in order, with the correct value (the hex column matches ASCII).

### Step 3 – Type `1`

```
[RX #6] '1' 0x31 -> [TX] LED toggled: ON
```

Hardware: LED turns **on**.

### Step 4 – Type `1` again

```
[RX #7] '1' 0x31 -> [TX] LED toggled: OFF
```

Hardware: LED turns **off**.

What steps 3–4 prove: a received command reaches the application and is turned into a GPIO action through
the LED driver; the printed state matches the physical LED.

### Step 5 – Type `?`

```
[RX #8] '?' 0x3F -> [TX] status
status: LED=OFF, bytes received=8
commands:
  1      toggle LED (PC6)
  ?      show status
  other  echoed back
```

What this proves: the byte counter matches the eight keys typed since reset (no byte lost or duplicated),
and the reported LED state matches the hardware.

## 5. Pass criteria

| # | Check | Pass if |
|---|---|---|
| 1 | Banner after reset | All six lines readable, no garbage characters |
| 2 | Echo | Five lines for `hello`, counters #1–#5, hex values 68 65 6C 6C 6F |
| 3 | LED on | Log says `ON` **and** LED is lit |
| 4 | LED off | Log says `OFF` **and** LED is dark |
| 5 | Status | `bytes received=8`, `LED=OFF` |

## 6. Optional checks

### 6.1 Line endings

Press **Enter** once. Depending on the terminal's end-of-line setting you will see one or two extra lines,
for example:

```
[RX #9] <CR> 0x0D -> [TX] echo:
[RX #10] <LF> 0x0A -> [TX] echo:

```

The blank line is produced by the echoed LF byte itself. This is expected and explains why counters in
logs captured with "send line" terminals (see the Task-3 `evidence.md`) advance by two per command.

### 6.2 Burst / buffer limit (Linux)

In a **second** shell, with the monitor still open in the first:

```bash
stty -F /dev/ttyACM0 115200 raw -echo
printf 'abcdefghij%.0s' $(seq 10) > /dev/ttyACM0     # 100 bytes at once
```

Then type `?` in the monitor. Per the calculation in
[APPLICATION_GUIDE.md §6.2](APPLICATION_GUIDE.md#62-input-rate-buffer-overflow), roughly 65 of the 100
bytes are expected to be logged and counted; the rest are dropped by the full ring buffer. The console must
keep working afterwards without a reset. A 50-byte burst should be received completely.

## 7. Troubleshooting

| Symptom | Likely cause | Action |
|---|---|---|
| Nothing printed after reset | Monitor opened on the wrong port, or opened after the banner | Check `pio device list`; press RESET again |
| Garbage characters | Terminal baud ≠ 115200 | Set 115200 8N1 |
| Typed characters appear twice | Terminal local echo is on | Disable local echo |
| Log says `ON` but LED stays dark | LED reversed, wrong pin, or missing GND | Check wiring; the log shows the commanded state, not a measured one |
| `Permission denied` on `/dev/ttyACM0` | User not in `dialout` group | See section 1 |
| Upload fails / no WCH-Link found | Missing udev rules or charge-only USB cable | See section 1; use a data cable |
