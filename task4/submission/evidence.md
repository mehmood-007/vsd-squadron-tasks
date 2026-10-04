# Evidence – Task 4

## 1. Build

Built from this `submission/` folder with `pio run` (PlatformIO, platform `ch32v`, framework `noneos-sdk`,
board `vsdsquadronMini`):

```
Compiling .../lib/gpio-lib.o
Compiling .../lib/pwm-lib.o
Compiling .../lib/timer-lib.o
Compiling .../lib/uart-lib.o
Compiling .../src/main.o
RAM:   [====      ]  42.0% (used 860 bytes from 2048 bytes)
Flash: [======    ]  56.0% (used 9176 bytes from 16384 bytes)
========================= [SUCCESS] Took 3.81 seconds =========================
```

## 2. Flash

<!-- TODO: paste the output of `pio run -t upload` (WCH-Link programming / verify OK). -->

## 3. Serial logs (115200 8N1)

<!-- TODO: paste real terminal captures for each step below. -->

### Boot + mode selection
```
TODO: paste log ("ready. cmd: mode <0..4>")
```

### Mode 1 – LED / GPIO shell (`led on`, `led off`, `blink 250`, `read C7`)
```
TODO: paste log
```

### Mode 2 – Breathing PWM (`set duty`, `set freq`, `set speed`)
```
TODO: paste log
```

### Mode 3 – Pattern sequencer (`set pattern 10110011 200`, `pattern play`, `pattern stop`)
```
TODO: paste log
```

### Mode 4 – Reaction game (timer check, reaction time, best, false start, timeout)
```
TODO: paste log
```

## 4. Photos / video

<!-- TODO: add files next to this document and link them, e.g.
![Board setup](setup.jpg)
[Pattern sequencer video](pattern_demo.mp4)
-->

| What | File |
|---|---|
| Board + LED (PC6) + button (PC7) wiring | TODO |
| Breathing LED (mode 2) | TODO |
| Pattern `10110011` playing (mode 3) | TODO |
| Reaction game result on terminal (mode 4) | TODO |
