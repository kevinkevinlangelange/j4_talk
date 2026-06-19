# j4_talk

Audio and mouth LED firmware for the Johnny 4 robot. Runs on a Teensy 4.1 with an Audio Shield Rev D. Plays WAV files from SD card as a serial-controlled jukebox and drives 18 mouth LEDs that react to the audio amplitude in real time.

## What this board does

- Scans the SD card for WAV files at boot and serves them as a jukebox
- Plays, stops, and reports tracks over a serial link from the j4_receiver (PLAY / STOP / LIST? in, PLAYING: out)
- Reports end of track over serial so the now-playing highlight clears on the controller display
- Sends a `PING` heartbeat once per second so j4_receiver (and the controller) can show j4_talk as connected on the status screen
- Applies the controller's volume pot to the SGTL5000 codec (previously fixed at 0.8)
- Analyzes live audio amplitude using the AudioAnalyzePeak library
- Drives 18 LEDs across 5 amplitude tiers -- the mouth opens progressively as audio gets louder
- PWM brightness on the leading tier is gamma-corrected so the ramp looks visually linear

## LED layout

18 PWM-capable Teensy pins drive the LED array. Each group of LEDs lights up as a tier when amplitude crosses its threshold:

| Tier | Pins | Description |
|------|------|-------------|
| 1 | 6, 9 | innermost pair |
| 2 | 5, 16, 15, 28 | |
| 3 | 4, 17, 29, 33 | |
| 4 | 3, 22, 36, 37 | |
| 5 | 2, 14, 10, 11 | outermost (full open) |

Pins 10, 11, 15, 28, 29, 33, 36, 37 each drive two LEDs wired in parallel. That is handled in the wiring, not in code.

Four channels were moved in v1_8: pins 24/25 are Serial6 to the receiver (driving them as PWM killed the serial link), and pins 38/39 have no PWM hardware on the Teensy 4.1 (they could only snap on/off). Those four LED channels now live on 14/15 and 10/11.

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | Teensy 4.1 |
| Audio | Teensy Audio Shield Rev D (SGTL5000 codec) |
| Storage | microSD card (in Teensy 4.1 built-in slot) |
| LED switching | RFP30N06LE N-channel MOSFET (one per LED channel) |

The MAX98357A used in earlier versions is no longer part of this build.

## MOSFET wiring

Each of the 18 LED channels uses a RFP30N06LE N-channel MOSFET to switch a 12V LED supply from 3.3V Teensy logic. The RFP30N06LE is rated 30A / 60V and fully enhances at Vgs ~2V, so the 3.3V output drives it with margin to spare.

Repeat this wiring for each channel:

| MOSFET pin | Connection |
|------------|------------|
| Gate | Teensy PWM pin |
| Source | GND (shared with Teensy) |
| Drain | LED cathode (-) |

The LED anode (+) connects to 12V through a current-limiting resistor sized for the LED forward voltage and desired current.

## Amplitude tiers

The amplitude-to-tier mapping uses piecewise linear thresholds defined in `THRESHOLDS[]`. Tune those five values to match your audio levels. Uncomment the `Serial.print` lines in `updateLEDs()` to watch the smoothed peak and level values in real time while tuning.

The smoothing constant `PEAK_SMOOTHING` (default 0.80) controls how fast the display reacts. Lower values respond faster but look noisier. Higher values smooth out transients but feel sluggish.

## Pin assignments

| Teensy Pin | Function |
|------------|----------|
| 2 | Tier 5 LED |
| 3 | Tier 4 LED |
| 4 | Tier 3 LED |
| 5 | Tier 2 LED |
| 6 | Tier 1 LED |
| 9 | Tier 1 LED |
| 10 | Tier 5 LED pair |
| 11 | Tier 5 LED pair |
| 14 | Tier 5 LED |
| 15 | Tier 2 LED pair |
| 16 | Tier 2 LED |
| 17 | Tier 3 LED |
| 22 | Tier 4 LED |
| 24 | Serial6 TX to j4_receiver (receiver GPIO 2) |
| 25 | Serial6 RX from j4_receiver (receiver GPIO 17) |
| 28 | Tier 2 LED pair |
| 29 | Tier 3 LED pair |
| 33 | Tier 3 LED pair |
| 36 | Tier 4 LED pair |
| 37 | Tier 4 LED pair |

Audio Shield Rev D occupies 7, 8, 20, 21, 23 (I2S) and 18, 19 (I2C); the SD card uses the Teensy 4.1 built-in slot. All LED pins are chosen to avoid those.

## Pin diagram

Standard Teensy 4.1 layout, component side up, USB at the TOP. The two long
edges read top-to-bottom; `*` marks a pin this firmware uses.

```
                      +======[ USB ]======+
                GND  -| GND            5V |-  5V
                 0   -|  0            GND |-  GND
                 1   -|  1           3.3V |-  3.3V
      LED T5     2  *-|  2            23  |-  (audio I2S)
      LED T4     3  *-|  3            22  |-* LED T4
      LED T3     4  *-|  4            21  |-  (audio I2S)
      LED T2     5  *-|  5            20  |-  (audio I2S)
      LED T1     6  *-|  6            19  |-  (audio I2C)
      (audio)    7   -|  7            18  |-  (audio I2C)
      (audio)    8   -|  8            17  |-* LED T3
      LED T1     9  *-|  9            16  |-* LED T2
      LED T5     10 *-| 10            15  |-* LED T2
      LED T5     11 *-| 11            14  |-* LED T5
                 12  -| 12            13  |-  (LED_BUILTIN)
   Serial6 TX    24 *-| 24            41  |-
   Serial6 RX    25 *-| 25            40  |-
                 26  -| 26            39  |-
                 27  -| 27            38  |-
      LED T2     28 *-| 28            37  |-* LED T4
      LED T3     29 *-| 29            36  |-* LED T4
                 30  -| 30            35  |-
                 31  -| 31            34  |-
                 32  -| 32         (bottom: SD card + SMD pads)
      LED T3     33 *-| 33                |
                      +===================+

   Serial6: 24 (TX) -> j4_receiver GPIO 2 ;  25 (RX) <- j4_receiver GPIO 17
   Audio Shield Rev D uses 7, 8, 20, 21, 23 (I2S) + 18, 19 (I2C) + built-in SD.
   LED tiers: T1 6,9 | T2 5,16,15,28 | T3 4,17,29,33 | T4 3,22,36,37 | T5 2,14,10,11
```

## Jukebox serial protocol

The board talks to j4_receiver over Serial6 at 115200 baud:

| Direction | Message | Meaning |
|-----------|---------|---------|
| in | `volume,neck` | CSV of volume (0-100) and neck value each frame |
| in | `PLAY:<id>` | start the track with that two-character ID |
| in | `STOP` | stop playback |
| in | `LIST?` | re-send the file list |
| out | `PLAYING:<id>` | a track started (empty payload = stopped / finished) |
| out | `LIST_START:<n>` ... `<id>\|<name>` ... `LIST_END` | the SD-card file list |

## WAV files

Place WAV files in the root of the SD card. The firmware scans them at boot and plays a file when it receives a matching `PLAY:<id>` command. 16-bit PCM, 44100 Hz stereo is the standard format for the Teensy Audio library.

## Building and uploading

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor serial output
pio device monitor
```

## Related projects

- **[j4_controller](https://github.com/kevinkevinlangelange/j4_controller)** -- the TTGO T-Display transmitter that sends control data
- **[j4_receiver](https://github.com/kevinkevinlangelange/j4_receiver)** -- the ESP32 receiver board on the robot that feeds this board over serial
