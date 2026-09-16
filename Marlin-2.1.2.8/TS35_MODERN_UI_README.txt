MKS TS35 V2.0 - Modern Touch UI for Marlin 2.1.2.8
====================================================

Base:
- Marlin 2.1.2.8
- MKS Monster8 V2.0
- MKS TS35 V2.0
- TFT_COLOR_UI
- TOUCH_SCREEN

This package is rebuilt from the known working TS35 touch-fix base.
The previous Modern-UI fix1..fix5 files are NOT used.

UI implemented in the native 480x320 TFT_COLOR_UI path:
- Home dashboard with icon cards
- Temperatures
- Motor (Camera) temperature using TEMP_SENSOR_CHAMBER when enabled
- Movement
- Print / SD entry
- Printing progress, pause, resume and cancel controls
- Extruder temperature and manual extrusion/retraction
- Fan controls
- Bed Mesh / G29 entry
- Settings
- About
- Dedicated motor icon

No LVGL or SPI_FLASH requirement is added.
The existing Monster8 TS35 pin mapping and working touch configuration are preserved.

Compile environment: mks_monster8
