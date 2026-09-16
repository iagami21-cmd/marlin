/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfigPre.h"

#if HAS_GRAPHICAL_TFT

#include "tft_image.h"
#include "ui_common.h"

const tImage NoLogo                 = { nullptr, 0, 0, NOCOLORS };

#if ENABLED(SHOW_BOOTSCREEN)
  const tImage MarlinLogo112x38x1   = { (void *)marlin_logo_112x38x1, 112, 38, GREYSCALE1 };
  const tImage MarlinLogo228x255x2  = { (void *)marlin_logo_228x255x2, 228, 255, GREYSCALE2 };
  const tImage MarlinLogo228x255x4  = { (void *)marlin_logo_228x255x4, 228, 255, GREYSCALE4 };
  const tImage MarlinLogo195x59x16  = { (void *)marlin_logo_195x59x16,  195,  59, HIGHCOLOR };
  const tImage MarlinLogo320x240x16 = { (void *)marlin_logo_320x240x16, 320, 240, HIGHCOLOR };
  const tImage MarlinLogo480x320x16 = { (void *)marlin_logo_480x320x16, 480, 320, HIGHCOLOR };
#endif
const tImage Background320x30x16    = { (void *)background_320x30x16, 320, 30, HIGHCOLOR };

const tImage HotEnd_64x64x4         = { (void *)hotend_64x64x4, 64, 64, GREYSCALE4 };
const tImage Bed_64x64x4            = { (void *)bed_64x64x4, 64, 64, GREYSCALE4 };
const tImage Bed_Heated_64x64x4     = { (void *)bed_heated_64x64x4, 64, 64, GREYSCALE4 };
const tImage Chamber_64x64x4        = { (void *)chamber_64x64x4, 64, 64, GREYSCALE4 };
const tImage Chamber_Heated_64x64x4 = { (void *)chamber_heated_64x64x4, 64, 64, GREYSCALE4 };
const tImage Motor_64x64x4          = { (void *)motor_64x64x4, 64, 64, GREYSCALE4 };
const tImage HotEnd_32x32x4          = { (void *)hotend_32x32x4, 32, 32, GREYSCALE4 };
const tImage Bed_32x32x4             = { (void *)heated_bed_32x32x4, 32, 32, GREYSCALE4 };
const tImage Fan_Fast_32x32x4        = { (void *)fan_fast_32x32x4, 32, 32, GREYSCALE4 };
const tImage SD_32x32x4              = { (void *)sd_32x32x4, 32, 32, GREYSCALE4 };
const tImage Settings_32x32x4        = { (void *)settings_32x32x4, 32, 32, GREYSCALE4 };
const tImage Menu_32x32x4            = { (void *)menu_32x32x4, 32, 32, GREYSCALE4 };
const tImage Motor_32x32x4           = { (void *)motor_32x32x4, 32, 32, GREYSCALE4 };
const tImage Home_32x32x4             = { (void *)home_32x32x4, 32, 32, GREYSCALE4 };
const tImage Fan0_64x64x4           = { (void *)fan0_64x64x4, 64, 64, GREYSCALE4 };
const tImage Fan1_64x64x4           = { (void *)fan1_64x64x4, 64, 64, GREYSCALE4 };
const tImage Fan_Slow0_64x64x4      = { (void *)fan_slow0_64x64x4, 64, 64, GREYSCALE4 };
const tImage Fan_Slow1_64x64x4      = { (void *)fan_slow1_64x64x4, 64, 64, GREYSCALE4 };
const tImage Fan_Fast0_64x64x4      = { (void *)fan_fast0_64x64x4, 64, 64, GREYSCALE4 };
const tImage Fan_Fast1_64x64x4      = { (void *)fan_fast1_64x64x4, 64, 64, GREYSCALE4 };
const tImage SD_64x64x4             = { (void *)sd_64x64x4, 64, 64, GREYSCALE4 };
const tImage Home_64x64x4           = { (void *)home_64x64x4, 64, 64, GREYSCALE4 };
const tImage BtnRounded_64x52x4     = { (void *)btn_rounded_64x52x4, 64, 52, GREYSCALE4 };
const tImage BtnRounded_42x39x4     = { (void *)btn_rounded_42x39x4, 42, 39, GREYSCALE4 };
const tImage Menu_64x64x4           = { (void *)menu_64x64x4, 64, 64, GREYSCALE4 };
const tImage Settings_64x64x4       = { (void *)settings_64x64x4, 64, 64, GREYSCALE4 };
const tImage Confirm_64x64x4        = { (void *)confirm_64x64x4, 64, 64, GREYSCALE4 };
const tImage Cancel_64x64x4         = { (void *)cancel_64x64x4, 64, 64, GREYSCALE4 };
const tImage Increase_64x64x4       = { (void *)increase_64x64x4, 64, 64, GREYSCALE4 };
const tImage Decrease_64x64x4       = { (void *)decrease_64x64x4, 64, 64, GREYSCALE4 };
const tImage Pause_64x64x4          = { (void *)pause_64x64x4, 64, 64, GREYSCALE4 };

const tImage Feedrate_32x32x4       = { (void *)feedrate_32x32x4, 32, 32, GREYSCALE4 };
const tImage Flowrate_32x32x4       = { (void *)flowrate_32x32x4, 32, 32, GREYSCALE4 };
const tImage Directory_32x32x4      = { (void *)directory_32x32x4, 32, 32, GREYSCALE4 };
const tImage Back_32x32x4           = { (void *)back_32x32x4, 32, 32, GREYSCALE4 };
const tImage Up_32x32x4             = { (void *)up_32x32x4, 32, 32, GREYSCALE4 };
const tImage Down_32x32x4           = { (void *)down_32x32x4, 32, 32, GREYSCALE4 };
const tImage Left_32x32x4           = { (void *)left_32x32x4, 32, 32, GREYSCALE4 };
const tImage Right_32x32x4          = { (void *)right_32x32x4, 32, 32, GREYSCALE4 };
const tImage Refresh_32x32x4        = { (void *)refresh_32x32x4, 32, 32, GREYSCALE4 };
const tImage Leveling_32x32x4       = { (void *)leveling_32x32x4, 32, 32, GREYSCALE4 };

const tImage ModernHome32_32x32x4 = { (void *)modern_home_32x32x4, 32, 32, GREYSCALE4 };
const tImage ModernTemp64_64x64x4   = { (void *)modern_temp_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernMove64_64x64x4   = { (void *)modern_move_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernPrint64_64x64x4  = { (void *)modern_print_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernSD64_64x64x4     = { (void *)modern_sd_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernExtruder64_64x64x4 = { (void *)modern_extruder_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernFan64_64x64x4    = { (void *)modern_fan_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernMesh64_64x64x4   = { (void *)modern_mesh_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernSettings64_64x64x4 = { (void *)modern_settings_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernAbout64_64x64x4  = { (void *)modern_about_76x76x4, 76, 76, GREYSCALE4 };
const tImage ModernGlobe32_32x32x4 = { (void *)modern_globe_32x32x4, 32, 32, GREYSCALE4 };
const tImage ModernSun32_32x32x4 = { (void *)modern_sun_32x32x4, 32, 32, GREYSCALE4 };
const tImage ModernSpeaker32_32x32x4 = { (void *)modern_speaker_32x32x4, 32, 32, GREYSCALE4 };
const tImage ModernClock32_32x32x4 = { (void *)modern_clock_32x32x4, 32, 32, GREYSCALE4 };
const tImage ModernWrench32_32x32x4 = { (void *)modern_wrench_32x32x4, 32, 32, GREYSCALE4 };
const tImage ModernBenchy96_96x96x4 = { (void *)modern_benchy_96x96x4, 96, 96, GREYSCALE4 };
#if ENABLED(SHOW_BOOTSCREEN)
const tImage ModernMarlinLogo112_112x38x1 = { (void *)marlin_logo_112x38x1, 112, 38, GREYSCALE1 };
#endif
const tImage ModernWifi24_24x24x4 = { (void *)modern_wifi_24x24x4, 24, 24, GREYSCALE4 };

const tImage Slider8x16x4           = { (void *)slider_8x16x4, 8, 16, GREYSCALE4 };

const tImage Images[imgCount] = {
  TERN(SHOW_BOOTSCREEN, TERN(BOOT_MARLIN_LOGO_SMALL, MarlinLogo195x59x16, MARLIN_LOGO_FULL_SIZE), NoLogo),
  HotEnd_64x64x4,
  Bed_64x64x4,
  Bed_Heated_64x64x4,
  Chamber_64x64x4,
  Chamber_Heated_64x64x4,
  Motor_64x64x4,
  HotEnd_32x32x4,
  Bed_32x32x4,
  Fan_Fast_32x32x4,
  SD_32x32x4,
  Settings_32x32x4,
  Menu_32x32x4,
  Motor_32x32x4,
  Home_32x32x4,
  ModernHome32_32x32x4,
  ModernTemp64_64x64x4,
  ModernMove64_64x64x4,
  ModernPrint64_64x64x4,
  ModernSD64_64x64x4,
  ModernExtruder64_64x64x4,
  ModernFan64_64x64x4,
  ModernMesh64_64x64x4,
  ModernSettings64_64x64x4,
  ModernAbout64_64x64x4,
  ModernGlobe32_32x32x4,
  ModernSun32_32x32x4,
  ModernSpeaker32_32x32x4,
  ModernClock32_32x32x4,
  ModernWrench32_32x32x4,
  ModernBenchy96_96x96x4,
  #if ENABLED(SHOW_BOOTSCREEN)
    ModernMarlinLogo112_112x38x1,
  #else
    NoLogo,
  #endif
  ModernWifi24_24x24x4,
  Fan0_64x64x4,
  Fan_Slow0_64x64x4,
  Fan_Slow1_64x64x4,
  Fan_Fast0_64x64x4,
  Fan_Fast1_64x64x4,
  Feedrate_32x32x4,
  Flowrate_32x32x4,
  SD_64x64x4,
  Menu_64x64x4,
  Settings_64x64x4,
  Directory_32x32x4,
  Confirm_64x64x4,
  Cancel_64x64x4,
  Increase_64x64x4,
  Decrease_64x64x4,
  Back_32x32x4,
  Up_32x32x4,
  Down_32x32x4,
  Left_32x32x4,
  Right_32x32x4,
  Refresh_32x32x4,
  Leveling_32x32x4,
  Slider8x16x4,
  Home_64x64x4,
  BtnRounded_64x52x4,
  BtnRounded_42x39x4,
};

#endif // HAS_GRAPHICAL_TFT
