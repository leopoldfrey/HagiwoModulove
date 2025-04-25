// Include the Modulove library.
#include <arythmatik.h>

// Script specific output class.
#include "output.h"

// Flag for enabling debug print to serial monitoring output.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
//#define DEBUG

// Flag for rotating the panel 180 degrees.
// #define ROTATE_PANEL

// Flag for reversing the encoder direction.
#define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

// Enum constants for current display page.
enum MenuPage {
  PAGE_MAIN,
  PAGE_MODE,
};
MenuPage selected_page = PAGE_MAIN;


// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;

DelayedOutput outputs[OUTPUT_COUNT];

// Script config definitions
const uint8_t PARAM_COUNT = 2;  // Count of editable parameters.

// Script state variables.
byte selected_out = 0;
byte selected_param = 0;

bool clockA = false;
bool clockB = false;

bool update_display = true;

unsigned long currentTime = 0;

// Script state & storage variables.
// Expected version string for this firmware.
const char SCRIPT_NAME[] = "GATEDELAY";
const byte SCRIPT_VER = 3;

struct State {
  // Version check.
  char script[sizeof(SCRIPT_NAME)];
  byte version;
  // State variables.
  byte delaysBYTE0[OUTPUT_COUNT];
  byte delaysBYTE1[OUTPUT_COUNT];
  byte clocks[OUTPUT_COUNT];
};
State state;

bool state_changed = false;

void intToByte(int i, byte &b0, byte &b1) {
  b0 = i >> 8;
  b1 = i & 0xFF;
}

int byteToInt(byte b0, byte b1) {
  return (b0 << 8) + b1;
}

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

    byte b0, b1;
    intToByte(3500, b0, b1);
    state.delaysBYTE0[0] = b0;
    state.delaysBYTE1[0] = b1;
    state.clocks[0] = 1;

    intToByte(100, b0, b1);
    state.delaysBYTE0[1] = b0;
    state.delaysBYTE1[1] = b1;
    state.clocks[1] = 0;

    intToByte(1000, b0, b1);
    state.delaysBYTE0[2] = b0;
    state.delaysBYTE1[2] = b1;
    state.clocks[2] = 0;

    intToByte(500, b0, b1);
    state.delaysBYTE0[3] = b0;
    state.delaysBYTE1[3] = b1;
    state.clocks[3] = 1;

    intToByte(1000, b0, b1);
    state.delaysBYTE0[4] = b0;
    state.delaysBYTE1[4] = b1;
    state.clocks[4] = 1;

    intToByte(5000, b0, b1);
    state.delaysBYTE0[5] = b0;
    state.delaysBYTE1[5] = b1;
    state.clocks[5] = 2;
  }

  // INIT FROM EEPROM
  for (int i = 0; i < OUTPUT_COUNT; i++) {
    int del = byteToInt(state.delaysBYTE0[i], state.delaysBYTE1[i]);
    Clock c = state.clocks[i] == 0 ? CLOCK_A : (state.clocks[i] == 1 ? CLOCK_B : CLOCK_BOTH);
    outputs[i].Init(hw.outputs[i], del, c);
  }
}

void SaveState() {
  if (!state_changed) return;
  state_changed = false;

  byte _delaysB0[OUTPUT_COUNT];
  byte _delaysB1[OUTPUT_COUNT];
  byte b0, b1;
  for (int i = 0; i < OUTPUT_COUNT; i++) {
    intToByte(outputs[i].GetDelay(), b0, b1);
    _delaysB0[i] = b0;
    _delaysB1[i] = b1;
  }

  memcpy(state.delaysBYTE0, _delaysB0, sizeof(_delaysB0));
  memcpy(state.delaysBYTE1, _delaysB1, sizeof(_delaysB1));

  byte _clocks[OUTPUT_COUNT] = {
    outputs[0].GetClockByte(),
    outputs[1].GetClockByte(),
    outputs[2].GetClockByte(),
    outputs[3].GetClockByte(),
    outputs[4].GetClockByte(),
    outputs[5].GetClockByte(),
  };

  memcpy(state.clocks, _clocks, sizeof(_clocks));

  EEPROM.put(0, state);
}

void setup() {
// Only enable Serial monitoring if DEBUG is defined.
// Note: this affects performance and locks LED 4 & 5 on HIGH.
#ifdef DEBUG
  Serial.begin(115200);
#endif

#ifdef ROTATE_PANEL
  hw.config.RotatePanel = true;
#endif

#ifdef ENCODER_REVERSED
  hw.config.ReverseEncoder = true;
#endif

  hw.eb.setLongClickDuration(600);
  hw.eb.setDebounceInterval(0);
  hw.eb.setClickHandler(ShortPress);
  hw.eb.setLongPressHandler(LongPress);
  hw.eb.setEncoderHandler(UpdateParameter);

  hw.AttachClockHandler(HandleClock);
  hw.AttachResetHandler(HandleReset);

  // Initialize the A-RYTH-MATIK peripherials.
  hw.Init();

  // Initialize each of the outputs with it's GPIO pins and probability.
  InitState();
  /*outputs[0].Init(hw.outputs[0], 50, CLOCK_A);
  outputs[1].Init(hw.outputs[1], 100, CLOCK_A);
  outputs[2].Init(hw.outputs[2], 1000, CLOCK_A);
  outputs[3].Init(hw.outputs[3], 500, CLOCK_B);
  outputs[4].Init(hw.outputs[4], 1000, CLOCK_B);
  outputs[5].Init(hw.outputs[5], 5000, CLOCK_BOTH);//*/

  // CLOCK LED (DIGITAL)
  pinMode(CLOCK_LED, OUTPUT);

  // OLED Display configuration.
  hw.display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  hw.display.setTextColor(WHITE);
  hw.display.clearDisplay();
  hw.display.display();
}

void loop() {
  // Read inputs to determine state.
  hw.ProcessInputs();

  currentTime = millis();
  for (int i = 0; i < OUTPUT_COUNT; i++)
    outputs[i].current(currentTime);

  SaveState();
  UpdateDisplay();
}

void HandleClock() {
  // Input clock has gone high, call each output's On() for a chance to
  // trigger that output.
  bool prevA = clockA;
  clockA = hw.clk.Read();
  UpdateOutputs();
  UpdateLeds();
  if (prevA != clockA)
    update_display = true;
}

void HandleReset() {
  // Input reset has gone high, call each output's On() for a chance to
  // trigger that output.
  bool prevB = clockB;
  clockB = hw.rst.Read();
  UpdateOutputs();
  UpdateLeds();
  if (prevB != clockB)
    update_display = true;
}

void UpdateOutputs() {
  // Input clock has gone low, turn off Outputs.
  for (int i = 0; i < OUTPUT_COUNT; i++) {
    switch (outputs[i].GetClock()) {
      case CLOCK_A:
        if (clockA)
          outputs[i].On();
        else
          outputs[i].Off();
        break;
      case CLOCK_B:
        if (clockB)
          outputs[i].On();
        else
          outputs[i].Off();
        break;
      case CLOCK_BOTH:
        if (clockA || clockB)
          outputs[i].On();
        else if (!clockA && !clockB)
          outputs[i].Off();
        break;
    }
  }
}

void UpdateLeds() {
  switch (outputs[selected_out].GetClock()) {
    case CLOCK_A:
      if (clockA == HIGH) {
        digitalWrite(CLOCK_LED, HIGH);
      } else {
        digitalWrite(CLOCK_LED, LOW);
      }
      break;
    case CLOCK_B:
      if (clockB == HIGH) {
        digitalWrite(CLOCK_LED, HIGH);
      } else {
        digitalWrite(CLOCK_LED, LOW);
      }
      break;
    case CLOCK_BOTH:
      if (clockA == HIGH || clockB == HIGH) {
        digitalWrite(CLOCK_LED, HIGH);
      } else if (clockA == LOW && clockB == LOW) {
        digitalWrite(CLOCK_LED, LOW);
      }
      break;
  }
}

// Short button press. Change editable parameter.
void ShortPress(EncoderButton &eb) {
  // Next param on Main page.
  switch (selected_page) {
    case PAGE_MAIN:
      selected_param = ++selected_param % PARAM_COUNT;
      break;
    case PAGE_MODE:
      switch (selected_param) {
        case 0:
          selected_param = 1;
          break;
        case 1:
          selected_param = 2;
          break;
        case 2:
          selected_param = 0;
          break;
      }
      break;
  }
  update_display = true;
}

// Long button press. Change menu page.
void LongPress(EncoderButton &eb) {
  switch (selected_page) {
    case PAGE_MAIN:
      selected_page = PAGE_MODE;
      selected_param = 1;
      break;
    case PAGE_MODE:
      selected_page = PAGE_MAIN;
      selected_param = 1;
      break;
  }
  update_display = true;
}

// Read encoder for a change in direction and update the selected parameter.
void UpdateParameter(EncoderButton &eb) {
  // Convert the configured encoder direction to an integer equivalent value.
  int dir = hw.EncoderDirection() == DIRECTION_INCREMENT ? 1 : -1;
  switch (selected_param) {
    case 0:
      UpdateOutput(dir);
      break;
    case 1:
      UpdateDelay(dir);
      break;
    case 2:
      if (selected_page == PAGE_MODE)
        UpdateClock(dir);
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

void UpdateDelay(int dir) {
  if (dir == 1)
    if (selected_page == PAGE_MAIN)
      outputs[selected_out].IncDelay();
    else
      outputs[selected_out].IncDelay2();
  if (dir == -1)
    if (selected_page == PAGE_MAIN)
      outputs[selected_out].DecDelay();
    else
      outputs[selected_out].DecDelay2();
}

void UpdateClock(int dir) {
  if (dir == 1) outputs[selected_out].IncClock();
  if (dir == -1) outputs[selected_out].DecClock();
}

void UpdateDisplay() {
  if (!update_display) return;
  update_display = false;
  hw.display.clearDisplay();

  switch (selected_page) {
    case PAGE_MAIN:
      DisplayMainPage();
      break;
    case PAGE_MODE:
      DisplayModePage();
      break;
  }

  hw.display.display();
}

void DisplayMainPage() {
  // Draw boxes for pattern length.
  byte top = 2;
  byte left = 2;
  byte barWidth = 17;
  byte barHeight = 45;
  byte padding = 4;
  byte probFill = 0;
  byte probTop = 0;

  for (byte i = 0; i < OUTPUT_COUNT; i++) {
    // Draw output probability bar.
    hw.display.drawRect(left, top, barWidth, barHeight, 1);

    // Fill current bar probability.
    float delF = outputs[i].GetDelayF();
    probFill = (float(barHeight) * delF);
    probTop = top + barHeight - probFill;
    hw.display.fillRect(left, probTop, barWidth, probFill, 1);

    /*if (delF > 0. && delF < 1.) {
      hw.display.setTextSize(0);
      hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
      if (delF >= 0.5)
        hw.display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
      else
        hw.display.setTextColor(SSD1306_WHITE);
      hw.display.println(int(delF * 100.));
    }*/

    hw.display.setTextColor(SSD1306_WHITE);
    // Show selected output.
    if (i == selected_out) {
      hw.display.drawChar(left + 5, SCREEN_HEIGHT - 16, selected_param == 0 ? 0x18 : 0x25, 1, 0, 1);
    }

    // DISPLAY CLOCK A/B/*
    hw.display.setCursor(left + 5, SCREEN_HEIGHT - 8);
    hw.display.println(outputs[i].DisplayClock());
    //}
    left += barWidth + padding;
  }
}

void DisplayModePage() {
  // Draw app title.
  hw.display.setTextSize(0);
  hw.display.setCursor(36, 0);
  hw.display.println("GATE DELAYS");
  hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);

  // Show mode edit page.
  hw.display.setTextSize(1);
  byte lineheight = 15;
  byte padding = 2;
  switch (selected_param) {
    case 0:
      hw.display.setCursor(6, lineheight);
      hw.display.print("Output >");
      hw.display.print(selected_out + 1);
      hw.display.println("<");
      hw.display.setCursor(6, 2 * lineheight + padding);
      hw.display.print("Delay   ");
      hw.display.println(outputs[selected_out].GetDelay());
      hw.display.setCursor(6, 3 * lineheight + padding);
      hw.display.print("Clock   ");
      hw.display.println(outputs[selected_out].DisplayClock());
      break;
    case 1:
      hw.display.setCursor(6, lineheight);
      hw.display.print("Output  ");
      hw.display.println(selected_out + 1);
      hw.display.setCursor(6, 2 * lineheight + padding);
      hw.display.print("Delay  >");
      hw.display.print(outputs[selected_out].GetDelay());
      hw.display.println("<");
      hw.display.setCursor(6, 3 * lineheight + padding);
      hw.display.print("Clock   ");
      hw.display.println(outputs[selected_out].DisplayClock());
      break;
    case 2:
      hw.display.setCursor(6, lineheight);
      hw.display.print("Output  ");
      hw.display.println(selected_out + 1);
      hw.display.setCursor(6, 2 * lineheight + padding);
      hw.display.print("Delay   ");
      hw.display.println(outputs[selected_out].GetDelay());
      hw.display.setCursor(6, 3 * lineheight + padding);
      hw.display.print("Clock  >");
      hw.display.print(outputs[selected_out].DisplayClock());
      hw.display.println("<");
      break;
  }
}
