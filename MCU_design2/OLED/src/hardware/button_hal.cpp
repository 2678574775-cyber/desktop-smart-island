/**
 * button_hal.cpp — 4 键直连 GPIO (A=GPIO2 B=GPIO4 C=GPIO3 D=GPIO1)
 * D 支持双击, A 支持长按(1s)
 */

#include "button_hal.h"

static const uint8_t keyPins[BTN_COUNT] = { KEY_PIN_A, KEY_PIN_B, KEY_PIN_C, KEY_PIN_D };

void initButtons() {
  for (int i = 0; i < BTN_COUNT; i++) {
    int mode = (keyPins[i] == 2) ? INPUT : INPUT_PULLUP;  // GPIO2 strapping
    pinMode(keyPins[i], mode);
    buttons[i] = {HIGH, HIGH, false, false, false, false, false, 0, 0, 0};
    btnRaw[i] = HIGH; debounceCnt[i] = 0; debounceState[i] = HIGH;
  }
}

void readButtons() {
  unsigned long now = millis();

  for (int i = 0; i < BTN_COUNT; i++) {
    Button& b = buttons[i];
    b.pressed = b.released = b.longPress = b.doubleTap = false;

    btnRaw[i] = digitalRead(keyPins[i]);
    uint8_t raw = btnRaw[i];
    if (raw == debounceState[i]) debounceCnt[i] = 0;
    else { debounceCnt[i]++; if (debounceCnt[i] >= DEBOUNCE_CNT) { debounceState[i] = raw; debounceCnt[i] = 0; } }

    bool sp = (debounceState[i] == LOW);
    b.raw = sp ? LOW : HIGH;

    if (sp && b.last == HIGH) {
      b.pressed = true; b.pressTime = now; b.last = LOW;
      // D 键双击检测 (idx=3)
      if (i == 3 && b.lastClickTime > 0 && (now - b.lastClickTime < DOUBLE_TAP_MS)) {
        b.doubleTap = true; b.lastClickTime = 0;
      }
    }
    else if (!sp && b.last == LOW) {
      b.released = true; b.releaseTime = now; b.last = HIGH;
      if (i == 3 && !b.longPressActive) b.lastClickTime = now;
      b.longPressActive = false;
    }

    // 长按: A 键 1s (录音)
    if (b.last == LOW && !b.longPressActive && i == 0) {
      if (now - b.pressTime >= LONG_PRESS_MS_A) {
        b.longPress = true; b.longPressActive = true;
      }
    }
    // D 键长按 2s → 设置 (在 dispatch 里处理, 这里不自动触发 longPress)
    if (b.last == LOW && !b.longPressActive && i == 3) {
      if (now - b.pressTime >= 2000) {
        b.longPress = true; b.longPressActive = true;
      }
    }
  }
}

#if BTN_DEBUG
static unsigned long lastDbg = 0;
void debugPrintButtons() {
  if (millis() - lastDbg < 1000) return; lastDbg = millis();
  Serial.print("BTN:"); for (int i = 0; i < BTN_COUNT; i++) Serial.print(btnRaw[i] == LOW ? '1' : '0');
  Serial.print(" fire:");
  for (int i = 0; i < BTN_COUNT; i++) if (buttons[i].pressed) { Serial.print(i); Serial.print(','); }
  Serial.println();
}
#endif
