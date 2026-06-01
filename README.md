# j4_talk

Audio and mouth LED firmware for the Johnny 4 robot. Runs on a Teensy 4.1 with an Audio Shield Rev D. Plays WAV files from SD card and drives 18 mouth LEDs that react to the audio amplitude in real time.

## What this board does

- Plays WAV files from SD card via the Teensy Audio Shield Rev D
- Analyzes live audio amplitude using the AudioAnalyzePeak library
- Drives 18 LEDs across 5 amplitude tiers -- the mouth opens progressively as audio gets louder
- PWM brightness on the leading tier is gamma-corrected so the ramp looks visually linear
- Receives serial data from the TTGO ESP32 controller board

## LED layout

18 PWM-capable Teensy pins drive the LED array. Each group of LEDs lights up as a tier when amplitude crosses its threshold:

| Tier | Pins | Description |
|------|------|-------------|
| 1 | 6, 9 | innermost pair |
| 2 | 5, 16, 25, 28 | |
| 3 | 4, 17, 29, 33 | |
| 4 | 3, 22, 36, 37 | |
| 5 | 2, 24, 38, 39 | outermost (full open) |

Pins 25, 28, 29, 33, 36, 37, 38, 39 each drive two LEDs wired in parallel. That is handled in the wiring, not in code.

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | Teensy 4.1 |
| Audio | Teensy Audio Shield Rev D |
| Storage | microSD card (in Teensy 4.1 built-in slot) |

The MAX98357A used in earlier versions is no longer part of this build.

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
| 16 | Tier 2 LED |
| 17 | Tier 3 LED |
| 22 | Tier 4 LED |
| 24 | Tier 5 LED |
| 25 | Tier 2 LED pair |
| 28 | Tier 2 LED pair |
| 29 | Tier 3 LED pair |
| 33 | Tier 3 LED pair |
| 36 | Tier 4 LED pair |
| 37 | Tier 4 LED pair |
| 38 | Tier 5 LED pair |
| 39 | Tier 5 LED pair |

Audio Shield Rev D occupies pins 7, 8, 10, 11, 12, 13, 18, 19, 20, 21, 23. All LED pins are chosen to avoid those.

## WAV files

Place WAV files in the root of the SD card. The firmware currently plays `TRACK01.WAV` on loop. 16-bit PCM, 44100 Hz stereo is the standard format for the Teensy Audio library.

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
- **[j4_receiver](https://github.com/kevinkevinlangelange/j4_receiver)** -- the ESP32 receiver board on the robot
