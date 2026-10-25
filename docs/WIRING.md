# Wiring guide

> ⚠️ **Read first.** The relay contacts can switch **mains voltage**. Wiring
> mistakes can destroy hardware, start a fire or kill. If you are not
> qualified to work with mains electricity, get a qualified electrician to do
> the mains side, or power 12/24 V loads instead. Always disconnect power
> before touching wiring, and keep the low-voltage (ESP32) side physically
> separated from the mains side.

## Bill of materials

| Part | Qty | Notes |
| --- | --- | --- |
| ESP32 Dev Module | 1 | Any 4 MB variant |
| 2-channel relay module, active-LOW, optocoupled | 2 | 5 V coil modules (e.g. Songle SRD-05VDC based) |
| Wall switch or push-button | 4 | Wired to GND, internal pull-ups used |
| 5 V DC power supply | 1 | ≥ 1 A, also powers the ESP32 via VIN/5V |
| Status LED + 220–470 Ω resistor | 1 | Optional |
| Hookup wire, screw terminals, enclosure | — | Mains-rated cable for the switched side |

## Pin map

| Function | GPIO |
| --- | --- |
| Relay 1 (module 1, IN1) | 23 |
| Relay 2 (module 1, IN2) | 22 |
| Relay 3 (module 2, IN1) | 21 |
| Relay 4 (module 2, IN2) | 19 |
| Switch 1 → GND | 13 |
| Switch 2 → GND | 12 |
| Switch 3 → GND | 14 |
| Switch 4 → GND | 27 |
| Status LED → resistor → GND | 2 |

## Connection order

1. **Relay module inputs** — each `IN` pin to its GPIO. The firmware defaults
   to **active-LOW** relay boards (`RELAY_ACTIVE_LOW true` in the sketch);
   invert the define if your module is active-HIGH.
2. **Common grounds** — join ESP32 GND, both relay module GNDs and the 5 V
   supply ground.
3. **Coil power** — the relay module's `VCC`/`JD-VCC` from the **5 V supply**,
   never from the ESP32's 3.3 V pin (two relay coils can draw ~150 mA; the
   3.3 V regulator cannot source that).
4. **Switches** — one side of each switch to its GPIO, the other to **GND**.
   The firmware uses `INPUT_PULLUP` and reads LOW when closed.
5. **Loads** — COM → live supply, NO (normally open) → load, load → neutral.
   Use NC instead if you want the load on when the relay is de-energised
   (rare).
6. **Power the ESP32** from the same 5 V supply (VIN pin) so everything shares
   one ground system, then flash over USB.

```
                     5 V supply
                  +----+  +----+
                  |    |  |    |
                 VCC  GND ...
                  |    |
   +--------------+    +----------------------------------+
   |                                                      |
   |        Relay module 1            Relay module 2       |
   |  IN1---GPIO23        IN1---GPIO21                    |
   |  IN2---GPIO22        IN2---GPIO19                    |
   |  VCC----+            VCC----+   (both to 5 V)        |
   |  GND----+--ESP32 GND--+                               |
   |                                                      |
   |  COM/NO/NC per channel -> your loads (mains-rated!)   |
   +------------------------------------------------------+

   Switch 1 --- GPIO13 ---+   (all switches to GND)
   Switch 2 --- GPIO12 ---+      ESP32  GPIO2 --[R]--> GND
   Switch 3 --- GPIO14 ---+              (status LED)
   Switch 4 --- GPIO27 ---+
                            GND
```

## Behaviour notes

- Switches are read with `INPUT_PULLUP` and switch to GND. Latching wall
  switches work in toggle mode. For momentary push-buttons set
  `SWITCH_MOMENTARY` to `true` in the sketch.
- The firmware seeds the switch state at boot, so a switch left on does not
  flip its relay when power returns.
- Per-channel **power-on behaviour** (off / on / restore last state) is set in
  the dashboard.
- If a channel should drive a pulse (e.g. a door lock or a bell), set its
  **inching** time in the dashboard — the relay closes for N seconds and
  opens again.

## Customising pins

All pins are defined at the top of `esp32-smart-home.ino`:

```cpp
const int relayPins[NUM_RELAYS]  = {23, 22, 21, 19};
const int switchPins[NUM_RELAYS] = {13, 12, 14, 27};
```

Pick any free GPIOs, then rebuild. Avoid strapping pins (0, 2, 5, 12, 15 —
12 is used here as a switch input, which is safe after boot) and keep
mains-side wiring well away from the board.
