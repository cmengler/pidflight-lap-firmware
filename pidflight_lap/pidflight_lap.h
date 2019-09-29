#ifndef pidflight_lap_h
#define pidflight_lap_h

#define WILDCARD_DEVICE_ID 0

extern uint8_t deviceID;

typedef enum {
    DEVICE_IDLE,
    DEVICE_TIMING,
    DEVICE_CALIBRATE_RSSI,
    DEVICE_INACTIVE
} deviceState_e;

extern deviceState_e state;

typedef enum {
    WAITING,
    START,
    STOP
} lapTimerState_e;

#define LAP_TIMER_MAXIMUM_LAPS 50

typedef struct lapTimer_t {
    uint16_t  timerMin;
    uint32_t  timerStart;
    uint32_t  timerElapsed;
    uint8_t   count;
    uint32_t  times[LAP_TIMER_MAXIMUM_LAPS];
    uint16_t  rssi[LAP_TIMER_MAXIMUM_LAPS];
    uint16_t  rssi_filter[LAP_TIMER_MAXIMUM_LAPS];
    lapTimerState_e state;
} lapTimer_t;

extern lapTimer_t lapTimer;

extern uint16_t rssi;
extern uint16_t rssi_threshold;
extern uint16_t rssi_min;
extern uint16_t rssi_max;
extern uint16_t channel;
extern uint8_t lap_maximum;
extern uint8_t debug;

extern uint16_t rssi_filter;
extern uint16_t rssi_filter_q;
extern uint16_t rssi_filter_r;

void lapTimerStart(uint32_t timerStart);
void lapTimerElapsed(void);
void lapTimerStop(uint32_t timerStop);
void lapTimerReset(void);

void reset(void);
void start(void);
void activate(void);
void deactivate(void);
void calibrateRSSI(void);
void rssiFilterUpdate(void);
void channelUpdate(void);

void readEEPROM(void);
void writeEEPROM(void);

#endif
