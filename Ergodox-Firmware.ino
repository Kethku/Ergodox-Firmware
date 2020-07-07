/* *INDENT-ON* */
#include "Kaleidoscope-MouseKeys.h"
#include "Kaleidoscope-HostPowerManagement.h"
#include "Kaleidoscope-USB-Quirks.h"
#include "Kaleidoscope-TapDance.h"
#include "Kaleidoscope-Leader.h"
#include "Kaleidoscope-Macros.h"
#include "MultiReport/Keyboard.h"
#include "Kaleidoscope.h"

enum { LEFT_BRACKET, RIGHT_BRACKET };
enum { QWERTY, FUNCTION };
enum { MACRO_PROG };

/* *INDENT-OFF* */
KEYMAPS(
  [QWERTY] = KEYMAP_STACKED
  (
      // left hand
      M(MACRO_PROG),          Key_1, Key_2, Key_3, Key_4, Key_5, Key_Equals,
      Key_Tab,                Key_Q, Key_W, Key_E, Key_R, Key_T, TD(LEFT_BRACKET),
      Key_Escape,             Key_A, Key_S, Key_D, Key_F, Key_G,
      Key_Backtick,           Key_Z, Key_X, Key_C, Key_V, Key_B, TD(LEFT_BRACKET),
      ShiftToLayer(FUNCTION), XXX,   XXX,   XXX,   XXX,

      Key_LeftAlt,            XXX,
      ShiftToLayer(FUNCTION),
      Key_Backspace,          Key_LeftShift, Key_LeftControl,

      // right hand
      Key_Minus,         Key_6, Key_7, Key_8,     Key_9,         Key_0,         XXX,
      TD(RIGHT_BRACKET), Key_Y, Key_U, Key_I,     Key_O,         Key_P,         Key_Backslash,
                         Key_H, Key_J, Key_K,     Key_L,         Key_Semicolon, Key_Quote,
      TD(RIGHT_BRACKET), Key_N, Key_M, Key_Comma, Key_Period,    Key_Slash,     XXX,
                                XXX,   XXX,       XXX,           XXX,           ShiftToLayer(FUNCTION),

      XXX, Key_RightGui,
      ShiftToLayer(FUNCTION),
      Key_RightAlt, Key_Enter, Key_Spacebar
  ),  
  [FUNCTION] = KEYMAP_STACKED
  (
      // left hand
      XXX, Key_F1, Key_F2,     Key_F3,      Key_F4,             Key_F5,        XXX,
      XXX, XXX,    XXX,        Key_mouseUp, Key_mouseScrollUp,  Key_mouseBtnR, XXX,
      XXX, XXX,    Key_mouseL, Key_mouseDn, Key_mouseR,         Key_mouseBtnL,
      XXX, XXX,    XXX,        XXX,         Key_mouseScrollDn,  Key_mouseBtnM, XXX,
      ShiftToLayer(FUNCTION), XXX,    XXX,        XXX,         XXX,

      XXX, XXX,
      ShiftToLayer(FUNCTION),
      Key_Delete, Key_LeftControl, Key_LeftShift,

      // right hand
      XXX, Key_F6,               Key_F7,               Key_F8,             Key_F9,                Key_F10, Key_F11,
      XXX, LCTRL(Key_LeftArrow), LCTRL(Key_DownArrow), LCTRL(Key_UpArrow), LCTRL(Key_RightArrow), XXX,     Key_F12,
           Key_LeftArrow,        Key_DownArrow,        Key_UpArrow,        Key_RightArrow,        XXX,     XXX,
      XXX, Key_Home,             Key_PageDown,         Key_PageUp,         Key_End,               XXX,     XXX,
                                 XXX,                  XXX,                XXX,                   XXX,     ShiftToLayer(FUNCTION),

      XXX, XXX,
      ShiftToLayer(FUNCTION),
      Key_Enter, Key_Tab, Key_RightGui
  ),
)
// *INDENT-ON*


void tapDanceAction(uint8_t tap_dance_index, byte row, byte col, uint8_t tap_count, kaleidoscope::plugin::TapDance::ActionType tap_dance_action) {
  switch (tap_dance_index) {
    case LEFT_BRACKET:
      return tapDanceActionKeys(tap_count, tap_dance_action, Key_LeftParen, Key_LeftCurlyBracket, Key_LeftBracket);
    case RIGHT_BRACKET:
      return tapDanceActionKeys(tap_count, tap_dance_action, Key_RightParen, Key_RightCurlyBracket, Key_RightBracket);
  }
}

bool skip = false;
void typeKey(Key key, uint8_t modifiers, bool tap) {
  HID_KeyboardReport_Data_t hid_report;
  memcpy(hid_report.allkeys, Keyboard.lastKeyReport.allkeys, sizeof(hid_report));
  Keyboard.keyReport.modifiers = modifiers;
  skip = true;
  handleKeyswitchEvent(key, UNKNOWN_KEYSWITCH_LOCATION, IS_PRESSED | INJECTED);
  kaleidoscope::hid::sendKeyboardReport();
  if (tap) {
    handleKeyswitchEvent(key, UNKNOWN_KEYSWITCH_LOCATION, WAS_PRESSED | INJECTED);
    kaleidoscope::hid::sendKeyboardReport();
  }
  memcpy(Keyboard.keyReport.allkeys, hid_report.allkeys, sizeof(Keyboard.keyReport));
  delay(10);
}

namespace kaleidoscope {
  class FDEscape : public kaleidoscope::Plugin {
    public: 
      FDEscape(void) {}
      
      EventHandlerResult onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state);
      EventHandlerResult afterEachCycle();

    private:
      static uint8_t stored_modifiers;
      static Key f_stored;
      static bool f_handled;
      static bool d_handled;
      static uint32_t start_time;
      static uint16_t time_out;
  };
  
  uint8_t FDEscape::stored_modifiers;
  Key FDEscape::f_stored = Key_NoKey;
  bool FDEscape::f_handled = false;
  bool FDEscape::d_handled = false;
  uint32_t FDEscape::start_time;
  uint16_t FDEscape::time_out = 200;
  
  EventHandlerResult FDEscape::onKeyswitchEvent(Key &mapped_key, KeyAddr key_addr, uint8_t key_state) {
    if (skip) {
      skip = false;
      return EventHandlerResult::OK;
    }

    if (!Layer.isActive(FUNCTION)) {
      if (keyToggledOn(key_state)) {
        if (Layer.lookup(key_addr) == Key_F) {
          start_time = Kaleidoscope.millisAtCycleStart();
          f_handled = true;
          if (f_stored != Key_NoKey) {
            f_stored = mapped_key;
            return EventHandlerResult::OK;
          } else {
            stored_modifiers = Keyboard.lastKeyReport.modifiers;
            f_stored = mapped_key;
            return EventHandlerResult::EVENT_CONSUMED;
          }
        } else if (Layer.lookup(key_addr) == Key_D) {
          if (f_stored != Key_NoKey) {
            f_stored = Key_NoKey;
            d_handled = true;
            typeKey(Key_Escape, Keyboard.lastKeyReport.modifiers, true); 
            return EventHandlerResult::EVENT_CONSUMED;
          } else {
            return EventHandlerResult::OK;
          }
        }

        // interrupted
        if (f_stored != Key_NoKey) {
          typeKey(f_stored, stored_modifiers, true);
          f_stored = Key_NoKey;
        }
        return EventHandlerResult::OK;
      } else if (keyIsPressed(key_state) && keyWasPressed(key_state)) {
        if (Layer.lookup(key_addr) == Key_F) {
          if (f_handled) {
            return EventHandlerResult::EVENT_CONSUMED;
          }
        } else if (Layer.lookup(key_addr) == Key_D) {
          if (d_handled) {
            return EventHandlerResult::EVENT_CONSUMED;
          }
        }
      } else if (keyToggledOff(key_state)) {
        if (Layer.lookup(key_addr) == Key_F) {
          if (f_stored != Key_NoKey) {
            typeKey(f_stored, stored_modifiers, true);
            f_stored = Key_NoKey;
          }
          f_handled = false;
        } else if (Layer.lookup(key_addr) == Key_D) {
          d_handled = false;
        }
      }
    }
    return EventHandlerResult::OK;
  }

  EventHandlerResult FDEscape::afterEachCycle() {
    if (Kaleidoscope.millisAtCycleStart() - start_time > time_out) {
      f_handled = false;
      if (f_stored != Key_NoKey) {
        typeKey(f_stored, stored_modifiers, false);
        f_stored = Key_NoKey;
      }
    }
  }
}

kaleidoscope::FDEscape FDEscape;

const macro_t *macroAction(uint8_t macroIndex, uint8_t keyState) {
  switch (macroIndex) {
    case MACRO_PROG:
      // Kaleidoscope.device().rebootBootloader();
      break;
  }
  return MACRO_NONE;
}

// First, tell Kaleidoscope which plugins you want to use.
// The order can be important. For example, LED effects are
// added in the order they're listed here.
KALEIDOSCOPE_INIT_PLUGINS(
  MouseKeys,
  HostPowerManagement,
  USBQuirks,
  TapDance,
  FDEscape,
  Macros
);

void setup() {
  Kaleidoscope.setup();
}

void loop() {
  Kaleidoscope.loop();
}
