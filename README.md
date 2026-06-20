# MineGuard — Simulated Prototype (Wokwi)

Loop-driven Modest-IoT safety node for ESP32. Sensors that Wokwi does not provide
natively are simulated with **custom chips**, exactly like the BykerZ prototype.
The firmware sensor **classes stay** (they are the drivers); the chips give them
real simulated hardware to read.

## Custom chips (the two you asked for)
| Chip | Files | Simulates | Read by class |
|------|-------|-----------|----------------|
| `chip-heart-rate` | `heart-rate.chip.c/.json` | Heart-rate sensor. A **BPM slider** outputs a DC voltage (40 BPM → 0 V, 180 BPM → 3.3 V) on pin **SIG**. | `PulseSensor` (analogRead on GPIO 32, maps back to BPM) |
| `chip-gps-neo6m`  | `gps-neo6m.chip.c/.json`  | NEO-6M GPS. Emits NMEA (GPGGA + GPRMC) at 1 Hz over UART, around **Lima, Peru**. | `GPSSensor` (TinyGPS++ on UART2, RX 16 / TX 17) |

## Pin layout
| Peripheral        | Pin(s)            |
|-------------------|-------------------|
| HC-SR04           | TRIG 5 / ECHO 18  |
| Heart-rate chip   | SIG → 32          |
| KY-031 / Choque   | 4                 |
| SOS button        | 13                |
| Red LED / Green   | 2 / 15 (220 Ω)    |
| Buzzer            | 23                |
| LCD I2C (0x27)    | SDA 21 / SCL 22   |
| GPS chip (UART2)  | TX→16 / RX→17     |

## How to load in Wokwi
1. Create a new ESP32 project. Paste `diagram.json`, `sketch.ino` and every
   `.h/.cpp` file as tabs.
2. Add the chips: tab menu (▾) → **New File** → create `heart-rate.chip.c`,
   `heart-rate.chip.json`, `gps-neo6m.chip.c`, `gps-neo6m.chip.json`. Wokwi
   compiles `.chip.c` automatically; the diagram references them as
   `chip-heart-rate` and `chip-gps-neo6m`.
3. `libraries.txt`: TinyGPSPlus, LiquidCrystal_I2C. (No Modest-IoT library — it's
   bundled flat here.)
4. Run. Drag the **Heart Rate (BPM)** slider on the heart-rate chip to trigger
   high-HR (≥100) or fatigue (<55) alerts; the GPS coordinates appear during SOS.

## What changed vs. the class-only version
Before, BPM was faked by mapping a generic potentiometer inside the class, and
the GPS had no data source. Now both are backed by proper Wokwi chips, so
`PulseSensor` and `GPSSensor` read real simulated signals — no class hacks.

## Event-driven architecture (no Arduino loop)
This version removes the busy `loop()`:
- **`loop()` is empty.**
- **Collision (KY-031) and SOS button** use real **hardware interrupts**
  (`attachInterruptArg`): the ISR latches the event; it is propagated as
  `COLLISION_DETECTED_EVENT` / `SOS_PRESSED_EVENT` in task context. Presses are
  never missed and nothing is polled for them.
- **Ultrasonic (x4), heart-rate and GPS** are sampled by a **Ticker** timer every
  200 ms — a ranging/analog/serial sensor must be *asked*, so it cannot announce
  itself. Each emits an event only when its value changes.
- **LCD, LEDs and buzzer refresh only on change**: drag a distance slider and the
  screen updates; while idle nothing is redrawn (the serial log prints `[CHANGE]`
  lines only when the visible state changes).
