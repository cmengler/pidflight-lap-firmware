#ifndef const_h
#define const_h

// Enable for PIDflight Lap support
//#define PDFL

// Enable for EasyRaceLapTimer Pocket Edition support
//#define ERLT

// Enable for Chorus RF support
//#define CHRF

// Enable buzzer
#if defined(PDFL)
  #define BUZZER
#endif

// Enable software serial
#if defined(ERLT)
  #define SOFTWARE_SERIAL
#endif

// Pin outs
#define PIN_SPI_DATA          10
#define PIN_SLAVE_SELECT      11
#define PIN_SPI_CLOCK         12
#if defined(ERLT)
  #define PIN_RSSI            A7
  #define PIN_SOFTSERIAL_RX   6
  #define PIN_SOFTSERIAL_TX   7
#elif defined(CHRF)
  #define PIN_RSSI            A3
#else
  #define PIN_BUZZER          6
  #define PIN_RSSI            A6
#endif

// Default RSSI threshold
#define RSSI_THRESHOLD        200

// RSSI threshold calibration
#define RSSI_THRESHOLD_CALIBRATE_READS 100

// Number of analog RSSI reads per tick
#define RSSI_READS            5

// Minimum time required for RX module to tune to frequency
#define MIN_TUNE_TIME         35

// Channels to sent to the SPI registers
const uint16_t channelTable[] PROGMEM = {
  // Channel 1 - 8
  0x2A05, 0x299B, 0x2991, 0x2987, 0x291D, 0x2913, 0x2909, 0x289F,    // Band A
  0x2903, 0x290C, 0x2916, 0x291F, 0x2989, 0x2992, 0x299C, 0x2A05,    // Band B
  0x2895, 0x288B, 0x2881, 0x2817, 0x2A0F, 0x2A19, 0x2A83, 0x2A8D,    // Band E
  0x2906, 0x2910, 0x291A, 0x2984, 0x298E, 0x2998, 0x2A02, 0x2A0C,    // Band F / Airwave
  0x281D, 0x288F, 0x2902, 0x2914, 0x2987, 0x2999, 0x2A0C, 0x2A1E     // Band C / Immersion Raceband
};

// Channels with their Mhz Values
const uint16_t channelFreqTable[] PROGMEM = {
  // Channel 1 - 8
  5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // Band A
  5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // Band B
  5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // Band E
  5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // Band F / Airwave
  5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917  // Band C / Immersion Raceband
};

// All Channels of the above List ordered by Mhz
const uint8_t channelList[] PROGMEM = {
  19, 18, 32, 17, 33, 16, 7, 34, 8, 24, 6, 9, 25, 5, 35, 10, 26, 4, 11, 27, 3, 36, 12, 28, 2, 13, 29, 37, 1, 14, 30, 0, 15, 31, 38, 20, 21, 39, 22, 23
};

// EEPROM version
#define EEPROM_BUILD_VERSION          3   // Increment this to force a "factory reset"

// EEPROM addresses
#define EEPROM_ADR_STATE              0
#define EEPROM_ADR_CHANNEL_L          1
#define EEPROM_ADR_CHANNEL_H          2
#define EEPROM_ADR_MIN_LAP_TIME       3
#define EEPROM_ADR_MAX_LAP            4
#define EEPROM_ADR_RSSI_MIN_L         5
#define EEPROM_ADR_RSSI_MIN_H         6
#define EEPROM_ADR_RSSI_MAX_L         7
#define EEPROM_ADR_RSSI_MAX_H         8
#define EEPROM_ADR_RSSI_THRESHOLD_L   9
#define EEPROM_ADR_RSSI_THRESHOLD_H   10
#define EEPROM_ADR_RSSI_FILTER_Q_L    11
#define EEPROM_ADR_RSSI_FILTER_Q_H    12
#define EEPROM_ADR_RSSI_FILTER_R_L    13
#define EEPROM_ADR_RSSI_FILTER_R_H    14

#endif
