
#ifndef LEDS_LED_CONTROLLER_H
#define LEDS_LED_CONTROLLER_H

#include <FastLED.h>

#include "formula.h"
#include "includes.h"

struct FormulaData {
  FormulaType type = int_formula;
  Form *form = nullptr;
  bool isVariable = false, isTimed = false;

  String toString() const {
    return form->toString(type);
  }

  int eval(int x, int t) const {
    return type == int_formula ? form->eval(x, t) : (int) form->eval((double) x, (double) t);
  }
};

class LedController {
private:

  FormulaData formulas[3];
  bool timedFormulas = false, variableFormulas = false;

  CRGB leds[NUM_LEDS];

  String device_name;
  uint8_t bright;
  uint16_t fade;
  int tick;

  bool changed = false;
  unsigned long last_changed = 0;

public:
  LedController();

  void loadConfig();
  void saveConfig();

  void init();
  void update();
  void mark_change(unsigned long current_ms);
  bool update_timed(unsigned long current_ms);

  String getDeviceName() const;
  void setDeviceName(const String &name);

  int getBrightness() const;
  int getFade() const;

  FormulaType getFormulaType(int index);
  String getFormula(int index);

  void setBrightness(int value);
  void setFade(int value);

  void setFormula(int formula_index, FormulaType type, const char *str);
};


#endif //LEDS_LED_CONTROLLER_H
