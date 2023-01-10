
#include "EEPROM.h"

#include "util.h"

#include "led_controller.h"


void LedController::loadConfig() {
  debugln("Loading config");

  char header[10];
  EEPROM.readBytes(0, header, 10);
  if (memcmp(header, "CentralLED", 10) == 0) {
    int addr = 10;

    device_name = EEPROM.readString(addr);
    addr += (int) device_name.length() + 1;

    bright = EEPROM.read(addr++);
    fade = EEPROM.read(addr++) << 8;
    fade |= EEPROM.read(addr++);

    for (int form_index = 0; form_index < 3; ++form_index) {
      auto type = (FormulaType) EEPROM.read(addr++);
      String s = EEPROM.readString(addr);
      addr += (int) s.length() + 1;
      setFormula(form_index, type, s.c_str());
    }
  } else {
    device_name = "Light";
    bright = 4;
    fade = 1;
    setFormula(0, int_formula, "x+t");
    setFormula(1, int_formula, "255");
    setFormula(2, int_formula, "255");

    saveConfig();
  }
  debugf("Name: %s\n", device_name.c_str());
  debugf("Bright: %i\n", bright);

  debugln(formulas[0].toString());
  debugln(formulas[1].toString());
  debugln(formulas[2].toString());
}

void LedController::saveConfig() {
  debugln("Saving config");

  EEPROM.writeBytes(0, "CentralLED", 10);
  int addr = 10;
  EEPROM.writeString(addr, device_name);
  addr += (int) device_name.length() + 1;
  EEPROM.write(addr++, bright);
  EEPROM.write(addr++, fade >> 8);
  EEPROM.write(addr++, fade & 0xFF);
  for (FormulaData &data : formulas) {
    String s = data.form->toString(data.type);
    EEPROM.write(addr++, data.type);
    EEPROM.writeString(addr, s);
    addr += (int) s.length() + 1;
  }

  EEPROM.commit();
}


void LedController::init() {
  tick = 0;

  for (auto &led : leds)
    led = 0;

  FastLED.addLeds<LED_TYPE, STRIP_1_PIN, COLOR_ORDER>(leds, led_count[0]).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, STRIP_2_PIN, COLOR_ORDER>(leds, led_count[0], led_count[1]).setCorrection(TypicalLEDStrip);

  delay(50);

  update();

  printf("Led is %i %i %i\n", leds[0].r, leds[0].g, leds[0].b);
  printf("Brightness is %i\n", bright);

  FastLED.setBrightness(bright);
  FastLED.show();
}

void LedController::update() {
  CRGB c{}, p = CRGB(0, 0, 0);

  static uint8_t h, s, v;
  if (!variableFormulas) {
    h = formulas[0].eval(0, tick) & 0xFF; // hue % 256
    s = clampByte(formulas[1].eval(0, tick));
    v = clampByte(formulas[2].eval(0, tick));
  }

//  debugf("hsv = %i %i %i\n", h, s, v);

  for (int calc_led = 0, fade_offset = 0, led_index, strip, strip_start;
      calc_led - fade + 1 < NUM_LEDS; calc_led += fade, fade_offset = fade - 1, p = c) {
    if (variableFormulas) {
      h = formulas[0].eval(calc_led, tick) & 0xFF; // hue % 256
      s = clampByte(formulas[1].eval(calc_led, tick));
      v = clampByte(formulas[2].eval(calc_led, tick));
    }

    c = CHSV(h, s, v);

    while (fade_offset >= 0 && (led_index = calc_led - fade_offset) < NUM_LEDS) {
      // Do some re-calculation for multiple strips
      strip_start = 0;
      strip = 0;
      // Keep adding strip counts until we're at the current strip
      while (strip_start + led_count[strip] <= led_index)
        strip_start += led_count[strip++];

      // Reverse odd strips
      if ((strip & 1) == 0)
        led_index = strip_start + (led_count[strip] - 1 - (led_index - strip_start));

      leds[led_index] = CRGB(
              (fade_offset * p.r + (fade - fade_offset) * c.r) / fade,
              (fade_offset * p.g + (fade - fade_offset) * c.g) / fade,
              (fade_offset * p.b + (fade - fade_offset) * c.b) / fade
      );
//      debugf("%i is %i %i %i\n", z, leds[z].r, leds[z].g, leds[z].b);
      --fade_offset;
    }
  }

  tick = tick + 1;
}

void LedController::mark_change(unsigned long current_ms) {
  changed = true;
  last_changed = current_ms;
  update();
}

bool LedController::update_timed(unsigned long current_ms) {
  if (bright > 0) {
    if (timedFormulas) {
      update();
    }
  }

  // 5 seconds after something was last changed, save, instead of saving on every change
  if (changed && current_ms - last_changed > POST_CHANGE_SAVE_DELAY) {
    changed = false;
    saveConfig();
  }

  return bright > 0;
}

String LedController::getDeviceName() const {
  return device_name;
}

void LedController::setDeviceName(const String &name) {
  device_name = name;
}

int LedController::getBrightness() const {
  return bright;
}

int LedController::getFade() const {
  return fade;
}

FormulaType LedController::getFormulaType(int index) {
  return formulas[index].type;
}

String LedController::getFormula(int index) {
  return formulas[index].toString();
}

void LedController::setBrightness(int value) {
  bright = value;

  FastLED.setBrightness(value);

  if (value == 0)
    FastLED.show();
}

void LedController::setFade(int value) {
  fade = value;
}

void LedController::setFormula(int formula_index, FormulaType type, const char *str) {
  Form *form = parseFormula(str);
  if (form != nullptr) {
    FormulaData &data = formulas[formula_index];

    delete data.form;

    data.type = type;
    data.form = form;
    data.isVariable = form->isVariable();
    data.isTimed = form->isTimed();
  }

  timedFormulas = variableFormulas = false;
  for (const FormulaData &formula : formulas) {
    timedFormulas |= formula.isTimed;
    variableFormulas |= formula.isVariable;
  }
}

LedController::LedController() : leds(), bright(), fade(), tick() {}
