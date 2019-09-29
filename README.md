# PIDflight Lap Firmware

![PIDflight](https://www.pidflight.com/logo.png)

[PIDflight Lap](https://www.pidflight.com/pidflight-lap/) is a video transmitter (VTx) lap timing solution for individual pilots and multi-pilot support for race meets of up to 8 pilots.

This is the firmware for the PIDflight Lap timing device.

The PIDflight Lap firmware is available for the original PIDflight Lap schematic, EasyRaceLapTimer PocketEdition and Chorus RF Lap Timer.

- **PIDflight Lap**: `pidflightlap_[VERSION]_PDFL.hex`
- **EasyRaceLapTimer Pocket Edition**: `pidflightlap_[VERSION]_ERLT.hex`
- **Chorus RF**: `pidflightlap_[VERSION]_CHRF.hex`

## Build via Arduino IDE

1. Download and install [Arduino IDE](https://www.arduino.cc/en/main/software)
2. Open the `pidflight_lap.ino` file with Arduino IDE
3. Select your **Board** (e.g. *Arduino Nano*) and **Port** from the **Tools** file menu
4. Select **Upload** from the **Sketch** file menu

## Build via PlatformIO

1. Install PlatformIO: `pip install -U platformio`
2. Clean build directory: `platformio run --target clean`
3. Build firmware: `platformio run --environment pdfl`
4. Upload to Arduino: `platformio run --environment pdfl -t upload`

### Firmware build and release

Update the `version` directive in the `[release]` section.

```ini
[release]
version = 2.2.0
```

Run: `platformio run` to build all variants of the firmware.

#### Build or release firmware for one specific schematic

- **PIDflight Lap** use `platformio run --environment pdfl`
- **EasyRaceLapTimer Pocket Edition** use `platformio run --environment erlt`
- **Chorus RF** use `platformio run --environment chrf`
