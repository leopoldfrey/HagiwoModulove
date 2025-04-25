#include <arythmatik.h>
#include "output.h"

//#define DEBUG
#define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;

LogicOutput outputs[OUTPUT_COUNT];

bool clockA = false;
bool clockB = false;

const uint8_t PARAM_COUNT = 2;
byte selected_out = 0;
byte selected_param = 0;

bool update_display = true;
bool state_changed = false;

// Script state & storage variables.
// Expected version string for this firmware.
const char SCRIPT_NAME[] = "LOGIC";
const byte SCRIPT_VER = 1;

struct State {
  // Version check.
  char script[sizeof(SCRIPT_NAME)];
  byte version;
  // State variables.
  byte modes[OUTPUT_COUNT];
};
State state;

// Initialize script state from EEPROM or default values.
void InitState() {
  // Read previously put state from EEPROM memory. If it doesn't exist or
  // match version, then populate the State struct with default values.
  EEPROM.get(0, state);

  // Check if the data in memory matches expected values.
  if ((strcmp(state.script, SCRIPT_NAME) != 0) || (state.version != SCRIPT_VER)) {
    // DEFAULT INIT
    strcpy(state.script, SCRIPT_NAME);
    state.version = SCRIPT_VER;

    state.modes[0] = MODE_AND;
    state.modes[1] = MODE_OR;
    state.modes[2] = MODE_NOTA;
    state.modes[3] = MODE_NAND;
    state.modes[4] = MODE_XOR;
    state.modes[5] = MODE_NOTB;
  }

  // INIT FROM EEPROM
  for (int i = 0; i < OUTPUT_COUNT; i++) {
    outputs[i].Init(hw.outputs[i], state.modes[i]);
  }
}

void setup() {

#ifdef ROTATE_PANEL
  hw.config.RotatePanel = true;
#endif

#ifdef ENCODER_REVERSED
  hw.config.ReverseEncoder = true;
#endif

  // Initialize the A-RYTH-MATIK peripherials.
  hw.eb.setDebounceInterval(0);
  hw.eb.setLongClickDuration(600);
  hw.eb.setClickHandler(UpdatePress);
  hw.eb.setLongPressHandler(UpdateLongPress);
  hw.eb.setEncoderHandler(UpdateRotate);
  hw.AttachClockHandler(HandleClock);
  hw.AttachResetHandler(HandleReset);

  hw.Init();

  InitState();

  /*
  outputs[0].Init(hw.outputs[0], MODE_AND);
  outputs[1].Init(hw.outputs[1], MODE_OR);
  outputs[2].Init(hw.outputs[2], MODE_XOR);
  outputs[3].Init(hw.outputs[3], MODE_NAND);
  outputs[4].Init(hw.outputs[4], MODE_NOR);
  outputs[5].Init(hw.outputs[5], MODE_XNOR);//*/

  // CLOCK LED (DIGITAL)
  pinMode(CLOCK_LED, OUTPUT);

  // OLED Display configuration.
  hw.display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  hw.display.setTextColor(WHITE);
  hw.display.clearDisplay();
  hw.display.display();

  UpdateOutputs();
  UpdateLeds();
}

void SaveState() {
  if (!state_changed) return;
  state_changed = false;

  byte _modes[OUTPUT_COUNT] = {
    outputs[0].GetMode(),
    outputs[1].GetMode(),
    outputs[2].GetMode(),
    outputs[3].GetMode(),
    outputs[4].GetMode(),
    outputs[5].GetMode(),
  };

  memcpy(state.modes, _modes, sizeof(_modes));

  EEPROM.put(0, state);
}

void loop() {
  hw.ProcessInputs();
  UpdateDisplay();
  SaveState();
}

void UpdatePress(EncoderButton &eb) {
  //Serial.println("Short Press");
  selected_param = ++selected_param % PARAM_COUNT;
  update_display = true;
}

void UpdateLongPress(EncoderButton &eb) {
  //Serial.println("Long Press");
}

void UpdateRotate(EncoderButton &eb) {
  // Convert the configured encoder direction to an integer equivalent value.
  int dir = hw.EncoderDirection() == DIRECTION_INCREMENT ? 1 : -1;
  switch (selected_param) {
    case 0:
      UpdateOutput(dir);
      break;
    case 1:
      UpdateMode(dir);
      break;
  }
  state_changed = true;
  update_display = true;
}

void UpdateOutput(int dir) {
  if (dir == 1 && selected_out < OUTPUT_COUNT - 1)
    selected_out++;
  if (dir == -1 && selected_out > 0)
    selected_out--;
}

void UpdateMode(int dir) {
  if (dir == 1) outputs[selected_out].IncMode();
  if (dir == -1) outputs[selected_out].DecMode();
  UpdateOutputs();
}

void UpdateLeds() {
  digitalWrite(CLOCK_LED, clockA || clockB);
}

void UpdateOutputs() {
  for (int i = 0; i < OUTPUT_COUNT; i++) {
    outputs[i].Update(clockA, clockB);
  }
}

void HandleClock() {
  clockA = hw.clk.Read();
  update_display = true;
  UpdateOutputs();
  UpdateLeds();
}

// RESET INPUT IS CLOCK B
void HandleReset() {
  clockB = hw.rst.Read();
  update_display = true;
  UpdateOutputs();
  UpdateLeds();
}

void UpdateDisplay() {
  if (!update_display) return;
  update_display = false;
  hw.display.clearDisplay();

  hw.display.setTextSize(1);
  static const byte lineheight = 12;
  static const byte padding = 2;
  static const byte left = 8;
  static const byte cursPosA = left + 24;
  static const byte cursPosB = 0;
  static const byte left2 = left + 24 + 10;
  static const byte right = 70;
  static const byte cursPos2A = right - 6;
  static const byte cursPos2B = right + 24;
  static const byte right2 = right + 24 + 10;
  static const char selCharA = '<';
  static const char selCharB = '>';
  static const char modCharA = '[';
  static const char modCharB = ']';

  // DISPLAY IN A
  hw.display.setCursor(left, lineheight);
  hw.display.println("   A");
  hw.display.setCursor(left2, lineheight);
  hw.display.print(clockA ? "[x]" : "[ ]");
  // DISPLAY IN B
  hw.display.setCursor(right, lineheight);
  hw.display.println("   B");
  hw.display.setCursor(right2, lineheight);
  hw.display.print(clockB ? "[x]" : "[ ]");

  // CURSOR
  switch (selected_out) {
    case 0:
      hw.display.drawChar(cursPosB, 2 * lineheight + padding, selected_param == 0 ? selCharA : modCharA, 1, 0, 1);
      hw.display.drawChar(cursPosA, 2 * lineheight + padding, selected_param == 0 ? selCharB : modCharB, 1, 0, 1);
      break;
    case 1:
      hw.display.drawChar(cursPosB, 3 * lineheight + 2 * padding, selected_param == 0 ? selCharA : modCharA, 1, 0, 1);
      hw.display.drawChar(cursPosA, 3 * lineheight + 2 * padding, selected_param == 0 ? selCharB : modCharB, 1, 0, 1);
      break;
    case 2:
      hw.display.drawChar(cursPosB, 4 * lineheight + 2 * padding, selected_param == 0 ? selCharA : modCharA, 1, 0, 1);
      hw.display.drawChar(cursPosA, 4 * lineheight + 2 * padding, selected_param == 0 ? selCharB : modCharB, 1, 0, 1);
      break;
    case 3:
      hw.display.drawChar(cursPos2B, 2 * lineheight + padding, selected_param == 0 ? selCharB : modCharB, 1, 0, 1);
      hw.display.drawChar(cursPos2A, 2 * lineheight + padding, selected_param == 0 ? selCharA : modCharA, 1, 0, 1);
      break;
    case 4:
      hw.display.drawChar(cursPos2B, 3 * lineheight + 2 * padding, selected_param == 0 ? selCharB : modCharB, 1, 0, 1);
      hw.display.drawChar(cursPos2A, 3 * lineheight + 2 * padding, selected_param == 0 ? selCharA : modCharA, 1, 0, 1);
      break;
    case 5:
      hw.display.drawChar(cursPos2B, 4 * lineheight + 2 * padding, selected_param == 0 ? selCharB : modCharB, 1, 0, 1);
      hw.display.drawChar(cursPos2A, 4 * lineheight + 2 * padding, selected_param == 0 ? selCharA : modCharA, 1, 0, 1);
      break;
  }

  // DISPLAY OUT 1
  hw.display.setCursor(left, 2 * lineheight + padding);
  hw.display.println(outputs[0].GetModeName());
  hw.display.setCursor(left2, 2 * lineheight + padding);
  hw.display.print(outputs[0].State() ? "[x]" : "[ ]");

  // DISPLAY OUT 2
  hw.display.setCursor(left, 3 * lineheight + 2 * padding);
  hw.display.println(outputs[1].GetModeName());
  hw.display.setCursor(left2, 3 * lineheight + 2 * padding);
  hw.display.println(outputs[1].State() ? "[x]" : "[ ]");

  // DISPLAY OUT 3
  hw.display.setCursor(left, 4 * lineheight + 2 * padding);
  hw.display.println(outputs[2].GetModeName());
  hw.display.setCursor(left2, 4 * lineheight + 2 * padding);
  hw.display.println(outputs[2].State() ? "[x]" : "[ ]");

  // DISPLAY OUT 4
  hw.display.setCursor(right, 2 * lineheight + padding);
  hw.display.println(outputs[3].GetModeName());
  hw.display.setCursor(right2, 2 * lineheight + padding);
  hw.display.print(outputs[3].State() ? "[x]" : "[ ]");

  // DISPLAY OUT 5
  hw.display.setCursor(right, 3 * lineheight + 2 * padding);
  hw.display.println(outputs[4].GetModeName());
  hw.display.setCursor(right2, 3 * lineheight + 2 * padding);
  hw.display.print(outputs[4].State() ? "[x]" : "[ ]");

  // DISPLAY OUT 6
  hw.display.setCursor(right, 4 * lineheight + 2 * padding);
  hw.display.println(outputs[5].GetModeName());
  hw.display.setCursor(right2, 4 * lineheight + 2 * padding);
  hw.display.print(outputs[5].State() ? "[x]" : "[ ]");

  //
  hw.display.display();
}
