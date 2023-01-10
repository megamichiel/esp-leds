
#include "led_server.h"
#include "util.h"


uint8_t LedServer::handlePacket(const uint8_t *&packet, unsigned long current_ms) {
  uint8_t flags = *(packet++);

  if (!(flags & 0b111111)) { // Nothing has changed
    return flags;
  }

  debugf("Updating leds, flags: %x\n", flags);

  if ((flags & 1)) {
    controller->setDeviceName(readString(packet));
  }
  if ((flags & 2)) {
    controller->setBrightness(*(packet++));
  }
  if (flags & 4) {
    int fade = *(packet++) << 8;
    fade |= *(packet++);
    if (fade == 0)
      fade = 1;

    controller->setFade(fade);
  }

  for (int i = 0; i < 3; ++i) {
    if (flags & (8 << i)) {
      FormulaType type = *(packet++) ? double_formula : int_formula;
      controller->setFormula(i, type, readString(packet).c_str());
    }
  }

  if (flags != 0) {
    controller->mark_change(current_ms);
  }

  return flags;
}

void LedServer::writePacket(uint8_t *packet, unsigned int &index, bool withName) {
  int fade = controller->getFade();

  if (withName) {
    writeString(packet, index, controller->getDeviceName());
  }
  packet[index++] = NUM_LEDS >> 8;
  packet[index++] = NUM_LEDS & 0xFF;
  packet[index++] = controller->getBrightness();
  packet[index++] = fade >> 8;
  packet[index++] = fade & 0xFF;
  for (int form_index = 0; form_index < 3; ++form_index) {
    packet[index++] = (uint8_t) controller->getFormulaType(form_index);
    writeString(packet, index, controller->getFormula(form_index));
  }
}
