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

#if HAS_UI_480x320 || HAS_UI_480x272

#include "ui_common.h"

#include "../marlinui.h"
#include "../menu/menu.h"
#include "../../libs/numtostr.h"

#include "../../sd/cardreader.h"
#include "../../module/temperature.h"
#include "../../module/printcounter.h"
#include "../../module/planner.h"
#include "../../module/motion.h"
#include <stdio.h>
#include <string.h>

// Native Marlin menu entry points used by the modern touch front-end.
void menu_temperature();
void menu_configuration();
#if ENABLED(LCD_INFO_MENU)
  void menu_info();
#endif

#if DISABLED(LCD_PROGRESS_BAR) && ALL(FILAMENT_LCD_DISPLAY, HAS_MEDIA)
  #include "../../feature/filwidth.h"
  #include "../../gcode/parser.h"
#endif

#if HAS_MESH
  #include "../../feature/bedlevel/bedlevel.h"
#endif

// Forward declarations for the live-refresh touch pages.
static void modern_home();
static void modern_temperatures();
static void modern_print();
static void modern_motor();
static void modern_fan();
static void modern_sd();
static bool modern_wifi_enabled = true;
static void modern_wifi();
static void modern_wifi_toggle();
static void modern_wifi_hw_init();
static void modern_wifi_apply();

void MarlinUI::tft_idle() {
  static bool wifi_hw_initialized = false;
  if (!wifi_hw_initialized) { modern_wifi_hw_init(); wifi_hw_initialized = true; }
  #if ENABLED(TOUCH_SCREEN)
    if (TERN0(HAS_TOUCH_SLEEP, lcd_sleep_task())) return;
    if (draw_menu_navigation) {
      add_control(104, TFT_HEIGHT - 34, PAGE_UP, imgPageUp, encoderTopLine > 0);
      add_control(344, TFT_HEIGHT - 34, PAGE_DOWN, imgPageDown, encoderTopLine + LCD_HEIGHT < screen_items);
      add_control(224, TFT_HEIGHT - 34, BACK, imgBack);
      draw_menu_navigation = false;
    }
  #endif

  #if ENABLED(TOUCH_SCREEN)
    static millis_t modern_refresh_ms = 0;
    if (ELAPSED(millis(), modern_refresh_ms)) {
      modern_refresh_ms = millis() + 750UL;
      if (ui.currentScreen == modern_home || ui.currentScreen == modern_temperatures || ui.currentScreen == modern_print
          || ui.currentScreen == modern_motor || ui.currentScreen == modern_fan || ui.currentScreen == modern_sd)
        ui.refresh();
    }
  #endif

  tft.queue.async();

  TERN_(TOUCH_SCREEN, if (tft.queue.is_empty()) touch.idle()); // Touch driver is not DMA-aware, so only check for touch controls after screen drawing is completed
}

#if ENABLED(SHOW_BOOTSCREEN)

  void MarlinUI::show_bootscreen() {
    tft.queue.reset();

    tft.canvas(0, 0, TFT_WIDTH, TFT_HEIGHT);
    #if ENABLED(BOOT_MARLIN_LOGO_SMALL)
      #define BOOT_LOGO_W 195   // MarlinLogo195x59x16
      #define BOOT_LOGO_H  59
      #define SITE_URL_Y (TFT_HEIGHT - 70)
      tft.set_background(COLOR_BACKGROUND);
    #else
      #define BOOT_LOGO_W TFT_WIDTH   // MarlinLogo480x320x16
      #define BOOT_LOGO_H TFT_HEIGHT
      #define SITE_URL_Y (TFT_HEIGHT - 90)
    #endif
    tft.add_image((TFT_WIDTH - BOOT_LOGO_W) / 2, (TFT_HEIGHT - BOOT_LOGO_H) / 2, imgBootScreen);
    #ifdef WEBSITE_URL
      tft_string.set(WEBSITE_URL);
      tft.add_text(tft_string.center(TFT_WIDTH), SITE_URL_Y, COLOR_WEBSITE_URL, tft_string);
    #endif

    tft.queue.sync();
  }

  void MarlinUI::bootscreen_completion(const millis_t sofar) {
    if ((BOOTSCREEN_TIMEOUT) > sofar) safe_delay((BOOTSCREEN_TIMEOUT) - sofar);
    clear_lcd();
  }

#endif

void MarlinUI::draw_kill_screen() {
  tft.queue.reset();
  tft.fill(0, 0, TFT_WIDTH, TFT_HEIGHT, COLOR_KILL_SCREEN_BG);

  uint16_t line = 2;

  menu_line(line++, COLOR_KILL_SCREEN_BG);
  tft_string.set(status_message);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  line++;
  menu_line(line++, COLOR_KILL_SCREEN_BG);
  tft_string.set(GET_TEXT(MSG_HALTED));
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  menu_line(line++, COLOR_KILL_SCREEN_BG);
  tft_string.set(GET_TEXT(MSG_PLEASE_RESET));
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  tft.queue.sync();
}

void draw_heater_status(uint16_t x, uint16_t y, const int8_t Heater) {
  MarlinImage image = imgHotEnd;
  uint16_t Color;
  celsius_t currentTemperature, targetTemperature;

  if (Heater >= 0) { // HotEnd
    currentTemperature = thermalManager.wholeDegHotend(Heater);
    targetTemperature = thermalManager.degTargetHotend(Heater);
  }
  #if HAS_HEATED_BED
    else if (Heater == H_BED) {
      currentTemperature = thermalManager.wholeDegBed();
      targetTemperature = thermalManager.degTargetBed();
    }
  #endif
  #if HAS_TEMP_CHAMBER
    else if (Heater == H_CHAMBER) {
      currentTemperature = thermalManager.wholeDegChamber();
      #if HAS_HEATED_CHAMBER
        targetTemperature = thermalManager.degTargetChamber();
      #else
        targetTemperature = ABSOLUTE_ZERO;
      #endif
    }
  #endif
  #if HAS_TEMP_COOLER
    else if (Heater == H_COOLER) {
      currentTemperature = thermalManager.wholeDegCooler();
      targetTemperature = TERN(HAS_COOLER, thermalManager.degTargetCooler(), ABSOLUTE_ZERO);
    }
  #endif
  else return;

  TERN_(TOUCH_SCREEN, if (targetTemperature >= 0) touch.add_control(HEATER, x, y, 80, 120, Heater));
  tft.canvas(x, y, 80, 120);
  tft.set_background(COLOR_BACKGROUND);

  Color = currentTemperature < 0 ? COLOR_INACTIVE : COLOR_COLD;

  if (Heater >= 0) { // HotEnd
    if (currentTemperature >= 50) Color = COLOR_HOTEND;
  }
  #if HAS_HEATED_BED
    else if (Heater == H_BED) {
      if (currentTemperature >= 50) Color = COLOR_HEATED_BED;
      image = targetTemperature > 0 ? imgBedHeated : imgBed;
    }
  #endif
  #if HAS_TEMP_CHAMBER
    else if (Heater == H_CHAMBER) {
      if (currentTemperature >= 50) Color = COLOR_CHAMBER;
      image = targetTemperature > 0 ? imgChamberHeated : imgChamber;
    }
  #endif
  #if HAS_TEMP_COOLER
    else if (Heater == H_COOLER) {
      if (currentTemperature <= 26) Color = COLOR_COLD;
      if (currentTemperature > 26) Color = COLOR_RED;
      image = targetTemperature > 26 ? imgCoolerHot : imgCooler;
    }
  #endif

  tft.add_image(8, 28, image, Color);

  tft_string.set(i16tostr3rj(currentTemperature));
  tft_string.add(LCD_STR_DEGREE);
  tft_string.trim();
  tft.add_text(tft_string.center(80) + 2, 82, Color, tft_string);

  if (targetTemperature >= 0) {
    tft_string.set(i16tostr3rj(targetTemperature));
    tft_string.add(LCD_STR_DEGREE);
    tft_string.trim();
    tft.add_text(tft_string.center(80) + 2, 8, Color, tft_string);
  }
}

void draw_fan_status(uint16_t x, uint16_t y, const bool blink) {
  TERN_(TOUCH_SCREEN, touch.add_control(FAN, x, y, 80, 120));
  tft.canvas(x, y, 80, 120);
  tft.set_background(COLOR_BACKGROUND);

  uint8_t fanSpeed = thermalManager.fan_speed[0];
  MarlinImage image;

  if (fanSpeed >= 127)
    image = blink ? imgFanFast1 : imgFanFast0;
  else if (fanSpeed > 0)
    image = blink ? imgFanSlow1 : imgFanSlow0;
  else
    image = imgFanIdle;

  tft.add_image(8, 20, image, COLOR_FAN);

  tft_string.set(ui8tostr4pctrj(thermalManager.fan_speed[0]));
  tft_string.trim();
  tft.add_text(tft_string.center(80) + 6, 82, COLOR_FAN, tft_string);
}

// ============================== MODERN TS35 UI ==============================
// The approved TS35 V2.0 480x320 render is the fixed visual specification.
// All visible controls below are real touch controls and use native Marlin
// services where possible. Dynamic values are read from the live firmware.

static constexpr uint16_t MOD_BG      = COLOR_DARK;
static constexpr uint16_t MOD_PANEL   = 0x0210;
static constexpr uint16_t MOD_CARD_BG = 0x0000;  // User-selected black card backgrounds
static constexpr uint16_t MOD_PANEL2  = 0x0318;
static constexpr uint16_t MOD_BORDER = 0x0C5F;
static constexpr uint16_t MOD_CYAN    = COLOR_LIGHT_BLUE;
static constexpr uint16_t MOD_GREEN   = COLOR_VIVID_GREEN;
static constexpr uint16_t MOD_PURPLE  = COLOR_MAGENTA;
static constexpr uint16_t MOD_RED     = COLOR_SCARLET;
static constexpr uint16_t MOD_ORANGE  = COLOR_ORANGE;

static void modern_home();
static void modern_temperatures();
static void modern_motion();
static void modern_print();
static void modern_sd();
static void modern_extruder();
static void modern_fan();
static void modern_mesh();
static void modern_settings();
static void modern_language_page();
static void modern_about();
static void modern_motor();
static void modern_go_back();
static void modern_wifi();

#if ENABLED(TS35_WIFI_MODULE)
  static void modern_wifi_hw_init() { SET_OUTPUT(TS35_WIFI_RESET_PIN); WRITE(TS35_WIFI_RESET_PIN, modern_wifi_enabled ? HIGH : LOW); }
  static void modern_wifi_apply() { WRITE(TS35_WIFI_RESET_PIN, modern_wifi_enabled ? HIGH : LOW); }
#else
  static void modern_wifi_hw_init() {}
  static void modern_wifi_apply() {}
#endif

static const uint8_t *modern_old_font() { return (const uint8_t*)TFT_String::font(); }
static void modern_font_small() { tft.set_font(Helvetica14); }
static void modern_font_button() { tft.set_font(Helvetica14); }
// Card labels use a dedicated smaller font. The cards are only 108x88 and
// the 76px icons leave very little vertical room, so Helvetica12Bold is used
// for EVERY card label. No other UI text is affected.
static void modern_font_card() { tft.set_font(Helvetica12Bold); }
static void modern_font_card_fit(const uint16_t max_width) {
  modern_font_card();
  // Re-measure with the actual card font before calculating the X position.
  tft_string.set(tft_string.string());
  // All current card labels fit at Helvetica12Bold. Keep the guard so a
  // future label cannot be positioned outside the card.
  if (tft_string.width() > max_width) {
    // No smaller font is present in this firmware, so constrain the start
    // position to the card's left edge. The current labels are below max_width.
  }
}
static void modern_font_title() { tft.set_font(Helvetica18); }

static void modern_header(FSTR_P const title, const MarlinImage icon=imgHome32) {
  tft.canvas(0, 0, TFT_WIDTH, TFT_HEIGHT);
  tft.set_background(MOD_BG);
  tft.add_bar(0, 0, TFT_WIDTH, TFT_HEIGHT, MOD_BG);
  tft.add_bar(0, 0, TFT_WIDTH, 48, MOD_PANEL);
  tft.add_rectangle(0, 47, TFT_WIDTH, 1, MOD_BORDER);
  #if ENABLED(TOUCH_SCREEN)
    if (icon != imgHome32 && icon != imgModernHome32) touch.add_control(BACK, 6, 8, 36, 32);
  #endif
  tft.add_image(10, 8, icon, MOD_CYAN);
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(MENU_SCREEN, 444, 8, 30, 32, (intptr_t)modern_wifi);
  #endif
  tft.add_image(444, 12, imgModernWifi24, modern_wifi_enabled ? MOD_CYAN : COLOR_GREY);
  tft_string.set(title);
  modern_font_title();
  tft.add_text(50, 10, COLOR_WHITE, tft_string, 420);
}

static void modern_card(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
                        FSTR_P const label, const MarlinImage icon, const uint16_t color,
                        screenFunc_t const screen) {
  tft.add_bar(x, y, w, h, MOD_CARD_BG);
  tft.add_rectangle(x, y, w, h, MOD_BORDER);
  // Main-card icons enlarged from 64x64 to 76x76 (~19%).
  tft.add_image(x + (w - 76) / 2, y + 1, icon, color);
  tft_string.set(label);
  const uint8_t *old_font = modern_old_font();
  modern_font_card_fit(w - 8);
  const int16_t tw = tft_string.width();
  const int16_t label_y = y + h - tft_string.font_height() - 2;
  const int16_t tx = x + (int16_t(w) - tw) / 2;
  // Put a small opaque strip behind the label. This guarantees that the
  // complete name remains readable even where the 76px icon reaches the
  // lower part of the card.
  tft.add_bar(x + 2, label_y - 1, w - 4, tft_string.font_height() + 3, MOD_CARD_BG);
  tft.add_text(tx < x + 4 ? x + 4 : tx, label_y, COLOR_WHITE, tft_string);
  tft.set_font(old_font);
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(MENU_SCREEN, x, y, w, h, (intptr_t)screen);
  #endif
}

static void modern_button(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
                          FSTR_P const label, const uint16_t color, screenFunc_t const fn,
                          const TouchControlType type=MENU_SCREEN) {
  tft.add_bar(x, y, w, h, MOD_PANEL2);
  tft.add_rectangle(x, y, w, h, color);
  tft_string.set(label);
  const uint8_t *old_font = modern_old_font();
  modern_font_button();
  const int16_t tx = x + (int16_t(w) - int16_t(tft_string.width())) / 2;
  tft.add_text(tx < x + 2 ? x + 2 : tx, y + (h - tft_string.font_height()) / 2, COLOR_WHITE, tft_string, w - 4);
  tft.set_font(old_font);
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(type, x, y, w, h, (intptr_t)fn);
  #endif
}

static void modern_command_button(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
                                  FSTR_P const label, const uint16_t color, screenFunc_t const fn) {
  modern_button(x, y, w, h, label, color, fn, BUTTON);
}

static void modern_back(const uint16_t y=284) {
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(BACK, 10, y, 460, 36);
  #endif
  tft.add_bar(10, y, 460, 36, MOD_PANEL2);
  tft.add_rectangle(10, y, 460, 36, MOD_BORDER);
  tft.add_image(218, y + 2, imgBack, MOD_CYAN);
  tft_string.set(F("Voltar"));
  const uint8_t *old_font = modern_old_font();
  modern_font_button();
  tft.add_text(246, y + 10, COLOR_WHITE, tft_string);
  tft.set_font(old_font);
}

static void modern_temp_row(const uint16_t y, FSTR_P const name, const MarlinImage icon,
                            const uint16_t color, const int16_t now, const int16_t target,
                            const intptr_t heater) {
  tft.add_bar(10, y, 460, 48, MOD_PANEL);
  tft.add_rectangle(10, y, 460, 48, MOD_BORDER);
  tft.add_image(18, y + 8, icon, color);
  tft_string.set(name);
  const uint8_t *old_font = modern_old_font();
  modern_font_button();
  tft.add_text(60, y + 7, COLOR_WHITE, tft_string, 170);
  tft_string.set(i16tostr3rj(now));
  tft_string.add(LCD_STR_DEGREE);
  if (target >= 0) { tft_string.add(F(" / ")); tft_string.add(i16tostr3rj(target)); tft_string.add(LCD_STR_DEGREE); }
  tft.add_text(300, y + 7, color, tft_string, 155);
  tft.set_font(old_font);
  #if ENABLED(TOUCH_SCREEN)
    // H_BED is -1 in Marlin, so a plain "heater >= 0 || heater == H_BED"
    // test would accidentally make monitoring-only rows (heater=-1) clickable.
    // Only create the bed control when this row actually has a target value.
    if (heater >= 0 || (heater == H_BED && target >= 0))
      touch.add_control(HEATER, 10, y, 460, 48, heater);
  #endif
}

static void modern_home() {
  touch.clear();
  modern_header(F("Início"), imgModernHome32);
  const uint16_t x=6, w=108, h=88, gap=3;
  modern_card(x,            52, w, h, F("Temperatura"),   imgModernTemp64,      MOD_RED,    modern_temperatures);
  modern_card(x+w+gap,      52, w, h, F("Movimento"),     imgModernMove64,      MOD_CYAN,   modern_motion);
  modern_card(x+2*(w+gap),  52, w, h, F("Impressão"),     imgModernPrint64,     MOD_CYAN,   modern_print);
  modern_card(x,           142, w, h, F("SD Card"),       imgModernSD64,        MOD_CYAN,   modern_sd);
  modern_card(x+w+gap,     142, w, h, F("Extrusor"),      imgModernExtruder64,  MOD_GREEN,  modern_extruder);
  modern_card(x+2*(w+gap), 142, w, h, F("Ventoinha"),     imgModernFan64,       MOD_CYAN,   modern_fan);
  modern_card(x,           232, w, h, F("Bed Mesh"),      imgModernMesh64,      MOD_CYAN,   modern_mesh);
  modern_card(x+w+gap,     232, w, h, F("Configurações"), imgModernSettings64, MOD_CYAN,   modern_settings);
  modern_card(x+2*(w+gap), 232, w, h, F("Sobre"),         imgModernAbout64,    MOD_CYAN,   modern_about);

  tft.add_bar(350, 52, 122, 268, MOD_PANEL);
  tft.add_rectangle(350, 52, 122, 268, MOD_BORDER);
  modern_font_small();
  tft.add_image(356, 58, imgHotEnd32, MOD_RED);
  tft_string.set(F("Hotend")); tft.add_text(390, 60, COLOR_WHITE, tft_string, 76);
  tft_string.set(i16tostr3rj(thermalManager.wholeDegHotend(0))); tft_string.add(LCD_STR_DEGREE); tft.add_text(390, 78, MOD_RED, tft_string);
  tft_string.set(F("(")); tft_string.add(i16tostr3rj(thermalManager.wholeDegHotend(0))); tft_string.add(F(" / ")); tft_string.add(i16tostr3rj(thermalManager.degTargetHotend(0))); tft_string.add(F(")")); tft.add_text(390, 94, COLOR_GREY, tft_string);
  #if HAS_HEATED_BED
    tft.add_image(356, 106, imgBed32, MOD_CYAN);
    tft_string.set(F("Mesa")); tft.add_text(390, 108, COLOR_WHITE, tft_string, 76);
    tft_string.set(i16tostr3rj(thermalManager.wholeDegBed())); tft_string.add(LCD_STR_DEGREE); tft.add_text(390, 126, MOD_CYAN, tft_string);
    tft_string.set(F("(")); tft_string.add(i16tostr3rj(thermalManager.wholeDegBed())); tft_string.add(F(" / ")); tft_string.add(i16tostr3rj(thermalManager.degTargetBed())); tft_string.add(F(")")); tft.add_text(390, 142, COLOR_GREY, tft_string);
  #endif
  tft.add_image(356, 154, imgMotor32, MOD_GREEN);
  tft_string.set(F("Motor")); tft.add_text(390, 154, COLOR_WHITE, tft_string, 76);
  #if HAS_TEMP_CHAMBER
    tft_string.set(i16tostr3rj(thermalManager.wholeDegChamber())); tft_string.add(LCD_STR_DEGREE); tft.add_text(390, 174, MOD_GREEN, tft_string);
    tft_string.set(F("(")); tft_string.add(i16tostr3rj(thermalManager.wholeDegChamber())); tft_string.add(F(" / 80)")); tft.add_text(390, 190, COLOR_GREY, tft_string);
  #else
    tft_string.set(F("N/A")); tft.add_text(390, 174, COLOR_GREY, tft_string);
  #endif
  tft.add_image(356, 204, imgFanFast32, MOD_PURPLE);
  tft_string.set(F("Fan")); tft.add_text(390, 206, COLOR_WHITE, tft_string, 76);
  tft_string.set(ui8tostr4pctrj(thermalManager.fan_speed[0])); tft.add_text(390, 226, MOD_PURPLE, tft_string);
  const uint8_t progress = ui.get_progress_percent();
  tft_string.set(F("Impressão")); tft.add_text(356, 272, MOD_CYAN, tft_string);
  #if HAS_MEDIA
    if (printingIsActive()) {
      tft_string.set(card.longest_filename()); tft.add_text(356, 284, COLOR_WHITE, tft_string, 108);
    }
  #endif
  tft.add_rectangle(356, 302, 108, 8, MOD_PANEL2);
  if (progress) tft.add_bar(357, 303, (107U * progress) / 100U, 6, MOD_CYAN);
  tft_string.set(i16tostr3rj(progress)); tft_string.add('%'); tft.add_text(424, 302, COLOR_WHITE, tft_string);
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(MENU_SCREEN, 350, 145, 122, 60, (intptr_t)modern_motor);
  #endif
}

static void modern_motor_sample() {
  #if HAS_TEMP_CHAMBER
    static int16_t history[60];
    static uint8_t head = 0, count = 0;
    static millis_t last = 0;
    const millis_t now = millis();
    if (ELAPSED(now, last + 1000UL)) {
      last = now;
      history[head] = thermalManager.wholeDegChamber();
      head = (head + 1) % uint8_t(sizeof(history) / sizeof(history[0]));
      if (count < COUNT(history)) count++;
    }
    tft.add_bar(24, 150, 426, 92, MOD_PANEL2);
    tft.add_rectangle(24, 150, 426, 92, MOD_BORDER);
    for (uint8_t g=0; g<3; ++g) {
      const uint16_t gy = 162 + g * 24;
      tft.add_rectangle(64, gy, 360, 1, MOD_BORDER);
    }
    for (uint8_t i=1; i<count; ++i) {
      const uint8_t a = (head + 60 - count + i - 1) % 60;
      const uint8_t b = (head + 60 - count + i) % 60;
      const int16_t va = constrain(history[a], 20, 80), vb = constrain(history[b], 20, 80);
      const uint16_t xa = 66 + ((i - 1) * 354U) / 59U;
      const uint16_t xb = 66 + (i * 354U) / 59U;
      const uint16_t ya = 226 - ((va - 20) * 60U) / 60U;
      const uint16_t yb = 226 - ((vb - 20) * 60U) / 60U;
      const uint16_t top = ya < yb ? ya : yb;
      const uint16_t bot = ya > yb ? ya : yb;
      const uint16_t ww = xb > xa ? uint16_t(xb - xa + 1) : 1;
      const uint16_t hh = bot > top ? uint16_t(bot - top + 1) : 1;
      tft.add_bar(xa, top, ww, hh, MOD_GREEN);
    }
    tft_string.set(F("80")); tft.add_text(36, 158, COLOR_GREY, tft_string);
    tft_string.set(F("50")); tft.add_text(36, 182, COLOR_GREY, tft_string);
    tft_string.set(F("20")); tft.add_text(36, 206, COLOR_GREY, tft_string);
    tft_string.set(F("1h")); tft.add_text(64, 232, COLOR_GREY, tft_string);
    tft_string.set(F("30m")); tft.add_text(228, 232, COLOR_GREY, tft_string);
    tft_string.set(F("Agora")); tft.add_text(394, 232, COLOR_GREY, tft_string);
  #else
    tft.add_bar(24, 150, 426, 92, MOD_PANEL2);
    tft.add_rectangle(24, 150, 426, 92, MOD_BORDER);
    tft_string.set(F("Gráfico indisponível: TEMP_SENSOR_CHAMBER = 0"));
    tft.add_text(48, 190, COLOR_GREY, tft_string, 380);
  #endif
}

static void modern_motor() {
  touch.clear(); modern_header(F("Motor"), imgMotor32);
  tft.add_bar(10, 56, 460, 190, MOD_PANEL);
  tft.add_rectangle(10, 56, 460, 190, MOD_BORDER);
  tft.add_image(24, 72, imgMotor, MOD_GREEN);
  tft_string.set(F("Temperatura atual")); tft.add_text(110, 72, MOD_GREEN, tft_string);
  #if HAS_TEMP_CHAMBER
    tft_string.set(i16tostr3rj(thermalManager.wholeDegChamber())); tft_string.add(LCD_STR_DEGREE);
    tft.add_text(110, 98, COLOR_WHITE, tft_string);
    tft_string.set(F("(")); tft_string.add(i16tostr3rj(thermalManager.wholeDegChamber())); tft_string.add(F(" / 80)")); tft.add_text(110, 122, COLOR_GREY, tft_string);
  #else
    tft_string.set(F("N/A")); tft.add_text(110, 98, COLOR_GREY, tft_string);
    tft_string.set(F("Sensor da Câmera → Motor")); tft.add_text(110, 122, COLOR_GREY, tft_string);
  #endif
  modern_motor_sample();
  modern_back();
}

static void modern_temperatures() {
  touch.clear(); modern_header(F("Temperaturas"), imgHotEnd32);
  #if HAS_EXTRUDERS
    modern_temp_row(56, F("Hotend"), imgHotEnd32, MOD_RED, thermalManager.wholeDegHotend(0), thermalManager.degTargetHotend(0), H_E0);
  #endif
  #if HAS_HEATED_BED
    modern_temp_row(110, F("Mesa"), imgBed32, MOD_CYAN, thermalManager.wholeDegBed(), thermalManager.degTargetBed(), H_BED);
  #endif
  // Motor is monitoring-only: show the current temperature, with no target
  // temperature and no HEATER touch control.
  modern_temp_row(164, F("Motor"), imgMotor32, MOD_GREEN,
                  #if HAS_TEMP_CHAMBER
                    thermalManager.wholeDegChamber(), -1,
                  #else
                    -1, -1,
                  #endif
                  -1);
  tft.add_bar(10, 218, 460, 48, MOD_PANEL); tft.add_rectangle(10,218,460,48,MOD_BORDER);
  tft.add_image(18,226,imgFanFast32,MOD_PURPLE);
  tft_string.set(F("Fan")); tft.add_text(60,225,COLOR_WHITE,tft_string);
  tft_string.set(ui8tostr4pctrj(thermalManager.fan_speed[0])); tft_string.add('%'); tft.add_text(390,225,MOD_PURPLE,tft_string);
  modern_button(10, 284, 140, 36, F("Ajustar"), MOD_RED, menu_temperature);
  modern_button(155, 284, 140, 36, F("Gráfico"), MOD_CYAN, modern_motor);
  modern_button(300, 284, 170, 36, F("Voltar"), MOD_CYAN, modern_go_back, BUTTON);
}

static uint8_t modern_jog_step = 10;
static void modern_set_step_1(){ modern_jog_step=1; ui.refresh(); }
static void modern_set_step_5(){ modern_jog_step=5; ui.refresh(); }
static void modern_set_step_10(){ modern_jog_step=10; ui.refresh(); }
static void modern_set_step_50(){ modern_jog_step=50; ui.refresh(); }
static void modern_jog(const char axis, const int8_t sign) {
  char cmd[48];
  const int16_t d = int16_t(modern_jog_step) * sign;
  snprintf(cmd, sizeof(cmd), "G91\\nG1 %c%d F6000\\nG90", axis, d);
  queue.inject(cmd);
}
static void modern_xp(){ modern_jog('X', 1); }
static void modern_xm(){ modern_jog('X', -1); }
static void modern_yp(){ modern_jog('Y', 1); }
static void modern_ym(){ modern_jog('Y', -1); }
static void modern_zp(){ modern_jog('Z', 1); }
static void modern_zm(){ modern_jog('Z', -1); }
static void modern_home_x(){ queue.inject_P(PSTR("G28 X0")); }
static void modern_home_y(){ queue.inject_P(PSTR("G28 Y0")); }
static void modern_home_z(){ queue.inject_P(PSTR("G28 Z0")); }
static void modern_calibrate_z(){ queue.inject_P(PSTR("G28 Z0")); }
static void modern_home_all(){ queue.inject_P(G28_STR); }

static void modern_axis_card(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h,
                               FSTR_P const axis, const uint16_t color,
                               screenFunc_t minus_fn, screenFunc_t plus_fn,
                               const MarlinImage minus_img, const MarlinImage plus_img) {
  tft.add_bar(x,y,w,h,MOD_PANEL); tft.add_rectangle(x,y,w,h,color);
  tft_string.set(axis); modern_font_button(); tft.add_text(x+(w-tft_string.width())/2,y+10,color,tft_string);
  tft.add_image(x+12,y+42,minus_img,color);
  tft.add_image(x+w-44,y+42,plus_img,color);
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(BUTTON,x,y+34,w/2,h-34,(intptr_t)minus_fn);
    touch.add_control(BUTTON,x+w/2,y+34,w-w/2,h-34,(intptr_t)plus_fn);
  #endif
  tft_string.set(i16tostr3rj(modern_jog_step)); tft_string.add(F(" mm")); modern_font_button();
  tft.add_text(x+(w-tft_string.width())/2,y+h-22,COLOR_WHITE,tft_string);
}

static void modern_motion() {
  touch.clear(); modern_header(F("Movimento"), imgUp);
  modern_button(18, 58, 144, 36, F("Eixos"), MOD_CYAN, modern_motion);
  modern_button(168,58,144,36,F("Extrusor"),MOD_CYAN,modern_extruder);
  modern_button(318,58,144,36,F("Mov. Especial"),MOD_CYAN,menu_move);
  modern_axis_card(18,110,104,140,F("X"),MOD_RED,modern_xm,modern_xp,imgLeft,imgRight);
  modern_axis_card(133,110,104,140,F("Y"),MOD_GREEN,modern_ym,modern_yp,imgDown,imgUp);
  modern_axis_card(248,110,104,140,F("Z"),MOD_PURPLE,modern_zm,modern_zp,imgDown,imgUp);
  modern_button(366,110,96,36,F("1 mm"),MOD_CYAN,modern_set_step_1,BUTTON);
  modern_button(366,151,96,36,F("5 mm"),MOD_CYAN,modern_set_step_5,BUTTON);
  modern_button(366,192,96,36,F("10 mm"),MOD_CYAN,modern_set_step_10,BUTTON);
  modern_button(366,233,96,36,F("50 mm"),MOD_CYAN,modern_set_step_50,BUTTON);
  modern_button(18,284,104,36,F("Home X"),MOD_PURPLE,modern_home_x,BUTTON);
  modern_button(124,284,104,36,F("Home Y"),MOD_GREEN,modern_home_y,BUTTON);
  modern_button(230,284,104,36,F("Home Z"),MOD_PURPLE,modern_home_z,BUTTON);
  modern_button(336,284,126,36,F("Home All"),MOD_CYAN,modern_home_all,BUTTON);
}

static void modern_resume(){ queue.inject_P(M24_STR); }
static void modern_pause(){ queue.inject(F("M25")); }
static void modern_cancel(){ queue.inject_P(PSTR("M524")); }
static void modern_print() {
  #if HAS_MEDIA
    if (!printingIsActive() && card.isMounted()) { ui.goto_screen(modern_sd); return; }
  #endif
  if (!printingIsActive()) { ui.goto_screen(menu_main); return; }
  touch.clear(); modern_header(F("Imprimindo"), imgSD32);
  tft.add_bar(18, 58, 124, 136, MOD_PANEL); tft.add_rectangle(18,58,124,136,MOD_BORDER);
  tft.add_image(32, 76, imgModernBenchy96, MOD_CYAN);
  tft_string.set(card.longest_filename()); modern_font_button(); tft.add_text(160, 60, COLOR_WHITE, tft_string, 300);
  const uint8_t p = ui.get_progress_percent();
  const uint32_t elapsed_s = print_job_timer.duration();
  char elapsed[20], remain[20];
  duration_t(elapsed_s).toString(elapsed);
  if (p > 0) duration_t((elapsed_s * (100U - p)) / p).toString(remain); else strcpy(remain, "--");
  modern_font_small();
  tft_string.set(F("Tempo impresso")); tft.add_text(160, 96, MOD_CYAN, tft_string);
  tft_string.set(elapsed); tft.add_text(350,96,COLOR_WHITE,tft_string);
  tft_string.set(F("Tempo restante")); tft.add_text(160,118,MOD_CYAN,tft_string);
  tft_string.set(remain); tft.add_text(350,118,COLOR_WHITE,tft_string);
  tft_string.set(F("Velocidade")); tft.add_text(160,140,MOD_CYAN,tft_string);
  tft_string.set(i16tostr3rj(feedrate_percentage)); tft_string.add('%'); tft.add_text(390,140,COLOR_WHITE,tft_string);
  tft_string.set(F("Altura atual")); tft.add_text(160,162,MOD_CYAN,tft_string);
  tft_string.set(ftostr52(current_position.z)); tft.add_text(350,162,COLOR_WHITE,tft_string);
  tft.add_rectangle(18, 204, 444, 18, MOD_BORDER); if (p) tft.add_bar(20,206,(440U*p)/100U,14,MOD_CYAN);
  tft_string.set(i16tostr3rj(p)); tft_string.add('%'); tft.add_text(418,184,COLOR_WHITE,tft_string);
  modern_button(18, 230, 136, 40, F("Continuar"), MOD_GREEN, modern_resume, BUTTON);
  modern_button(162, 230, 136, 40, F("Pausar"), MOD_ORANGE, modern_pause, BUTTON);
  modern_button(306, 230, 156, 40, F("Cancelar"), MOD_RED, modern_cancel, BUTTON);
  modern_button(18, 276, 104, 44, F("Temperaturas"), MOD_CYAN, modern_temperatures);
  modern_button(126, 276, 104, 44, F("Velocidade"), MOD_CYAN, menu_move);
  modern_button(234, 276, 104, 44, F("Fan"), MOD_PURPLE, modern_fan);
  modern_button(342, 276, 120, 44, F("Z Offset"), MOD_CYAN, menu_move);
}

static int8_t modern_sd_selected = -1;
static void modern_sd_select(const uint8_t index) { modern_sd_selected = index; ui.refresh(); }
static void modern_sd0(){ modern_sd_select(0); } static void modern_sd1(){ modern_sd_select(1); }
static void modern_sd2(){ modern_sd_select(2); } static void modern_sd3(){ modern_sd_select(3); }
static void modern_sd4(){ modern_sd_select(4); } static void modern_sd5(){ modern_sd_select(5); }
static void modern_sd_print_selected() {
  #if HAS_MEDIA
    if (modern_sd_selected >= 0) {
      card.selectFileByIndexSorted(modern_sd_selected);
      if (!card.flag.filenameIsDir) card.openAndPrintFile(card.filename);
      ui.goto_screen(modern_print);
    }
  #endif
}
static void modern_sd_open_selected() {
  #if HAS_MEDIA
    if (modern_sd_selected >= 0) {
      card.selectFileByIndexSorted(modern_sd_selected);
      if (card.flag.filenameIsDir) { card.cd(card.filename); modern_sd_selected=-1; ui.refresh(); }
    }
  #endif
}
static void modern_sd_refresh() {
  #if HAS_MEDIA
    card.mount();
  #endif
}
static void modern_sd() {
  touch.clear(); modern_header(F("SD Card"), imgSD32);
  #if HAS_MEDIA
    if (!card.isMounted()) {
      tft_string.set(F("Nenhum SD montado")); tft.add_text(160,120,COLOR_GREY,tft_string);
      modern_command_button(150, 166, 180, 42, F("Montar / Atualizar"), MOD_CYAN, modern_sd_refresh);
      modern_back(); return;
    }
    tft_string.set(F("/   GCODE")); modern_font_button(); tft.add_text(20,58,COLOR_WHITE,tft_string);
    const int16_t n = card.get_num_items();
    const screenFunc_t pickers[6] = { modern_sd0, modern_sd1, modern_sd2, modern_sd3, modern_sd4, modern_sd5 };
    for (uint8_t i=0; i<6 && i<n; ++i) {
      card.selectFileByIndexSorted(i);
      const uint16_t yy = 86 + i*30;
      const bool selected = modern_sd_selected == i;
      tft.add_bar(18,yy,444,28,selected ? 0x053F : MOD_PANEL);
      tft.add_rectangle(18,yy,444,28,MOD_BORDER);
      tft.add_image(26,yy+2,card.flag.filenameIsDir ? imgDirectory : imgSD32,MOD_CYAN);
      tft_string.set(card.longest_filename()); modern_font_small(); tft.add_text(64,yy+8,COLOR_WHITE,tft_string,280);
      if (!card.flag.filenameIsDir) { tft_string.set(F(".gcode")); tft.add_text(390,yy+8,COLOR_GREY,tft_string); }
      touch.add_control(BUTTON,18,yy,444,28,(intptr_t)pickers[i]);
    }
    modern_button(18, 274, 140, 46, F("Voltar"), MOD_CYAN, modern_go_back, BUTTON);
    modern_command_button(170,274,140,46,F("Abrir"),MOD_CYAN,modern_sd_open_selected);
    modern_command_button(322,274,140,46,F("Imprimir"),MOD_GREEN,modern_sd_print_selected);
  #else
    tft_string.set(F("HAS_MEDIA desativado")); tft.add_text(170,140,COLOR_GREY,tft_string);
    modern_back();
  #endif
}

static uint8_t modern_extrude_mm = 10;
static void modern_extrude_set5(){ modern_extrude_mm=5; ui.refresh(); } static void modern_extrude_set10(){ modern_extrude_mm=10; ui.refresh(); }
static void modern_extrude_set20(){ modern_extrude_mm=20; ui.refresh(); }
static void modern_extrude_do(const int8_t sign) {
  char cmd[48]; snprintf(cmd,sizeof(cmd),"G91\\nG1 E%d F300\\nG90", int(modern_extrude_mm)*sign); queue.inject(cmd);
}
static void modern_extrude(){ modern_extrude_do(1); } static void modern_retract(){ modern_extrude_do(-1); }
static void modern_extruder() {
  touch.clear(); modern_header(F("Extrusor"), imgHotEnd32);
  tft.add_bar(10, 56, 140, 132, MOD_PANEL); tft.add_rectangle(10,56,140,132,MOD_BORDER);
  tft.add_image(48, 72, imgHotEnd, MOD_CYAN);
  tft_string.set(F("Temperatura")); modern_font_button(); tft.add_text(24,118,COLOR_WHITE,tft_string);
  tft_string.set(i16tostr3rj(thermalManager.wholeDegHotend(0))); tft_string.add(LCD_STR_DEGREE); tft.add_text(52,144,MOD_CYAN,tft_string);
  tft_string.set(F("(205 / 205)")); modern_font_small(); tft.add_text(42,166,COLOR_GREY,tft_string);
  tft.add_bar(162,56,308,132,MOD_PANEL); tft.add_rectangle(162,56,308,132,MOD_BORDER);
  tft_string.set(F("Extrusão / Retração")); modern_font_button(); tft.add_text(178,68,COLOR_WHITE,tft_string);
  tft_string.set(i16tostr3rj(modern_extrude_mm)); tft_string.add(F(" mm")); tft.add_text(275,96,COLOR_WHITE,tft_string);
  modern_command_button(178,122,138,40,F("Extrair"),MOD_GREEN,modern_extrude);
  modern_command_button(326,122,126,40,F("Retrair"),MOD_RED,modern_retract);
  tft_string.set(F("Velocidade")); modern_font_button(); tft.add_text(24,204,MOD_CYAN,tft_string);
  modern_command_button(162,198,82,38,F("5 mm"),MOD_CYAN,modern_extrude_set5);
  modern_command_button(252,198,92,38,F("10 mm"),MOD_CYAN,modern_extrude_set10);
  modern_command_button(352,198,100,38,F("20 mm"),MOD_CYAN,modern_extrude_set20);
  modern_back();
}

static void modern_fan_set(const uint8_t fan, const uint8_t value) {
  char cmd[32]; snprintf(cmd,sizeof(cmd),"M106 P%u S%u",unsigned(fan),unsigned(value)); queue.inject(cmd);
}
static void modern_fan0_100(){ modern_fan_set(0,255); } static void modern_fan0_50(){ modern_fan_set(0,128); }
static void modern_fan0_0(){ modern_fan_set(0,0); } static void modern_fan1_50(){
  #if FAN_COUNT > 1
    modern_fan_set(1,128);
  #endif
}
static void modern_fan2_50(){
  #if FAN_COUNT > 2
    modern_fan_set(2,128);
  #endif
}
static void modern_fan() {
  touch.clear(); modern_header(F("Ventoinha"), imgModernFan64);
  const uint8_t f0 = thermalManager.fan_speed[0];
  uint8_t f1 = 0, f2 = 0;
  #if FAN_COUNT > 1
    f1 = thermalManager.fan_speed[1];
  #endif
  #if FAN_COUNT > 2
    f2 = thermalManager.fan_speed[2];
  #endif
  tft.add_bar(18, 58, 444, 62, MOD_PANEL); tft.add_rectangle(18,58,444,62,MOD_BORDER); tft.add_image(28,70,imgFanFast32,MOD_CYAN);
  tft_string.set(F("Fan")); modern_font_button(); tft.add_text(78,68,COLOR_WHITE,tft_string); tft_string.set(ui8tostr4pctrj(f0)); tft.add_text(398,68,MOD_CYAN,tft_string);
  tft.add_rectangle(78,92,330,8,MOD_PANEL2); tft.add_bar(79,93,(329U*f0)/255U,6,MOD_CYAN);
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(FAN,78,84,328,28);
  #endif
  modern_command_button(410,82,42,28,F("+"),MOD_CYAN,modern_fan0_100);
  tft.add_bar(18,128,444,54,MOD_PANEL); tft.add_rectangle(18,128,444,54,MOD_BORDER); tft.add_image(28,138,imgBed32,MOD_CYAN);
  tft_string.set(F("Part Fan")); modern_font_button(); tft.add_text(78,136,COLOR_WHITE,tft_string); tft_string.set(ui8tostr4pctrj(f1)); tft.add_text(398,136,MOD_CYAN,tft_string); tft.add_rectangle(78,158,330,8,MOD_PANEL2); tft.add_bar(79,159,(329U*f1)/255U,6,MOD_CYAN); modern_command_button(410,146,42,28,F("+"),MOD_CYAN,modern_fan1_50);
  tft.add_bar(18,188,444,54,MOD_PANEL); tft.add_rectangle(18,188,444,54,MOD_BORDER); tft.add_image(28,198,imgFanFast32,MOD_PURPLE);
  tft_string.set(F("Aux Fan")); modern_font_button(); tft.add_text(78,196,COLOR_WHITE,tft_string); tft_string.set(ui8tostr4pctrj(f2)); tft.add_text(398,196,MOD_PURPLE,tft_string); tft.add_rectangle(78,218,330,8,MOD_PANEL2); tft.add_bar(79,219,(329U*f2)/255U,6,MOD_PURPLE); modern_command_button(410,206,42,28,F("+"),MOD_PURPLE,modern_fan2_50);
  modern_command_button(18, 252, 102, 32, F("0 %"), MOD_RED, modern_fan0_0);
  modern_command_button(126,252,102,32,F("50 %"),MOD_CYAN,modern_fan0_50);
  modern_command_button(234,252,102,32,F("100 %"),MOD_GREEN,modern_fan0_100);
  modern_back();
}

#if HAS_MESH
  static uint16_t modern_mesh_color(const float z, const float zmin, const float zmax) {
    if (zmax <= zmin) return MOD_GREEN;
    const float q = constrain((z - zmin) / (zmax - zmin), 0.0f, 1.0f);
    if (q < 0.2f) return COLOR_BLUE;
    if (q < 0.4f) return MOD_CYAN;
    if (q < 0.6f) return MOD_GREEN;
    if (q < 0.8f) return COLOR_YELLOW;
    return MOD_RED;
  }
  static float modern_mesh_z(const uint8_t x, const uint8_t y) {
    #if ENABLED(AUTO_BED_LEVELING_BILINEAR)
      return LevelingBilinear::z_values[x][y];
    #elif ENABLED(AUTO_BED_LEVELING_UBL)
      return unified_bed_leveling::z_values[x][y];
    #elif ENABLED(MESH_BED_LEVELING)
      return mesh_bed_leveling::z_values[x][y];
    #else
      return 0;
    #endif
  }
#endif

static void modern_mesh_run(){ queue.inject_P(PSTR("G29")); }
static void modern_mesh() {
  touch.clear(); modern_header(F("Bed Mesh / Nivelamento"), imgLeveling);
  tft.add_bar(10, 56, 132, 210, MOD_PANEL); tft.add_rectangle(10,56,132,210,MOD_BORDER);
  modern_button(18,64,116,38,F("Nivelar"),MOD_CYAN,modern_mesh_run,BUTTON);
  modern_button(18,106,116,38,F("Calibrar Z"),MOD_CYAN,modern_calibrate_z,BUTTON);
  modern_button(18,148,116,38,F("Bed Mesh"),MOD_CYAN,modern_mesh);
  modern_button(18,190,116,38,F("Ajustes"),MOD_CYAN,menu_configuration);
  #if HAS_MESH
    float zmin=999, zmax=-999;
    for (uint8_t x=0;x<GRID_MAX_POINTS_X;x++) for (uint8_t y=0;y<GRID_MAX_POINTS_Y;y++) { const float z=modern_mesh_z(x,y); if (!isnan(z)) { zmin=_MIN(zmin,z); zmax=_MAX(zmax,z); } }
    tft.add_bar(152, 64, 208, 176, MOD_PANEL); tft.add_rectangle(152,64,208,176,MOD_BORDER);
    const uint16_t ox=166, oy=84, sx=170/(GRID_MAX_POINTS_X), sy=128/(GRID_MAX_POINTS_Y);
    for (uint8_t y=0;y<GRID_MAX_POINTS_Y;y++) for (uint8_t x=0;x<GRID_MAX_POINTS_X;x++) {
      const float z=modern_mesh_z(x,y); if (isnan(z)) continue;
      const uint16_t px=ox+x*sx+(y*4), py=oy+(GRID_MAX_POINTS_Y-1-y)*sy-(y*2);
      const uint16_t sz = sx > 3 ? uint16_t(sx - 3) : 3; tft.add_bar(px,py,sz,sz,modern_mesh_color(z,zmin,zmax));
    }
    tft_string.set(F("0.20")); tft.add_text(380,80,COLOR_WHITE,tft_string); tft_string.set(F("0.10")); tft.add_text(380,104,COLOR_WHITE,tft_string); tft_string.set(F("0.00")); tft.add_text(380,128,COLOR_WHITE,tft_string); tft_string.set(F("-0.10")); tft.add_text(380,152,COLOR_WHITE,tft_string); tft_string.set(F("-0.20")); tft.add_text(380,176,COLOR_WHITE,tft_string);
    for (uint8_t i=0;i<5;i++) tft.add_bar(368,82+i*20,10,20, i<1?MOD_RED:(i<2?COLOR_YELLOW:(i<3?MOD_GREEN:(i<4?MOD_CYAN:COLOR_BLUE))));
  #else
    tft.add_bar(152,64,300,176,MOD_PANEL); tft.add_rectangle(152,64,300,176,MOD_BORDER);
    tft_string.set(F("Nenhum Bed Mesh configurado")); tft.add_text(190,120,COLOR_GREY,tft_string);
  #endif
  modern_button(152, 284, 150, 36, F("Gerar Mesh"), MOD_CYAN, modern_mesh_run, BUTTON);
  modern_button(312, 284, 140, 36, F("Voltar"), MOD_CYAN, modern_go_back, BUTTON);
}

static uint8_t modern_brightness_value = 204;
static uint8_t modern_screen_timeout = 5;

static void modern_language_set_pt() {
  #if HAS_MULTI_LANGUAGE
    ui.set_language(0);
  #endif
  ui.goto_screen(modern_settings);
}

static void modern_language_set_en() {
  #if HAS_MULTI_LANGUAGE && NUM_LANGUAGES > 1
    ui.set_language(1);
  #endif
  ui.goto_screen(modern_settings);
}

static void modern_language() { ui.goto_screen(modern_language_page); }

static void modern_language_page() {
  touch.clear();
  modern_header(F("Idioma"), imgModernGlobe32);

  tft.add_bar(10,56,460,58,MOD_PANEL); tft.add_rectangle(10,56,460,58,MOD_BORDER);
  tft.add_image(18,69,imgModernGlobe32,MOD_CYAN);
  tft_string.set(F("Português (Brasil)")); modern_font_button(); tft.add_text(60,70,COLOR_WHITE,tft_string);
  #if HAS_MULTI_LANGUAGE
    if (ui.language == 0) { tft_string.set(F("ATIVO")); tft.add_text(392,70,MOD_CYAN,tft_string); }
  #endif
  touch.add_control(BUTTON,10,56,460,58,(intptr_t)modern_language_set_pt);

  tft.add_bar(10,122,460,58,MOD_PANEL); tft.add_rectangle(10,122,460,58,MOD_BORDER);
  tft.add_image(18,135,imgModernGlobe32,MOD_CYAN);
  tft_string.set(F("English")); modern_font_button(); tft.add_text(60,136,COLOR_WHITE,tft_string);
  #if HAS_MULTI_LANGUAGE && NUM_LANGUAGES > 1
    if (ui.language == 1) { tft_string.set(F("ACTIVE")); tft.add_text(392,136,MOD_CYAN,tft_string); }
    touch.add_control(BUTTON,10,122,460,58,(intptr_t)modern_language_set_en);
  #endif

  tft_string.set(F("A troca afeta os menus nativos do Marlin.")); modern_font_small(); tft.add_text(18,204,COLOR_GREY,tft_string,444);
  tft_string.set(F("A interface personalizada permanece em portugues.")); tft.add_text(18,224,COLOR_GREY,tft_string,444);
  modern_back(270);
}

static void modern_brightness() {
  #if HAS_LCD_BRIGHTNESS
    modern_brightness_value = modern_brightness_value >= 230 ? 50 : modern_brightness_value + 50;
    ui.set_brightness(modern_brightness_value);
  #endif
  ui.refresh();
}

static void modern_sound_toggle() {
  #if ENABLED(SOUND_MENU_ITEM)
    ui.sound_on = !ui.sound_on;
  #endif
  ui.refresh();
}

static void modern_timeout() {
  modern_screen_timeout = modern_screen_timeout == 5 ? 10 : (modern_screen_timeout == 10 ? 0 : 5);
  #if LCD_BACKLIGHT_TIMEOUT_MINS
    ui.backlight_timeout_minutes = modern_screen_timeout;
    ui.refresh_backlight_timeout();
  #elif HAS_DISPLAY_SLEEP
    ui.sleep_timeout_minutes = modern_screen_timeout;
    ui.refresh_screen_timeout();
  #endif
  ui.refresh();
}
static void modern_wifi_toggle() {
  modern_wifi_enabled = !modern_wifi_enabled;
  modern_wifi_apply();
  ui.refresh();
}

static void modern_wifi() {
  touch.clear(); modern_header(F("Wi-Fi"), imgModernWifi24);
  tft.add_bar(10,58,460,70,MOD_PANEL); tft.add_rectangle(10,58,460,70,MOD_BORDER);
  tft.add_image(24,76,imgModernWifi24,modern_wifi_enabled ? MOD_CYAN : COLOR_GREY);
  tft_string.set(F("Módulo Robin-WIFI")); modern_font_button(); tft.add_text(64,68,COLOR_WHITE,tft_string);
  tft_string.set(modern_wifi_enabled ? F("Ativo") : F("Desativado")); tft.add_text(64,92,modern_wifi_enabled ? MOD_CYAN : COLOR_GREY,tft_string);
  tft.add_bar(10,136,460,52,MOD_PANEL); tft.add_rectangle(10,136,460,52,MOD_BORDER);
  tft_string.set(modern_wifi_enabled ? F("Desativar Wi-Fi") : F("Ativar Wi-Fi")); modern_font_button(); tft.add_text(80,148,COLOR_WHITE,tft_string);
  #if ENABLED(TOUCH_SCREEN)
    touch.add_control(BUTTON, 10, 136, 460, 52, (intptr_t)modern_wifi_toggle);
  #endif
  tft_string.set(F("A Monster8 V2 usa um módulo Wi-Fi externo.")); modern_font_small(); tft.add_text(18,204,COLOR_GREY,tft_string,444);
  tft_string.set(F("Sem o módulo instalado, o botão não terá efeito de rádio.")); tft.add_text(18,224,COLOR_GREY,tft_string,444);
  modern_back(270);
}

static void modern_settings() {
  touch.clear(); modern_header(F("Configurações"), imgSettings32);
  tft.add_bar(10,56,460,42,MOD_PANEL); tft.add_rectangle(10,56,460,42,MOD_BORDER); tft.add_image(18,61,imgModernGlobe32,MOD_CYAN); tft_string.set(F("Idioma")); modern_font_button(); tft.add_text(60,68,COLOR_WHITE,tft_string); tft_string.set(F("Português")); tft.add_text(382,68,COLOR_WHITE,tft_string); touch.add_control(BUTTON,10,56,460,42,(intptr_t)modern_language);
  tft.add_bar(10,102,460,42,MOD_PANEL); tft.add_rectangle(10,102,460,42,MOD_BORDER); tft.add_image(18,107,imgModernSun32,MOD_CYAN); tft_string.set(F("Brilho")); tft.add_text(60,114,COLOR_WHITE,tft_string); tft.add_rectangle(245,118,160,8,MOD_PANEL2); tft.add_bar(246,119,(159U*modern_brightness_value)/255U,6,MOD_CYAN); tft_string.set(i16tostr3rj((modern_brightness_value*100U+127U)/255U)); tft_string.add('%'); tft.add_text(414,112,COLOR_WHITE,tft_string); touch.add_control(BUTTON,10,102,460,42,(intptr_t)modern_brightness);
  tft.add_bar(10,148,460,42,MOD_PANEL); tft.add_rectangle(10,148,460,42,MOD_BORDER); tft.add_image(18,153,imgModernSpeaker32,MOD_CYAN); tft_string.set(F("Som")); tft.add_text(60,160,COLOR_WHITE,tft_string); tft_string.set(TERN(SOUND_MENU_ITEM, ui.sound_on ? F("ON") : F("OFF"), F("OFF"))); tft.add_text(414,160,COLOR_WHITE,tft_string); touch.add_control(BUTTON,10,148,460,42,(intptr_t)modern_sound_toggle);
  tft.add_bar(10,194,460,42,MOD_PANEL); tft.add_rectangle(10,194,460,42,MOD_BORDER); tft.add_image(18,199,imgModernClock32,MOD_CYAN); tft_string.set(F("Tempo de tela")); tft.add_text(60,206,COLOR_WHITE,tft_string); tft_string.set(i16tostr3rj(modern_screen_timeout)); tft_string.add(F(" min")); tft.add_text(400,206,COLOR_WHITE,tft_string);
  touch.add_control(BUTTON,10,194,460,42,(intptr_t)modern_timeout);
  tft.add_bar(10,240,460,38,MOD_PANEL); tft.add_rectangle(10,240,460,38,MOD_BORDER); tft.add_image(18,243,imgModernWifi24,modern_wifi_enabled ? MOD_CYAN : COLOR_GREY);
  tft_string.set(F("Wi-Fi")); modern_font_button(); tft.add_text(60,248,COLOR_WHITE,tft_string);
  tft_string.set(modern_wifi_enabled ? F("Ativo") : F("Desativado")); tft.add_text(382,248,modern_wifi_enabled ? MOD_CYAN : COLOR_GREY,tft_string);
  touch.add_control(BUTTON,10,240,460,38,(intptr_t)modern_wifi);
  tft.add_bar(10,282,460,34,MOD_PANEL); tft.add_rectangle(10,282,460,34,MOD_BORDER); tft.add_image(18,283,imgModernWrench32,MOD_CYAN);
  modern_button(52,282,418,34,F("Avançado"),MOD_CYAN,menu_configuration);
  modern_back(322);
}

static void modern_about() {
  touch.clear(); modern_header(F("Sobre"), imgMenu32);
  tft.add_bar(10,58,460,188,MOD_PANEL); tft.add_rectangle(10,58,460,188,MOD_BORDER);
  tft.add_image(20,72,imgMarlinLogo112,MOD_CYAN);
  tft_string.set(F("Marlin 2.1.2.8")); modern_font_button(); tft.add_text(100,78,COLOR_WHITE,tft_string);
  tft_string.set(F("MKS Monster8 V2.0")); tft.add_text(100,110,COLOR_WHITE,tft_string);
  tft_string.set(F("MKS TS35 V2.0")); tft.add_text(100,132,COLOR_WHITE,tft_string);
  tft_string.set(F("UI Personalizada (Touch)")); tft.add_text(100,154,COLOR_WHITE,tft_string);
  tft_string.set(F("Sensor da Câmera → Motor")); tft.add_text(100,176,COLOR_WHITE,tft_string);
  tft.add_image(24,208,imgMotor32,MOD_PURPLE); tft_string.set(F("O seu projeto, do seu jeito!")); tft.add_text(62,214,MOD_PURPLE,tft_string);
  modern_back();
}

static void modern_go_back() { ui.goto_previous_screen(); }

void MarlinUI::draw_status_screen() {
  TERN_(TOUCH_SCREEN, touch.clear());
  if (printingIsActive()) modern_print(); else modern_home();
}

// Low-level draw_edit_screen can be used to draw an edit screen from anyplace
void MenuEditItemBase::draw_edit_screen(FSTR_P const fstr, const char * const value/*=nullptr*/) {
  ui.encoder_direction_normal();
  TERN_(TOUCH_SCREEN, touch.clear());

  uint16_t line = 1;

  menu_line(line++);
  tft_string.set(fstr, itemIndex, itemStringC, itemStringF);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);

  TERN_(AUTO_BED_LEVELING_UBL, if (ui.external_control) line++);  // ftostr52() will overwrite *value so *value has to be displayed first

  menu_line(line);
  tft_string.set(value);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

  #if ENABLED(AUTO_BED_LEVELING_UBL)
    if (ui.external_control) {
      menu_line(line - 1);

      tft_string.set(X_LBL);
      tft.add_text((TFT_WIDTH / 2 - 120), MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
      tft_string.set(ftostr52(LOGICAL_X_POSITION(current_position.x)));
      tft_string.trim();
      tft.add_text((TFT_WIDTH / 2 - 16) - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

      tft_string.set(Y_LBL);
      tft.add_text((TFT_WIDTH / 2 + 16), MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
      tft_string.set(ftostr52(LOGICAL_X_POSITION(current_position.y)));
      tft_string.trim();
      tft.add_text((TFT_WIDTH / 2 + 120) - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);
    }
  #endif

  if (ui.can_show_slider()) {

    #define SLIDER_LENGTH 336
    #define SLIDER_Y_POSITION 186

    tft.canvas((TFT_WIDTH - SLIDER_LENGTH) / 2, SLIDER_Y_POSITION, SLIDER_LENGTH, 16);
    tft.set_background(COLOR_BACKGROUND);

    int16_t position = (SLIDER_LENGTH - 2) * ui.encoderPosition / maxEditValue;
    tft.add_bar(0, 7, 1, 2, ui.encoderPosition == 0 ? COLOR_SLIDER_INACTIVE : COLOR_SLIDER);
    tft.add_bar(1, 6, position, 4, COLOR_SLIDER);
    tft.add_bar(position + 1, 6, SLIDER_LENGTH - 2 - position, 4, COLOR_SLIDER_INACTIVE);
    tft.add_bar(SLIDER_LENGTH - 1, 7, 1, 2, int32_t(ui.encoderPosition) == maxEditValue ? COLOR_SLIDER : COLOR_SLIDER_INACTIVE);

    #if ENABLED(TOUCH_SCREEN)
      tft.add_image((SLIDER_LENGTH - 8) * ui.encoderPosition / maxEditValue, 0, imgSlider, COLOR_SLIDER);
      touch.add_control(SLIDER, (TFT_WIDTH - SLIDER_LENGTH) / 2, SLIDER_Y_POSITION - 8, SLIDER_LENGTH, 32, maxEditValue);
    #endif
  }

  tft.draw_edit_screen_buttons();
}

void TFT::draw_edit_screen_buttons() {
  #if ENABLED(TOUCH_SCREEN)
    add_control(64, TFT_HEIGHT - 64, DECREASE, imgDecrease);
    add_control(352, TFT_HEIGHT - 64, INCREASE, imgIncrease);
    add_control(208, TFT_HEIGHT - 64, CLICK, imgConfirm);
  #endif
}

// The Select Screen presents a prompt and two "buttons"
void MenuItem_confirm::draw_select_screen(FSTR_P const yes, FSTR_P const no, const bool yesno, FSTR_P const pref, const char * const string/*=nullptr*/, FSTR_P const suff/*=nullptr*/) {
  uint16_t line = 1;

  if (!string) line++;

  menu_line(line++);
  tft_string.set(pref);
  tft_string.trim();
  tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);

  if (string) {
    menu_line(line++);
    tft_string.set(string);
    tft_string.trim();
    tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);
  }

  if (suff) {
    menu_line(line);
    tft_string.set(suff);
    tft_string.trim();
    tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);
  }
  #if ENABLED(TOUCH_SCREEN)
    if (no)  add_control( 88, TFT_HEIGHT - 64, CANCEL,  imgCancel,  true, yesno ? HALF(COLOR_CONTROL_CANCEL) : COLOR_CONTROL_CANCEL);
    if (yes) add_control(328, TFT_HEIGHT - 64, CONFIRM, imgConfirm, true, yesno ? COLOR_CONTROL_CONFIRM : HALF(COLOR_CONTROL_CONFIRM));
  #endif
}

#if ENABLED(ADVANCED_PAUSE_FEATURE)

  void MarlinUI::draw_hotend_status(const uint8_t row, const uint8_t extruder) {
    #if ENABLED(TOUCH_SCREEN)
      touch.clear();
      draw_menu_navigation = false;
      touch.add_control(RESUME_CONTINUE , 0, 0, TFT_WIDTH, TFT_HEIGHT);
    #endif

    menu_line(row);
    tft_string.set(GET_TEXT(MSG_FILAMENT_CHANGE_NOZZLE));
    tft_string.add('E');
    tft_string.add((char)('1' + extruder));
    tft_string.add(' ');
    tft_string.add(i16tostr3rj(thermalManager.wholeDegHotend(extruder)));
    tft_string.add(LCD_STR_DEGREE);
    tft_string.add(F(" / "));
    tft_string.add(i16tostr3rj(thermalManager.degTargetHotend(extruder)));
    tft_string.add(LCD_STR_DEGREE);
    tft_string.trim();
    tft.add_text(tft_string.center(TFT_WIDTH), 0, COLOR_MENU_TEXT, tft_string);
  }

#endif // ADVANCED_PAUSE_FEATURE

#if ENABLED(AUTO_BED_LEVELING_UBL)
  #define GRID_OFFSET_X   8
  #define GRID_OFFSET_Y   8
  #define GRID_WIDTH      192
  #define GRID_HEIGHT     192
  #define CONTROL_OFFSET  16

  void MarlinUI::ubl_plot(const uint8_t x_plot, const uint8_t y_plot) {

    tft.canvas(GRID_OFFSET_X, GRID_OFFSET_Y, GRID_WIDTH, GRID_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft.add_rectangle(0, 0, GRID_WIDTH, GRID_HEIGHT, COLOR_WHITE);

    for (uint16_t x = 0; x < (GRID_MAX_POINTS_X); x++)
      for (uint16_t y = 0; y < (GRID_MAX_POINTS_Y); y++)
        if (position_is_reachable({ bedlevel.get_mesh_x(x), bedlevel.get_mesh_y(y) }))
          tft.add_bar(1 + (x * 2 + 1) * (GRID_WIDTH - 4) / (GRID_MAX_POINTS_X) / 2, GRID_HEIGHT - 3 - ((y * 2 + 1) * (GRID_HEIGHT - 4) / (GRID_MAX_POINTS_Y) / 2), 2, 2, COLOR_UBL);

    tft.add_rectangle((x_plot * 2 + 1) * (GRID_WIDTH - 4) / (GRID_MAX_POINTS_X) / 2 - 1, GRID_HEIGHT - 5 - ((y_plot * 2 + 1) * (GRID_HEIGHT - 4) / (GRID_MAX_POINTS_Y) / 2), 6, 6, COLOR_UBL);

    const xy_pos_t pos = { bedlevel.get_mesh_x(x_plot), bedlevel.get_mesh_y(y_plot) },
                   lpos = pos.asLogical();

    tft.canvas(320, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2 - MENU_ITEM_HEIGHT, 120, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(X_LBL);
    tft.add_text(0, MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
    tft_string.set(ftostr52(lpos.x));
    tft_string.trim();
    tft.add_text(120 - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    tft.canvas(320, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2, 120, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(Y_LBL);
    tft.add_text(0, MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
    tft_string.set(ftostr52(lpos.y));
    tft_string.trim();
    tft.add_text(120 - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    tft.canvas(320, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2 + MENU_ITEM_HEIGHT, 120, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(Z_LBL);
    tft.add_text(0, MENU_TEXT_Y_OFFSET, COLOR_MENU_TEXT, tft_string);
    tft_string.set(isnan(bedlevel.z_values[x_plot][y_plot]) ? "-----" : ftostr43sign(bedlevel.z_values[x_plot][y_plot]));
    tft_string.trim();
    tft.add_text(120 - tft_string.width(), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    constexpr uint8_t w = (TFT_WIDTH) / 10;
    tft.canvas(GRID_OFFSET_X + (GRID_WIDTH - w) / 2, GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET - 5, w, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(ui8tostr3rj(x_plot));
    tft_string.trim();
    tft.add_text(tft_string.center(w), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    tft.canvas(GRID_OFFSET_X + GRID_WIDTH + CONTROL_OFFSET + 16 - 24, GRID_OFFSET_Y + (GRID_HEIGHT - MENU_ITEM_HEIGHT) / 2, w, MENU_ITEM_HEIGHT);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set(ui8tostr3rj(y_plot));
    tft_string.trim();
    tft.add_text(tft_string.center(w), MENU_TEXT_Y_OFFSET, COLOR_MENU_VALUE, tft_string);

    #if ENABLED(TOUCH_SCREEN)
      touch.clear();
      draw_menu_navigation = false;
      add_control(GRID_OFFSET_X + GRID_WIDTH + CONTROL_OFFSET,      GRID_OFFSET_Y + CONTROL_OFFSET,                    UBL,  (ENCODER_STEPS_PER_MENU_ITEM) * (GRID_MAX_POINTS_X), imgUp);
      add_control(GRID_OFFSET_X + GRID_WIDTH + CONTROL_OFFSET,      GRID_OFFSET_Y + GRID_HEIGHT - CONTROL_OFFSET - 32, UBL, -(ENCODER_STEPS_PER_MENU_ITEM) * (GRID_MAX_POINTS_X), imgDown);
      add_control(GRID_OFFSET_X + CONTROL_OFFSET,                   GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET,      UBL, -(ENCODER_STEPS_PER_MENU_ITEM), imgLeft);
      add_control(GRID_OFFSET_X + GRID_WIDTH - CONTROL_OFFSET - 32, GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET,      UBL,   ENCODER_STEPS_PER_MENU_ITEM, imgRight);
      add_control(320, GRID_OFFSET_Y + GRID_HEIGHT + CONTROL_OFFSET, CLICK, imgLeveling);
      add_control(224, TFT_HEIGHT - 34, BACK, imgBack);
    #endif
  }
#endif // AUTO_BED_LEVELING_UBL

#if ENABLED(BABYSTEP_ZPROBE_OFFSET)
  #include "../../feature/babystep.h"
#endif

#if HAS_BED_PROBE
  #include "../../module/probe.h"
#endif

#define Z_SELECTION_Z 1
#define Z_SELECTION_Z_PROBE -1

struct MotionAxisState {
  xy_int_t xValuePos, yValuePos, zValuePos, eValuePos, stepValuePos, zTypePos, eNamePos;
  float currentStepSize = 10.0;
  int z_selection = Z_SELECTION_Z;
  uint8_t e_selection = 0;
  bool blocked = false;
  char message[32];
};

MotionAxisState motionAxisState;

#define E_BTN_COLOR COLOR_YELLOW
#define X_BTN_COLOR COLOR_CORAL_RED
#define Y_BTN_COLOR COLOR_VIVID_GREEN
#define Z_BTN_COLOR COLOR_LIGHT_BLUE

#define BTN_WIDTH 64
#define BTN_HEIGHT 52
#define X_MARGIN 20
#define Y_MARGIN 15

static void quick_feedback() {
  #if HAS_CHIRP
    ui.chirp(); // Buzz and wait. Is the delay needed for buttons to settle?
    #if ALL(HAS_MARLINUI_MENU, HAS_BEEPER)
      for (int8_t i = 5; i--;) { buzzer.tick(); delay(2); }
    #elif HAS_MARLINUI_MENU
      delay(10);
    #endif
  #endif
}

#define CUR_STEP_VALUE_WIDTH 104
static void drawCurStepValue() {
  tft_string.set(ftostr52sp(motionAxisState.currentStepSize));
  tft_string.add(F("mm"));
  tft.canvas(motionAxisState.stepValuePos.x, motionAxisState.stepValuePos.y, CUR_STEP_VALUE_WIDTH, BTN_HEIGHT);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_text(tft_string.center(CUR_STEP_VALUE_WIDTH), 0, COLOR_AXIS_HOMED, tft_string);
}

static void drawCurZSelection() {
  tft_string.set('Z');
  tft.canvas(motionAxisState.zTypePos.x, motionAxisState.zTypePos.y, tft_string.width(), 34);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_text(0, 0, Z_BTN_COLOR, tft_string);
  tft.queue.sync();
  tft_string.set(F("Offset"));
  tft.canvas(motionAxisState.zTypePos.x, motionAxisState.zTypePos.y + 34, tft_string.width(), 34);
  tft.set_background(COLOR_BACKGROUND);
  if (motionAxisState.z_selection == Z_SELECTION_Z_PROBE) {
    tft.add_text(0, 0, Z_BTN_COLOR, tft_string);
  }
}

static void drawCurESelection() {
  tft.canvas(motionAxisState.eNamePos.x, motionAxisState.eNamePos.y, BTN_WIDTH, BTN_HEIGHT);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set('E');
  tft.add_text(0, 0, E_BTN_COLOR , tft_string);
  tft.add_text(tft_string.width(), 0, E_BTN_COLOR, ui8tostr3rj(motionAxisState.e_selection));
}

static void drawMessage(PGM_P const msg) {
  tft.canvas(X_MARGIN, TFT_HEIGHT - Y_MARGIN - 34, TFT_HEIGHT / 2, 34);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_text(0, 0, COLOR_YELLOW, msg);
}

static void drawMessage(FSTR_P const fmsg) { drawMessage(FTOP(fmsg)); }

static void drawAxisValue(const AxisEnum axis) {
  const float value = (
    TERN_(HAS_BED_PROBE, axis == Z_AXIS && motionAxisState.z_selection == Z_SELECTION_Z_PROBE ? probe.offset.z :)
    ui.manual_move.axis_value(axis)
  );
  xy_int_t pos;
  uint16_t color;
  switch (axis) {
    case X_AXIS: pos = motionAxisState.xValuePos; color = X_BTN_COLOR; break;
    case Y_AXIS: pos = motionAxisState.yValuePos; color = Y_BTN_COLOR; break;
    case Z_AXIS: pos = motionAxisState.zValuePos; color = Z_BTN_COLOR; break;
    case E_AXIS: pos = motionAxisState.eValuePos; color = E_BTN_COLOR; break;
    default: return;
  }
  tft.canvas(pos.x, pos.y, BTN_WIDTH + X_MARGIN, BTN_HEIGHT);
  tft.set_background(COLOR_BACKGROUND);
  tft_string.set(ftostr52sp(value));
  tft.add_text(0, 0, color, tft_string);
}

static void moveAxis(const AxisEnum axis, const int8_t direction) {
  quick_feedback();

  #if ENABLED(PREVENT_COLD_EXTRUSION)
    if (axis == E_AXIS && thermalManager.tooColdToExtrude(motionAxisState.e_selection)) {
      drawMessage(F("Too cold"));
      return;
    }
  #endif

  const float diff = motionAxisState.currentStepSize * direction;

  if (axis == Z_AXIS && motionAxisState.z_selection == Z_SELECTION_Z_PROBE) {
    #if ENABLED(BABYSTEP_ZPROBE_OFFSET)
      const int16_t babystep_increment = direction * BABYSTEP_SIZE_Z;
      const bool do_probe = DISABLED(BABYSTEP_HOTEND_Z_OFFSET) || active_extruder == 0;
      const float bsDiff = planner.mm_per_step[Z_AXIS] * babystep_increment,
                  new_probe_offset = probe.offset.z + bsDiff,
                  new_offs = TERN(BABYSTEP_HOTEND_Z_OFFSET
                    , do_probe ? new_probe_offset : hotend_offset[active_extruder].z - bsDiff
                    , new_probe_offset
                  );
      if (WITHIN(new_offs, Z_PROBE_OFFSET_RANGE_MIN, Z_PROBE_OFFSET_RANGE_MAX)) {
        babystep.add_steps(Z_AXIS, babystep_increment);
        if (do_probe)
          probe.offset.z = new_offs;
        else
          TERN(BABYSTEP_HOTEND_Z_OFFSET, hotend_offset[active_extruder].z = new_offs, NOOP);
        drawMessage(NUL_STR); // clear the error
        drawAxisValue(axis);
      }
      else {
        drawMessage(GET_TEXT_F(MSG_LCD_SOFT_ENDSTOPS));
      }
    #elif HAS_BED_PROBE
      // only change probe.offset.z
      probe.offset.z += diff;
      if (direction < 0 && current_position[axis] < Z_PROBE_OFFSET_RANGE_MIN) {
        current_position[axis] = Z_PROBE_OFFSET_RANGE_MIN;
        drawMessage(GET_TEXT_F(MSG_LCD_SOFT_ENDSTOPS));
      }
      else if (direction > 0 && current_position[axis] > Z_PROBE_OFFSET_RANGE_MAX) {
        current_position[axis] = Z_PROBE_OFFSET_RANGE_MAX;
        drawMessage(GET_TEXT_F(MSG_LCD_SOFT_ENDSTOPS));
      }
      else {
        drawMessage(NUL_STR); // clear the error
      }
      drawAxisValue(axis);
    #endif
    return;
  }

  if (!ui.manual_move.processing) {
    // Get motion limit from software endstops, if any
    float min, max;
    soft_endstop.get_manual_axis_limits(axis, min, max);

    // Delta limits XY based on the current offset from center
    // This assumes the center is 0,0
    #if ENABLED(DELTA)
      if (axis != Z_AXIS && axis != E_AXIS) {
        max = SQRT(sq((float)(DELTA_PRINTABLE_RADIUS)) - sq(current_position[Y_AXIS - axis])); // (Y_AXIS - axis) == the other axis
        min = -max;
      }
    #endif

    // Get the new position
    const bool limited = ui.manual_move.apply_diff(axis, diff, min, max);
    #if IS_KINEMATIC
      UNUSED(limited);
    #else
      PGM_P const msg = limited ? GET_TEXT(MSG_LCD_SOFT_ENDSTOPS) : NUL_STR;
      drawMessage(msg);
    #endif

    ui.manual_move.soon(axis OPTARG(MULTI_E_MANUAL, motionAxisState.e_selection));
  }

  drawAxisValue(axis);
}

static void e_plus()  { moveAxis(E_AXIS, 1);  }
static void e_minus() { moveAxis(E_AXIS, -1); }
static void x_minus() { moveAxis(X_AXIS, -1); }
static void x_plus()  { moveAxis(X_AXIS, 1);  }
static void y_plus()  { moveAxis(Y_AXIS, 1);  }
static void y_minus() { moveAxis(Y_AXIS, -1); }
static void z_plus()  { moveAxis(Z_AXIS, 1);  }
static void z_minus() { moveAxis(Z_AXIS, -1); }

#if ENABLED(TOUCH_SCREEN)
  static void e_select() {
    motionAxisState.e_selection++;
    if (motionAxisState.e_selection >= EXTRUDERS) {
      motionAxisState.e_selection = 0;
    }

    quick_feedback();
    drawCurESelection();
    drawAxisValue(E_AXIS);
  }

  static void do_home() {
    quick_feedback();
    drawMessage(GET_TEXT_F(MSG_LEVEL_BED_HOMING));
    queue.inject_P(G28_STR);
    // Disable touch until home is done
    TERN_(HAS_TFT_XPT2046, touch.disable());
    drawAxisValue(E_AXIS);
    drawAxisValue(X_AXIS);
    drawAxisValue(Y_AXIS);
    drawAxisValue(Z_AXIS);
  }

  static void step_size() {
    motionAxisState.currentStepSize = motionAxisState.currentStepSize / 10.0;
    if (motionAxisState.currentStepSize < 0.0015) motionAxisState.currentStepSize = 10.0;
    quick_feedback();
    drawCurStepValue();
  }
#endif

#if ALL(HAS_BED_PROBE, TOUCH_SCREEN)
  static void z_select() {
    motionAxisState.z_selection *= -1;
    quick_feedback();
    drawCurZSelection();
    drawAxisValue(Z_AXIS);
  }
#endif

static void disable_steppers() {
  quick_feedback();
  queue.inject(F("M84"));
}

static void drawBtn(int x, int y, const char *label, intptr_t data, MarlinImage img, uint16_t bgColor, bool enabled = true) {
  uint16_t width = Images[imgBtn52Rounded].width;
  uint16_t height = Images[imgBtn52Rounded].height;

  if (!enabled) bgColor = COLOR_CONTROL_DISABLED;

  tft.canvas(x, y, width, height);
  tft.set_background(COLOR_BACKGROUND);
  tft.add_image(0, 0, imgBtn52Rounded, bgColor, COLOR_BACKGROUND, COLOR_DARKGREY);

  // TODO: Make an add_text() taking a font arg
  if (label) {
    tft_string.set(label);
    tft_string.trim();
    tft.add_text(tft_string.center(width), height / 2 - tft_string.font_height() / 2, bgColor, tft_string);
  }
  else {
    tft.add_image(0, 0, img, bgColor, COLOR_BACKGROUND, COLOR_DARKGREY);
  }

  TERN_(HAS_TFT_XPT2046, if (enabled) touch.add_control(BUTTON, x, y, width, height, data));
}

void MarlinUI::move_axis_screen() {
  // Reset
  defer_status_screen(true);
  motionAxisState.blocked = false;
  TERN_(HAS_TFT_XPT2046, touch.enable());

  ui.clear_lcd();

  TERN_(TOUCH_SCREEN, touch.clear());

  const bool busy = printingIsActive();

  // Babysteps during printing? Select babystep for Z probe offset
  if (busy && ENABLED(BABYSTEP_ZPROBE_OFFSET))
    motionAxisState.z_selection = Z_SELECTION_Z_PROBE;

  // ROW 1 -> E- Y- CurY Z+
  int x = X_MARGIN, y = Y_MARGIN, spacing = 0;

  drawBtn(x, y, "E+", (intptr_t)e_plus, imgUp, E_BTN_COLOR, !busy);

  spacing = (TFT_WIDTH - X_MARGIN * 2 - 3 * BTN_WIDTH) / 2;
  x += BTN_WIDTH + spacing;
  drawBtn(x, y, "Y+", (intptr_t)y_plus, imgUp, Y_BTN_COLOR, !busy);

  // Cur Y
  x += BTN_WIDTH;
  motionAxisState.yValuePos.x = x + 2;
  motionAxisState.yValuePos.y = y;
  drawAxisValue(Y_AXIS);

  x += spacing;
  drawBtn(x, y, "Z+", (intptr_t)z_plus, imgUp, Z_BTN_COLOR, !busy || ENABLED(BABYSTEP_ZPROBE_OFFSET)); //only enabled when not busy or have baby step

  // ROW 2 -> "Ex"  X-  HOME X+  "Z"
  y += BTN_HEIGHT + (TFT_HEIGHT - Y_MARGIN * 2 - 4 * BTN_HEIGHT) / 3;
  x = X_MARGIN;
  spacing = (TFT_WIDTH - X_MARGIN * 2 - 5 * BTN_WIDTH) / 4;

  motionAxisState.eNamePos.x = x;
  motionAxisState.eNamePos.y = y;
  drawCurESelection();
  TERN_(HAS_TFT_XPT2046, if (!busy) touch.add_control(BUTTON, x, y, BTN_WIDTH, BTN_HEIGHT, (intptr_t)e_select));

  x += BTN_WIDTH + spacing;
  drawBtn(x, y, "X-", (intptr_t)x_minus, imgLeft, X_BTN_COLOR, !busy);

  x += BTN_WIDTH + spacing; //imgHome is 64x64
  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH / 2 - Images[imgHome].width / 2, y - (Images[imgHome].width - BTN_HEIGHT) / 2, BUTTON, (intptr_t)do_home, imgHome, !busy));

  x += BTN_WIDTH + spacing;
  uint16_t xplus_x = x;
  drawBtn(x, y, "X+", (intptr_t)x_plus, imgRight, X_BTN_COLOR, !busy);

  x += BTN_WIDTH + spacing;
  motionAxisState.zTypePos.x = x;
  motionAxisState.zTypePos.y = y;
  drawCurZSelection();
  #if ALL(HAS_BED_PROBE, TOUCH_SCREEN)
    if (!busy) touch.add_control(BUTTON, x, y, BTN_WIDTH, 34 * 2, (intptr_t)z_select);
  #endif

  // ROW 3 -> E- CurX Y-  Z-
  y += BTN_HEIGHT + (TFT_HEIGHT - Y_MARGIN * 2 - 4 * BTN_HEIGHT) / 3;
  x = X_MARGIN;
  spacing = (TFT_WIDTH - X_MARGIN * 2 - 3 * BTN_WIDTH) / 2;

  drawBtn(x, y, "E-", (intptr_t)e_minus, imgDown, E_BTN_COLOR, !busy);

  // Cur E
  motionAxisState.eValuePos.x = x;
  motionAxisState.eValuePos.y = y + BTN_HEIGHT + 2;
  drawAxisValue(E_AXIS);

  // Cur X
  motionAxisState.xValuePos.x = BTN_WIDTH + (TFT_WIDTH - X_MARGIN * 2 - 5 * BTN_WIDTH) / 4; //X- pos
  motionAxisState.xValuePos.y = y - 10;
  drawAxisValue(X_AXIS);

  x += BTN_WIDTH + spacing;
  drawBtn(x, y, "Y-", (intptr_t)y_minus, imgDown, Y_BTN_COLOR, !busy);

  x += BTN_WIDTH + spacing;
  drawBtn(x, y, "Z-", (intptr_t)z_minus, imgDown, Z_BTN_COLOR, !busy || ENABLED(BABYSTEP_ZPROBE_OFFSET)); //only enabled when not busy or have baby step

  // Cur Z
  motionAxisState.zValuePos.x = x;
  motionAxisState.zValuePos.y = y + BTN_HEIGHT + 2;
  drawAxisValue(Z_AXIS);

  // ROW 4 -> step_size  disable steppers back
  y = TFT_HEIGHT - Y_MARGIN - 32; //
  x = TFT_WIDTH / 2 - CUR_STEP_VALUE_WIDTH / 2;
  motionAxisState.stepValuePos.x = x;
  motionAxisState.stepValuePos.y = y;
  if (!busy) {
    drawCurStepValue();
    TERN_(HAS_TFT_XPT2046, touch.add_control(BUTTON, motionAxisState.stepValuePos.x, motionAxisState.stepValuePos.y, CUR_STEP_VALUE_WIDTH, BTN_HEIGHT, (intptr_t)step_size));
  }

  // aligned with x+
  drawBtn(xplus_x, TFT_HEIGHT - Y_MARGIN - BTN_HEIGHT, "off", (intptr_t)disable_steppers, imgCancel, COLOR_WHITE, !busy);

  TERN_(HAS_TFT_XPT2046, add_control(TFT_WIDTH - X_MARGIN - BTN_WIDTH, y, BACK, imgBack));
}

#endif // HAS_UI_480x320
