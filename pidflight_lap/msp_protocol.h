#pragma once

#define MSP_PROTOCOL_VERSION                0

#define API_VERSION_MAJOR                   2 // increment when major changes are made
#define API_VERSION_MINOR                   6 // increment when any change is made, reset to zero when major changes are released after changing API_VERSION_MAJOR

#define API_VERSION_LENGTH                  2

/******************************************************************
 * MSP CODES
 ******************************************************************/

#define MSP_API_VERSION                     1

#define MSP_STATUS                          4 // Status of the device

#define MSP_RESET                           5 // Reset lap timer
#define MSP_RSSI_CALIBRATE                  6 // Calibrate RSSI threshold
#define MSP_START                           7 // Start lap timer
#define MSP_ACTIVATE                        8 // Activate lap timer
#define MSP_DEACTIVATE                      9 // Deactivate lap timer

#define MSP_DEVICE_ID                       10 // Device ID
#define MSP_SET_DEVICE_ID                   11

#define MSP_CHANNEL                         20 // VTx channel frequency
#define MSP_SET_CHANNEL                     21

#define MSP_CURRENT_LAP                     30 // Current lap and time
#define MSP_LAP                             31 // Lap and time
#define MSP_SET_LAP                         32

#define MSP_LAP_MIN_TIME                    34 // Lap minimum time
#define MSP_SET_LAP_MIN_TIME                35

#define MSP_LAP_MAX                         37 // Lap maximum laps
#define MSP_SET_LAP_MAX                     38

#define MSP_RSSI                            40 // Current RSSI reading

#define MSP_RSSI_THRESHOLD                  42 // RSSI threshold
#define MSP_SET_RSSI_THRESHOLD              43

#define MSP_RSSI_FILTER                     44 // RSSI filter parameters
#define MSP_SET_RSSI_FILTER                 45

#define MSP_DEBUG                           90 // Get debug data
#define MSP_SET_DEBUG                       91 // Enable debug

#define MSP_EEPROM_WRITE                    250
