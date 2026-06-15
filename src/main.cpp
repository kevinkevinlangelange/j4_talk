//******************************************************************************
//        file name:  j4_talk [ADAPTED FROM gp_talk_v1_4]
//     v0_1 created:  2024-03-12 -- 1523 CDT -KL
//     v0_8 updated:  2024-06-24 -- 1440 CDR -KL
//     v1_2 created:  2025-05-26 -- 0142 CDT -KL
//     v1_3 created:  2025-07-11 -- 0210 CDT -KL
//     v1_4 created:  2025-07-16 -- 2326 PDT -KL
//     v1_5 created:  2026-05-12 -- 0938 CDT -KL
//     v1_6 created:  2026-05-12 -- 1113 CDT -KL
//     last updated:  2025-07-17 -- 0004 PDT -KL
//     last updated:  2026-04-29 -- 0251 CDT -KL
//     last updated:  2026-06-01 -- 1520 CDT
//     last updated:  2026-06-02 -- 0953 CDT
//     last updated:  2026-06-02 -- 1015 CDT
//     last updated:  2026-06-10 -- CDT
//
//           author:  Kevin Lange
//      description:  Main code for Johnny 4 voice audio and mouth LEDs
//        equipment:  Teensy 4.1
//       update log:  v0_1   -- First proof of concept trial
//                    v0_1c  -- Changed WAV file to 8.3 filename convention
//                    v0_1d  -- Eliminated Servo Functionality (for troubleshooting)
//                    v0_1e  -- GOT SDCARD AND WAV FILE TO WORK WITH MAX98357A!!!
//                            - Re-implimented Servo Functionality
//                    v0_2   -- I'm not sure what happened here
//                    v0_3   -- Failed attempt at PWM LED brightness control
//                    v0_4   -- First attempt at software-controlled volume control
//                    v0_5   -- Adding Serial Communications with LilyGO TTGO T-Display v1.1
//                            - TX (Pin 17 on TTGO) and RX (Pin 2 on TTGO)
//                    v1_2   -- Changed Servo pin from pin 10 to pin 29 (to avoid Audio Shield)
//                            - Removed MAX98357
//                            - Added Teensy AudioShield (rev D)
//                            - Changed Serial Comms to TTGO from "Serial3" (pins 14 and 15 on Teensy 4.1)
//                              to "Serial8" (pins 34 and 35 on Teensy 4.1)
//                    v1_3   -- Working to get it working with Teensy 4 Audio Shield (Rev D) instead of the MAX98357A
//                    v1_4   -- Made servos work by receiving serial data from the LilyGO TTGO T-Display
//                           -- Changed Serial Comms to TTGO from "Serial3" (pins 14 and 15 on Teensy 4.1)
//                              to "Serial6" (pins 24 and 25 on Teensy 4.1)
//                    v1_5   -- Stripped back to LED sweep test to verify all 18 channels
//                    v1_6   -- Replaced bar-graph approach with 5-tier amplitude mapping
//                           -- Added gamma-corrected PWM and piecewise linear thresholds
//                    v1_7   -- Added SD card file list scanning at boot
//                           -- Added jukebox serial protocol (PLAY/STOP/PLAYING/LIST)
//                    v1_8   -- MOVED LED CHANNELS: 24 to 14, 25 to 15 (pins 24/25 are
//                              Serial6 to the receiver -- driving them as PWM killed
//                              the serial link), 38 to 10, 39 to 11 (pins 38/39 have
//                              no PWM on the Teensy 4.1, they could only snap on/off)
//                           -- Volume from the controller pot now applied to SGTL5000
//                              (was fixed at 0.8)
//                           -- Re-send file list on LIST? request from receiver
//                           -- Report end of track over serial so the now-playing
//                              highlight clears on the controller display
//
//
//
//  ------------------------------------------
//  NOTES
//  ------------------------------------------
// ******DEPRECATED****** -- MAX98357A is no longer used
//  You'll need to connect the following pins from the MAX98357 module to your Teensy 4.1:
//
//  VIN (or VCC): Connect this to the 3.3V or 5V on the Teensy
//  (the MAX98357 is typically 5V tolerant, but always check your module's specifications).
//  GND: Connect to any GND on the Teensy.
//  LRC: Connect to Teensy pin 3 (I2S Frame Sync).
//  BCLK: Connect to Teensy pin 4 (I2S Bit Clock).
//  DIN: Connect to Teensy pin 2 (I2S Data In).
//
//  Attach servo on pin 10
//
//  The issue that took days to resolve: just needed everything to be grounded. -KL
// *******************************************
//  ------------------------------------------
//
//
//
//
//
//  ------------------------------------------
//  RFP30N06LE MOSFET -- LED switching (one per channel)
//  ------------------------------------------
//  Each Teensy PWM pin drives a RFP30N06LE N-channel MOSFET to switch
//  the 12V LED supply from 3.3V logic.
//
//  Wiring (repeat for each of the 18 LED channels):
//    Gate   -- Teensy PWM pin (3.3V logic is sufficient to fully enhance this FET)
//    Source -- GND (shared ground with Teensy)
//    Drain  -- LED cathode (-)
//    LED anode (+) -- 12V through a current-limiting resistor
//
//  The RFP30N06LE is rated 30A / 60V and turns on fully at Vgs ~2V,
//  so the 3.3V Teensy output drives it with margin to spare.
//  ------------------------------------------
//
//
//
//
//
//  ------------------------------------------
//  Teensy 4.1 Module's Pin Connections
//  ------------------------------------------
//  VIN:
//  GND:
//  24:   Serial Transmit (TX6) to LilyGO TTGO T-Display j4_receiver (pin 2)
//  25:   Serial Receive (RX6) from LilyGO TTGO T-Display j4_receiver (pin 17)
//
//  Mouth LED channels (PWM, via RFP30N06LE MOSFET gates):
//  02, 03, 04, 05, 06, 09, 10, 11, 14, 15, 16, 17, 22, 28, 29, 33, 36, 37
//
//  Used by Audio Shield Rev D (do not reuse):
//  07, 08, 20, 21, 23 (I2S) and 18, 19 (I2C)
//  ------------------------------------------
//
//
//
//
//
//******************************************************************************

#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <math.h>

// =============================================================================
// Audio-Reactive Robot Mouth -- Teensy 4.1 + Audio Shield Rev D
// =============================================================================
//
//           MOUTH LED PHYSICAL LAYOUT:
//
//            2   3   4   5   6   ||  9   16   17   22   14
//            15                                                          15
//            28                                                          28
//            29                                                          29
//            33                                                          33
//            36                                                          36
//            37                                                          37
//            10                                                          10
//            11                                                          11
//
// AMPLITUDE TIERS (each tier ADDS to the previous one):
//   Level 0 (silence):  all off
//   Level 1 (quiet):    6, 9                       (innermost pair)
//   Level 2:           +5, 16, 15, 28
//   Level 3:           +4, 17, 29, 33
//   Level 4:           +3, 22, 36, 37
//   Level 5 (loudest): +2, 14, 10, 11              (outermost)
//
// BRIGHTNESS BEHAVIOUR:
//   - Lower tiers stay fully lit at 255.
//   - The "next" tier above the current solid level dims smoothly from
//     0 to 255 as amplitude rises toward the next threshold.
//   - Brightness is gamma-corrected so partial-tier ramps look visually
//     linear to the human eye.
//
// Note: pins 10, 11, 15, 28, 29, 33, 36, 37 each drive a pair of LEDs
//       wired in parallel -- handled by the wiring, not the code.
// =============================================================================

// ---------------------------------------------------------------------------
// AMPLITUDE TIER PIN GROUPS
// ---------------------------------------------------------------------------
// Pins 24/25 are Serial6 to the receiver and pins 38/39 have no PWM hardware
// on the Teensy 4.1, so those four channels live on 14/15 and 10/11 instead.
const uint8_t TIER_1_PINS[] = { 6, 9 };
const uint8_t TIER_2_PINS[] = { 5, 16, 15, 28 };
const uint8_t TIER_3_PINS[] = { 4, 17, 29, 33 };
const uint8_t TIER_4_PINS[] = { 3, 22, 36, 37 };
const uint8_t TIER_5_PINS[] = { 2, 14, 10, 11 };

const uint8_t TIER_1_COUNT = sizeof(TIER_1_PINS) / sizeof(TIER_1_PINS[0]);
const uint8_t TIER_2_COUNT = sizeof(TIER_2_PINS) / sizeof(TIER_2_PINS[0]);
const uint8_t TIER_3_COUNT = sizeof(TIER_3_PINS) / sizeof(TIER_3_PINS[0]);
const uint8_t TIER_4_COUNT = sizeof(TIER_4_PINS) / sizeof(TIER_4_PINS[0]);
const uint8_t TIER_5_COUNT = sizeof(TIER_5_PINS) / sizeof(TIER_5_PINS[0]);

const uint8_t NUM_TIERS = 5;

const uint8_t ALL_PINS[] = {
  2, 3, 4, 5, 6, 9, 10, 11, 14,
  15, 16, 17, 22, 28, 29, 33, 36, 37
};
const uint8_t NUM_PINS = sizeof(ALL_PINS) / sizeof(ALL_PINS[0]);

// ---------------------------------------------------------------------------
// AMPLITUDE -> TIER LEVEL MAPPING (piecewise linear anchor points)
// ---------------------------------------------------------------------------
// THRESHOLDS[i] is the amplitude at which tier (i+1) becomes fully lit.
// Anything below THRESHOLDS[0] is silence (level 0.00).
// Anything at or above THRESHOLDS[4] is full mouth (level 5.00).
// Tune these to match your audio levels -- see Serial.println in updateLEDs().
// ---------------------------------------------------------------------------
const float THRESHOLDS[NUM_TIERS] = {
  0.02f,   // tier 1 fully on at this amplitude
  0.15f,   // tier 2 fully on
  0.35f,   // tier 3 fully on
  0.60f,   // tier 4 fully on
  0.85f    // tier 5 fully on (mouth fully open)
};

// ---------------------------------------------------------------------------
// VISUAL TUNING
// ---------------------------------------------------------------------------
// GAMMA: 2.2 is standard for sRGB-like perceptual brightness.
//        Lower (1.5-1.8) = brighter mid-range, partial tier lights up faster.
//        Higher (2.5-3.0) = darker mid-range, partial tier ramps up slower.
const float GAMMA = 2.2f;

// ---------------------------------------------------------------------------
// SMOOTHING / TIMING
// ---------------------------------------------------------------------------
const float         PEAK_SMOOTHING     = 0.80f;
const unsigned long UPDATE_INTERVAL_MS = 20;
const unsigned long ESP32_BAUD         = 115200;

// ---------------------------------------------------------------------------
// JUKEBOX FILE LIST
// ---------------------------------------------------------------------------
const uint8_t MAX_FILES     = 50;
const uint8_t FILE_ID_LEN   =  2;
const uint8_t FILE_NAME_MAX = 22;  // display name chars after 2-char ID, including extension

// ---------------------------------------------------------------------------
// AUDIO OBJECTS
// ---------------------------------------------------------------------------
AudioPlaySdWav        playSdWav;
AudioAnalyzePeak      peakLeft;
AudioAnalyzePeak      peakRight;
AudioOutputI2S        audioOutput;
AudioControlSGTL5000  sgtl5000;

AudioConnection patchL1(playSdWav, 0, peakLeft,    0);
AudioConnection patchR1(playSdWav, 1, peakRight,   0);
AudioConnection patchL2(playSdWav, 0, audioOutput, 0);
AudioConnection patchR2(playSdWav, 1, audioOutput, 1);

// ---------------------------------------------------------------------------
// STATE
// ---------------------------------------------------------------------------
float         smoothedPeak    = 0.0f;
unsigned long lastUpdate      = 0;

// ---------------------------------------------------------------------------
// JUKEBOX STATE
// ---------------------------------------------------------------------------
struct FileEntry {
  char id[FILE_ID_LEN + 1];
  char name[FILE_NAME_MAX + 1];
  char fullname[FILE_ID_LEN + FILE_NAME_MAX + 1];
};

FileEntry fileList[MAX_FILES];
uint8_t   fileCount  = 0;
char      nowPlaying[FILE_ID_LEN + 1] = "";
String    serialBuf  = "";
int           lastVolume      = -1;  // last 0-100 value applied to the SGTL5000
unsigned long playStartMillis = 0;

// ---------------------------------------------------------------------------
// FORWARD DECLARATIONS
// ---------------------------------------------------------------------------
void    initLEDs();
void    initAudio();
void    updateLEDs();
float   amplitudeToLevel(float amplitude);
void    setMouthLevel(float level);
void    writeTier(const uint8_t* pins, uint8_t count, uint8_t pwm);
uint8_t gammaCorrect(float fraction);
void    errorBlink();
void    scanSD();
void    sendFileList();
void    checkSerial();
void    processSerialLine(const String &line);

// ---------------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial6.begin(ESP32_BAUD);

  initLEDs();
  initAudio();

  scanSD();
  sendFileList();

  Serial.println("Robot mouth ready.");
}

// ---------------------------------------------------------------------------
// MAIN LOOP
// ---------------------------------------------------------------------------
void loop() {
  checkSerial();

  unsigned long now = millis();
  if (now - lastUpdate < UPDATE_INTERVAL_MS) return;
  lastUpdate = now;

  updateLEDs();

  // Report end of track so the now-playing highlight clears downstream.
  // The 250ms grace period matters: isPlaying() reads false for a few ms
  // right after play() while the WAV header is still being parsed.
  if (nowPlaying[0] != '\0' && !playSdWav.isPlaying()
      && now - playStartMillis > 250) {
    nowPlaying[0] = '\0';
    Serial6.println("PLAYING:");
  }
}

// ---------------------------------------------------------------------------
// FUNCTION DEFINITIONS
// ---------------------------------------------------------------------------

void initLEDs() {
  for (uint8_t i = 0; i < NUM_PINS; i++) {
    pinMode(ALL_PINS[i], OUTPUT);
    analogWrite(ALL_PINS[i], 0);
  }
}

void initAudio() {
  AudioMemory(16);
  sgtl5000.enable();
  sgtl5000.volume(0.8f);

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("ERROR: SD card not found.");
    errorBlink();
  }
}

void updateLEDs() {
  float rawPeak = 0.0f;
  if (peakLeft.available())  rawPeak = max(rawPeak, peakLeft.read());
  if (peakRight.available()) rawPeak = max(rawPeak, peakRight.read());

  smoothedPeak = (PEAK_SMOOTHING * smoothedPeak)
               + ((1.0f - PEAK_SMOOTHING) * rawPeak);

  float level = amplitudeToLevel(smoothedPeak);
  setMouthLevel(level);

  // Uncomment to tune thresholds:
  // Serial.print("peak="); Serial.print(smoothedPeak, 3);
  // Serial.print("  level="); Serial.println(level, 2);
}

float amplitudeToLevel(float amplitude) {
  if (amplitude < THRESHOLDS[0]) return 0.0f;
  if (amplitude >= THRESHOLDS[NUM_TIERS - 1]) return (float)NUM_TIERS;

  for (uint8_t i = 0; i < NUM_TIERS - 1; i++) {
    if (amplitude < THRESHOLDS[i + 1]) {
      float bandLow  = THRESHOLDS[i];
      float bandHigh = THRESHOLDS[i + 1];
      float frac     = (amplitude - bandLow) / (bandHigh - bandLow);
      return (float)(i + 1) + frac;
    }
  }
  return (float)NUM_TIERS;
}

void setMouthLevel(float level) {
  level = constrain(level, 0.0f, (float)NUM_TIERS);

  uint8_t solidTiers  = (uint8_t)level;
  float   partialFrac = level - solidTiers;
  uint8_t partialPwm  = gammaCorrect(partialFrac);

  // Tier 1
  if (solidTiers >= 1)      writeTier(TIER_1_PINS, TIER_1_COUNT, 255);
  else if (solidTiers == 0) writeTier(TIER_1_PINS, TIER_1_COUNT, partialPwm);

  // Tier 2
  if (solidTiers >= 2)      writeTier(TIER_2_PINS, TIER_2_COUNT, 255);
  else if (solidTiers == 1) writeTier(TIER_2_PINS, TIER_2_COUNT, partialPwm);
  else                      writeTier(TIER_2_PINS, TIER_2_COUNT, 0);

  // Tier 3
  if (solidTiers >= 3)      writeTier(TIER_3_PINS, TIER_3_COUNT, 255);
  else if (solidTiers == 2) writeTier(TIER_3_PINS, TIER_3_COUNT, partialPwm);
  else                      writeTier(TIER_3_PINS, TIER_3_COUNT, 0);

  // Tier 4
  if (solidTiers >= 4)      writeTier(TIER_4_PINS, TIER_4_COUNT, 255);
  else if (solidTiers == 3) writeTier(TIER_4_PINS, TIER_4_COUNT, partialPwm);
  else                      writeTier(TIER_4_PINS, TIER_4_COUNT, 0);

  // Tier 5
  if (solidTiers >= 5)      writeTier(TIER_5_PINS, TIER_5_COUNT, 255);
  else if (solidTiers == 4) writeTier(TIER_5_PINS, TIER_5_COUNT, partialPwm);
  else                      writeTier(TIER_5_PINS, TIER_5_COUNT, 0);
}

void writeTier(const uint8_t* pins, uint8_t count, uint8_t pwm) {
  for (uint8_t i = 0; i < count; i++) {
    analogWrite(pins[i], pwm);
  }
}

uint8_t gammaCorrect(float fraction) {
  fraction = constrain(fraction, 0.0f, 1.0f);
  float corrected = powf(fraction, GAMMA);
  return (uint8_t)(corrected * 255.0f + 0.5f);
}

void errorBlink() {
  while (true) {
    for (uint8_t i = 0; i < NUM_PINS; i++) analogWrite(ALL_PINS[i], 255);
    delay(300);
    for (uint8_t i = 0; i < NUM_PINS; i++) analogWrite(ALL_PINS[i], 0);
    delay(300);
  }
}

void scanSD() {
  File root = SD.open("/");
  while (fileCount < MAX_FILES) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (entry.isDirectory()) { entry.close(); continue; }

    const char *fname = entry.name();
    size_t len = strlen(fname);

    if (len < 6) { entry.close(); continue; }  // minimum: "XX.WAV"

    // Teensy SD library returns uppercase names
    if (strcmp(fname + len - 4, ".WAV") != 0) { entry.close(); continue; }

    FileEntry &fe = fileList[fileCount];

    strncpy(fe.id, fname, FILE_ID_LEN);
    fe.id[FILE_ID_LEN] = '\0';

    strncpy(fe.name, fname + FILE_ID_LEN, FILE_NAME_MAX);
    fe.name[FILE_NAME_MAX] = '\0';

    strncpy(fe.fullname, fname, sizeof(fe.fullname) - 1);
    fe.fullname[sizeof(fe.fullname) - 1] = '\0';

    fileCount++;
    entry.close();
  }
  root.close();
  Serial.printf("SD scan: %d files found.\n", fileCount);
}

void sendFileList() {
  Serial6.printf("LIST_START:%d\n", fileCount);
  for (uint8_t i = 0; i < fileCount; i++) {
    Serial6.printf("%s|%s\n", fileList[i].id, fileList[i].name);
  }
  Serial6.println("LIST_END");
}

void checkSerial() {
  while (Serial6.available()) {
    char c = (char)Serial6.read();
    if (c == '\n') {
      serialBuf.trim();
      if (serialBuf.length() > 0) processSerialLine(serialBuf);
      serialBuf = "";
    } else if (c != '\r') {
      serialBuf += c;
    }
  }
}

void processSerialLine(const String &line) {
  if (line.startsWith("PLAY:")) {
    String id = line.substring(5);
    for (uint8_t i = 0; i < fileCount; i++) {
      if (strncmp(fileList[i].id, id.c_str(), FILE_ID_LEN) == 0) {
        playSdWav.stop();
        playSdWav.play(fileList[i].fullname);
        playStartMillis = millis();
        strncpy(nowPlaying, fileList[i].id, FILE_ID_LEN);
        nowPlaying[FILE_ID_LEN] = '\0';
        Serial6.printf("PLAYING:%s\n", nowPlaying);
        return;
      }
    }

  } else if (line == "STOP") {
    playSdWav.stop();
    nowPlaying[0] = '\0';
    Serial6.println("PLAYING:");

  } else if (line == "LIST?") {
    // Receiver missed the boot-time list (it booted after us) -- send it again
    sendFileList();

  } else {
    // "volume,neck" CSV from j4_receiver. Volume is 0-100 from the controller
    // pot; neck is ignored here. Only touch the SGTL5000 when the value changes.
    int comma = line.indexOf(',');
    if (comma > 0) {
      int vol = line.substring(0, comma).toInt();
      if (vol != lastVolume && vol >= 0 && vol <= 100) {
        lastVolume = vol;
        sgtl5000.volume(vol * 0.008f);  // 0-100 mapped to 0.0-0.8 (safe max)
      }
    }
  }
}
