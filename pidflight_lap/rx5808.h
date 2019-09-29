#ifndef rx5808_h
#define rx5808_h

#include "Arduino.h"

class RX5808
{
  public:
    RX5808(uint8_t _rssiPin, uint8_t _spiDataPin, uint8_t _slaveSelectPin, uint8_t _spiClockPin);
    void init();
    void setFrequency(uint16_t frequency);
    void setFrequencyByIndex(uint8_t index);
    uint8_t getFrequencyIndex(uint16_t frequency);
    uint16_t readRssi();

  private:
    void waitRssi();
    void serialEnable(const uint8_t);
    void serialSendBit(const uint8_t);

    uint8_t rssiPin;
    uint8_t spiDataPin;
    uint8_t slaveSelectPin;
    uint8_t spiClockPin;
};

#endif
