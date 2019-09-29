#include <EEPROM.h>
#include <math.h>

#include "const.h"
#include "kalman.h"
#include "rx5808.h"
#include "pidflight_lap.h"
#include "buf_writer.c"
#include "serial_msp.c"

#ifdef SOFTWARE_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial swSerial(PIN_SOFTSERIAL_RX, PIN_SOFTSERIAL_TX);
#endif

KalmanFilter kalmanFilter;

uint8_t deviceID = 0;
deviceState_e state;
lapTimer_t lapTimer;

RX5808 RX5808(PIN_RSSI, PIN_SPI_DATA, PIN_SLAVE_SELECT, PIN_SPI_CLOCK);

uint16_t rssi = 0;
uint16_t rssi_min = 0;
uint16_t rssi_max = 0;

uint16_t rssi_threshold = RSSI_THRESHOLD;
uint8_t rssi_threshold_offset = 36;

uint16_t rssi_filter = 0;
uint16_t rssi_filter_q = 2000; //  0.01 - 655.36
uint16_t rssi_filter_r = 40; // 0.0001 - 65.536

uint16_t channel = 5740;

uint8_t lap_maximum = LAP_TIMER_MAXIMUM_LAPS;

uint16_t rssi_peak = 0;
uint32_t rssi_peak_time = 0;

uint8_t debug = 0;

void setup()
{
  // Setup serial connections
  Serial.begin(115200);
#ifdef SOFTWARE_SERIAL
  swSerial.begin(9600);
#endif

  // Buzzer
#ifdef BUZZER
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, HIGH);
#endif

  // Set the timer defaults
  lapTimer.timerMin = 4 * 1000;

  // Perform factory reset of EEPROM if required
  resetEEPROM();

  // Override default settings from EEPROM
  readEEPROM();

  // Initialise the RSSI filter
  rssiFilterUpdate();

  // Initialise the RX5808 module
  RX5808.init();

  activate();

  // Set the channel
  channelUpdate();
}

void reset(void)
{
  if (isDeviceActive()) {
    // Set the state to idle
    state = DEVICE_IDLE;

    // Reset the lap timer
    lapTimerReset();
  }
}

void start(void)
{
  if (isDeviceActive()) {
    // Set the state to timing
    state = DEVICE_TIMING;
  }
}

void calibrateRSSI(void)
{
  // Start RSSI calibration
  beep(500);

  // Reset RSSI min/max
  rssi_min = 1024;
  rssi_max = 0;

  uint32_t rssi_average = 0;

  for (uint8_t i = 0; i < RSSI_THRESHOLD_CALIBRATE_READS; i++) {
    rssi_average += round(kalmanFilter.filter(RX5808.readRssi(), 0));
  }

  rssi_threshold = (rssi_average / RSSI_THRESHOLD_CALIBRATE_READS) - 6;

  // Calibration done
  beep(500);
}

void activate(void)
{
  // Set the state to idle
  state = DEVICE_IDLE;

  // Reset the lap timer
  lapTimerReset();
}

void deactivate(void)
{
  // Set the state to inactive
  state = DEVICE_INACTIVE;

  // Reset the lap timer
  lapTimerReset();
}

bool isDeviceActive()
{
  return (state != DEVICE_INACTIVE);
}

void rssiFilterUpdate(void)
{
  kalmanFilter.setMeasurementNoise(rssi_filter_q * 0.01f);
  kalmanFilter.setProcessNoise(rssi_filter_r * 0.0001f);
}

void loop()
{
  // Always read RSSI when device is idling or timing
  if (state == DEVICE_IDLE || state == DEVICE_TIMING) {
    rssi = RX5808.readRssi();
    rssi_filter = round(kalmanFilter.filter(rssi, 0));

    // Update RSSI minimum and maximum values
    rssi_min = min(rssi_min, rssi_filter);
    rssi_max = max(rssi_max, rssi_filter);
  }

  // Process lap timer
  if (state == DEVICE_TIMING) {
    lapProcess();
  }

  // Process serial
  mspProcess(&Serial, &hwPort);
#ifdef SOFTWARE_SERIAL
  mspProcess(&swSerial, &swPort);
#endif

  // Dispatch debug data
  if (debug == 1) {
    mspProcessDebug(&Serial, &hwPort);
  }
}

/******************************************************************
 * LAP PROCESSING
 ******************************************************************/

void lapTimerStart(uint32_t timerStart)
{
  lapTimer.state = START;
  lapTimer.timerStart = timerStart;
}

void lapTimerElapsed(void)
{
  uint32_t timerStop = millis();
  lapTimer.timerElapsed = timerStop - lapTimer.timerStart;
}

void lapTimerStop(uint32_t timerStop)
{
  lapTimer.state = STOP;
  lapTimer.timerElapsed = timerStop - lapTimer.timerStart;
  lapTimer.count++;
  lapTimer.times[lapTimer.count - 1] = lapTimer.timerElapsed;
  lapTimer.rssi[lapTimer.count - 1] = rssi;
  lapTimer.rssi_filter[lapTimer.count - 1] = rssi_peak;
}

void lapTimerReset(void)
{
  lapTimer.state = WAITING;
  lapTimer.timerStart = 0;
  lapTimer.timerElapsed = 0;
  lapTimer.count = 0;
  for (uint8_t i = 0; i < lap_maximum; i++) {
    lapTimer.times[i] = 0;
  }
}

void lapPeakCapture(void)
{
  // Check if RSSI is on or post threshold, update RSSI peak
  if (rssi_filter >= rssi_threshold) {
    // Check if RSSI is greater than the previous detected peak
    if (rssi_filter > rssi_peak) {
      rssi_peak = rssi_filter;
      rssi_peak_time = millis();
    }
  }
}

bool lapPeakCaptured(void)
{
  if (rssi_filter < rssi_peak && rssi_filter < (rssi_threshold - rssi_threshold_offset)) {
    return true;
  }
  return false;
}

void lapPeakReset(void)
{
  rssi_peak = 0;
  rssi_peak_time = 0;
}

void lapProcess()
{
  switch (lapTimer.state) {
    case WAITING:
      lapPeakCapture();
      // Check if the peak has been captured, start lap timer
      if (lapPeakCaptured()) {
        // Start timer with peak time
        lapTimerStart(rssi_peak_time);

        // Reset peak
        lapPeakReset();

		    // Beep to indicate lap started
        beep(100);
      }
      break;
    case START:
      // Calculate current elapsed time
      lapTimerElapsed();

      // Check if timer min has elapsed, start capturing peak
      if (lapTimer.timerElapsed > lapTimer.timerMin) {
        lapPeakCapture();
      }

      // Check if the peak has been captured, stop/start lap timer
      if (lapPeakCaptured()) {
        // Stop current lap with peak time
        lapTimerStop(rssi_peak_time);

        // Check if lap count is less than the maximum number of laps, start timer
        if (lapTimer.count < lap_maximum) {
          // Start new lap with peak time again
          lapTimerStart(rssi_peak_time);
        }

        // Reset peak
        lapPeakReset();

        // Beep to indicate lap started
        beep(100);
      }
      break;
  }
}

void beep(uint16_t time)
{
#ifdef BUZZER
  digitalWrite(PIN_BUZZER, LOW); // activate beep
  delay(time / 2);
  digitalWrite(PIN_BUZZER, HIGH);
#endif
}

/******************************************************************
 * RX5808
 ******************************************************************/

void channelUpdate(void) {
  RX5808.setFrequency(channel);
}

/******************************************************************
 * MSP SERIAL PROCESSING
 ******************************************************************/

void mspProcess(Stream *serialPort, mspPort_t *port)
{
  mspSetCurrentPort(port);

  uint8_t buf[sizeof(bufWriter_t) + 20];
  writer = bufWriterInit(buf, sizeof(buf), (bufWrite_t) mspSerialWriteBuf, serialPort);

  while (serialPort->available() > 0) {
    uint8_t c = serialPort->read();
    bool consumed = mspProcessReceivedData(c);

    if (currentPort->c_state == COMMAND_RECEIVED) {
      // Process this MSP command if for this device ID only or all devices (wildcard)
      if (currentPort->c_message_type == MESSAGE_IN && (currentPort->deviceID == deviceID || currentPort->deviceID == WILDCARD_DEVICE_ID)) {
        mspProcessReceivedCommand();
      }
      // Pass this MSP command along the chain if its wildcard or command not for this device ID
      if (currentPort->c_message_type == MESSAGE_OUT || currentPort->deviceID != deviceID) {
        mspPassThroughCommand();
      }
      break; // process one command at a time so as not to block.
    }
  }
  bufWriterFlush(writer);
}

void mspSerialWriteBuf(Stream *serialPort, uint8_t *data, int count)
{
  for (uint8_t *p = data; count > 0; count--, p++) {
    serialPort->write(*p);
  }
}

void mspProcessDebug(Stream *serialPort, mspPort_t *port)
{
  mspSetCurrentPort(port);

  uint8_t buf[sizeof(bufWriter_t) + 20];
  writer = bufWriterInit(buf, sizeof(buf), (bufWrite_t) mspSerialWriteBuf, serialPort);

  mspProcessSendCommand(MSP_DEBUG);

  bufWriterFlush(writer);
}

/******************************************************************
 * EEPROM
 ******************************************************************/

void readEEPROM(void)
{
  if (EEPROM.read(EEPROM_ADR_STATE) == EEPROM_BUILD_VERSION) {
    channel = ((EEPROM.read(EEPROM_ADR_CHANNEL_H) << 8) | (EEPROM.read(EEPROM_ADR_CHANNEL_L)));
    lapTimer.timerMin = EEPROM.read(EEPROM_ADR_MIN_LAP_TIME) * 1000;
    lap_maximum = EEPROM.read(EEPROM_ADR_MAX_LAP);
    rssi_min = ((EEPROM.read(EEPROM_ADR_RSSI_MIN_H) << 8) | (EEPROM.read(EEPROM_ADR_RSSI_MIN_L)));
    rssi_max = ((EEPROM.read(EEPROM_ADR_RSSI_MAX_H) << 8) | (EEPROM.read(EEPROM_ADR_RSSI_MAX_L)));
    rssi_threshold = ((EEPROM.read(EEPROM_ADR_RSSI_THRESHOLD_H) << 8) | (EEPROM.read(EEPROM_ADR_RSSI_THRESHOLD_L)));
    rssi_filter_q = ((EEPROM.read(EEPROM_ADR_RSSI_FILTER_Q_H) << 8) | (EEPROM.read(EEPROM_ADR_RSSI_FILTER_Q_L)));
    rssi_filter_r = ((EEPROM.read(EEPROM_ADR_RSSI_FILTER_R_H) << 8) | (EEPROM.read(EEPROM_ADR_RSSI_FILTER_R_L)));
  }
}

void writeEEPROM(void)
{
  EEPROM.write(EEPROM_ADR_STATE, EEPROM_BUILD_VERSION);
  EEPROM.write(EEPROM_ADR_CHANNEL_L, lowByte(channel));
  EEPROM.write(EEPROM_ADR_CHANNEL_H, highByte(channel));
  EEPROM.write(EEPROM_ADR_MIN_LAP_TIME, (lapTimer.timerMin / 1000));
  EEPROM.write(EEPROM_ADR_MAX_LAP, lap_maximum);
  EEPROM.write(EEPROM_ADR_RSSI_MIN_L, lowByte(rssi_min));
  EEPROM.write(EEPROM_ADR_RSSI_MIN_H, highByte(rssi_min));
  EEPROM.write(EEPROM_ADR_RSSI_MAX_L, lowByte(rssi_max));
  EEPROM.write(EEPROM_ADR_RSSI_MAX_H, highByte(rssi_max));
  EEPROM.write(EEPROM_ADR_RSSI_THRESHOLD_L, lowByte(rssi_threshold));
  EEPROM.write(EEPROM_ADR_RSSI_THRESHOLD_H, highByte(rssi_threshold));
  EEPROM.write(EEPROM_ADR_RSSI_FILTER_Q_L, lowByte(rssi_filter_q));
  EEPROM.write(EEPROM_ADR_RSSI_FILTER_Q_H, highByte(rssi_filter_q));
  EEPROM.write(EEPROM_ADR_RSSI_FILTER_R_L, lowByte(rssi_filter_r));
  EEPROM.write(EEPROM_ADR_RSSI_FILTER_R_H, highByte(rssi_filter_r));
}

void resetEEPROM(void)
{
  if (EEPROM.read(EEPROM_ADR_STATE) != EEPROM_BUILD_VERSION) {
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0);
    }
    writeEEPROM();
  }
}
