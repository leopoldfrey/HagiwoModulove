#include <arythmatik.h>

/**
 * @file Uncertainty.ino
 * @author Adam Wonak (https://github.com/awonak/) MOD LF
 * @brief Stochastic gate processor based on the Olivia Artz Modular's Uncertainty.
 * @version 0.2
 * @date 2023-07-19
 *
 * @copyright Copyright (c) 2023
 *
 * Connect a trigger or gate source to the CLK or RST input and the each output will
 * mirror that signal according to a decreasing probability and various opetation modes.
 */


// Script specific output class.
#include "output.h"

//#define DEBUG

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;

ProbablisticOutput outputs[OUTPUT_COUNT];

// Enum constants for current display page.
enum MenuPage {
  PAGE_MAIN,
  PAGE_MODE,
};
MenuPage selected_page = PAGE_MAIN;


// Script config definitions
const byte PARAM_COUNT = 2;  // Count of editable parameters.
const byte MODE_COUNT = 12;
const byte MODE_COUNT_CH1 = 4;

// Script state variables.
byte selected_out = 0;
byte selected_param = 0;
byte selected_mode = 0;
byte last_mode = 0;
bool state_changed = false;

bool clockA = false;
bool clockB = false;

bool update_display = true;

// Script state & storage variables.
// Expected version string for this firmware.
const char SCRIPT_NAME[] = "UNC_RMX2";
const byte SCRIPT_VER = 2;

struct State {
  // Version check.
  char script[sizeof(SCRIPT_NAME)];
  byte version;
  // State variables.
  byte probability[OUTPUT_COUNT];
  byte modes[OUTPUT_COUNT];
  byte clocks[OUTPUT_COUNT];
  byte divisions[OUTPUT_COUNT];
};
State state;

// Initialize script state from EEPROM or default values.
void InitState() {
  // Read previously put state from EEPROM memory. If it doesn't exist or
  // match version, then populate the State struct with default values.
  EEPROM.get(0, state);
  //Serial.print("STATE ");
  //Serial.print(state.script);
  //Serial.print(" ");
  //Serial.println(state.version);

  // Check if the data in memory matches expected values.
  if ((strcmp(state.script, SCRIPT_NAME) != 0) || (state.version != SCRIPT_VER)) {
    //Serial.print("INIT STATE");
    // DEFAULT INIT
    strcpy(state.script, SCRIPT_NAME);
    state.version = SCRIPT_VER;

    state.probability[0] = 90;
    state.modes[0] = CLOCK_DIV;
    state.clocks[0] = 0;
    state.divisions[0] = 1;

    state.probability[1] = 80;
    state.modes[1] = TRIGGER;
    state.clocks[1] = 0;
    state.divisions[1] = 0;

    state.probability[2] = 50;
    state.modes[2] = PREV_BERNOULLI;
    state.clocks[2] = 0;
    state.divisions[2] = 0;

    state.probability[3] = 90;
    state.modes[3] = CLOCK_DIV;
    state.clocks[3] = 1;
    state.divisions[3] = 2;

    state.probability[4] = 80;
    state.modes[4] = TRIGGER;
    state.clocks[4] = 1;
    state.divisions[4] = 0;

    state.probability[5] = 50;
    state.modes[5] = TRIGGER;
    state.clocks[5] = 2;
    state.divisions[5] = 0;
  }
  // INIT FROM EEPROM
  for (int i = 0; i < OUTPUT_COUNT; i++) {
    //Serial.print("INIT ");
    //Serial.print(i);
    //Serial.print(" ");
    //Serial.print(state.probability[i]);
    //Serial.print(" ");
    //Serial.print(state.modes[i]);
    //Serial.print(" ");
    //Serial.print(state.clocks[i]);
    //Serial.print(" ");
    //Serial.print(state.divisions[i]);
    //Serial.println();
    outputs[i].Init(hw.outputs[i], state.probability[i], state.modes[i], state.clocks[i], state.divisions[i]);
  }
}

// Save state to EEPROM if state has changed.
void SaveChanges() {
  if (!state_changed) return;
  state_changed = false;
  //Serial.println("Save changes");

  byte _prob[OUTPUT_COUNT] = {
    outputs[0].GetProbInt(),
    outputs[1].GetProbInt(),
    outputs[2].GetProbInt(),
    outputs[3].GetProbInt(),
    outputs[4].GetProbInt(),
    outputs[5].GetProbInt(),
  };

  memcpy(state.probability, _prob, sizeof(_prob));

  byte _modes[OUTPUT_COUNT] = {
    outputs[0].GetMode(),
    outputs[1].GetMode(),
    outputs[2].GetMode(),
    outputs[3].GetMode(),
    outputs[4].GetMode(),
    outputs[5].GetMode(),
  };

  memcpy(state.modes, _modes, sizeof(_modes));

  byte _clocks[OUTPUT_COUNT] = {
    outputs[0].GetClock(),
    outputs[1].GetClock(),
    outputs[2].GetClock(),
    outputs[3].GetClock(),
    outputs[4].GetClock(),
    outputs[5].GetClock(),
  };

  memcpy(state.clocks, _clocks, sizeof(_clocks));

  byte _divisions[OUTPUT_COUNT] = {
    outputs[0].GetDivision(),
    outputs[1].GetDivision(),
    outputs[2].GetDivision(),
    outputs[3].GetDivision(),
    outputs[4].GetDivision(),
    outputs[5].GetDivision(),
  };

  memcpy(state.divisions, _divisions, sizeof(_divisions));

  EEPROM.put(0, state);
}

void setup() {

#ifdef DEBUG
  //Serial.begin(115200);
  //Serial.println();
  //Serial.println();
  //Serial.println();
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

  // EEPROM STATE
  InitState();

  // CLOCK LED (DIGITAL)
  pinMode(CLOCK_LED, OUTPUT);

  // OLED Display configuration.
  hw.display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  hw.display.setTextColor(WHITE);
  hw.display.clearDisplay();
  hw.display.display();
  //Serial.println("Uncertainty RMX Ready!");
}

void loop() {
  hw.ProcessInputs();
  // Render any new UI changes to the OLED display.
  UpdateDisplay();
}

void UpdateLeds() {
  switch (outputs[selected_out].GetClock()) {
    case A:
      if (clockA == HIGH) {
        digitalWrite(CLOCK_LED, HIGH);
      } else {
        digitalWrite(CLOCK_LED, LOW);
      }
      break;
    case B:
      if (clockB == HIGH) {
        digitalWrite(CLOCK_LED, HIGH);
      } else {
        digitalWrite(CLOCK_LED, LOW);
      }
      break;
    case BOTH:
      if (clockA == HIGH || clockB == HIGH) {
        digitalWrite(CLOCK_LED, HIGH);
      } else if (clockA == LOW && clockB == LOW) {
        digitalWrite(CLOCK_LED, LOW);
      }
      break;
  }
}

void HandleClock() {
  // Input clock has gone high, call each output's On() for a chance to trigger that output.
  bool prevOut = false;
  clockA = hw.clk.Read();
  if (clockA == HIGH) {
    prevOut = false;
    for (byte i = 0; i < OUTPUT_COUNT; i++) {
      prevOut = outputs[i].On(Clock::A, prevOut);
    }
  } else if (clockA == LOW) {
    prevOut = false;
    for (byte i = 0; i < OUTPUT_COUNT; i++) {
      prevOut = outputs[i].Off(Clock::A, prevOut);
    }
  }
  UpdateLeds();
}

// RESET INPUT IS CLOCK B
void HandleReset() {
  bool prevOut = false;
  clockB = hw.rst.Read();
  if (clockB == HIGH) {
    for (byte i = 0; i < OUTPUT_COUNT; i++) {
      prevOut = outputs[i].On(Clock::B, prevOut);
    }
  } else if (clockB == LOW) {
    for (byte i = 0; i < OUTPUT_COUNT; i++) {
      prevOut = outputs[i].Off(Clock::B, prevOut);
    }
  }
  UpdateLeds();
}

// Short button press. Change editable parameter.
void UpdatePress(EncoderButton &eb) {
  //Serial.println("Short Press");
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
  SaveChanges();
  update_display = true;
}

// Long button press. Change menu page.
void UpdateLongPress(EncoderButton &eb) {
  //Serial.println("Long Press");
  switch (selected_page) {
    case PAGE_MAIN:
      selected_page = PAGE_MODE;
      selected_param = 1;
      break;
    case PAGE_MODE:
      selected_page = PAGE_MAIN;
      switch (last_mode) {
        case PREV_COPY:
        case PREV_BERNOULLI:
        case PREV_BERNOULLI_FLIP:
        case OFF:
          selected_param = 0;
          break;
        default:
          selected_param = 1;
          break;
      }
      break;
  }
  SaveChanges();
  update_display = true;
}

void UpdateRotate(EncoderButton &eb) {
  //void UpdateParameter(Direction dir) {
  Direction dir = hw.EncoderDirection();
  //Serial.print("Rotate ");
  //Serial.println(dir);
  //update_display = true;

  if (dir == DIRECTION_UNCHANGED) return;
  switch (selected_param) {
    case 0:
      UpdateOutput(dir);
      break;
    case 1:
      if (selected_page == PAGE_MAIN)
        UpdateProb(dir);
      else
        UpdateMode(dir);
      break;
    case 2:
      UpdateClock(dir);
      break;
  }
}

void UpdateOutput(Direction dir) {
  if (dir == DIRECTION_DECREMENT && selected_out < OUTPUT_COUNT - 1)
    selected_out++;
  if (dir == DIRECTION_INCREMENT && selected_out > 0)
    selected_out--;
  update_display = true;
}

void UpdateMode(Direction dir) {
  byte newMode;
  if (dir == DIRECTION_DECREMENT)
    newMode = outputs[selected_out].GetMode() + 1;
  if (dir == DIRECTION_INCREMENT) {
    newMode = outputs[selected_out].GetMode() - 1;
    newMode = newMode >= -1 ? newMode : -1;
  }

  // Update for current output only
  if (selected_out == 0) {
    newMode %= MODE_COUNT_CH1;
    newMode = newMode >= 0 ? newMode : 0;
  } else
    newMode %= MODE_COUNT;
  outputs[selected_out].SetMode(newMode);
  state_changed = true;
  update_display = true;
}

void UpdateProb(Direction dir) {
  if (dir == DIRECTION_DECREMENT) outputs[selected_out].IncProb();
  if (dir == DIRECTION_INCREMENT) outputs[selected_out].DecProb();
  state_changed = true;
  update_display = true;
}

void UpdateClock(Direction dir) {
  if (dir == DIRECTION_DECREMENT) {
    switch (outputs[selected_out].GetClock()) {
      case 0:
        outputs[selected_out].SetClock(1);
        break;
      case 1:
        outputs[selected_out].SetClock(2);
        break;
      case 2:
        outputs[selected_out].SetClock(0);
        break;
    }
  } else if (dir == DIRECTION_INCREMENT) {
    switch (outputs[selected_out].GetClock()) {
      case 0:
        outputs[selected_out].SetClock(2);
        break;
      case 1:
        outputs[selected_out].SetClock(0);
        break;
      case 2:
        outputs[selected_out].SetClock(1);
        break;
    }
  }
  state_changed = true;
  update_display = true;
}

void UpdateDisplay() {
  if (!update_display) return;
  //Serial.println("Display loop");
  update_display = false;
  hw.display.clearDisplay();

  switch (selected_page) {
    case PAGE_MAIN:
      DisplayMainPage();
      break;
    case PAGE_MODE:
      // Draw app title.
      hw.display.setTextSize(0);
      hw.display.setCursor(21, 0);
      hw.display.setTextColor(SSD1306_WHITE);
      hw.display.println("UNCERTAINTY RMX");
      hw.display.drawFastHLine(0, 10, SCREEN_WIDTH, WHITE);
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
  byte barHeight = 35;
  byte padding = 4;
  byte probFill = 0;
  byte probTop = 0;

  for (byte i = 0; i < OUTPUT_COUNT; i++) {
    switch (outputs[i].GetMode()) {
      case 0:  // TRIGGER
        // Draw output probability bar.
        hw.display.drawRect(left, top, barWidth, barHeight, 1);

        // Fill current bar probability.
        probFill = (float(barHeight) * outputs[i].GetProb());
        probTop = top + barHeight - probFill;
        hw.display.fillRect(left, probTop, barWidth, probFill, 1);

        if (outputs[i].GetProb() > 0. && outputs[i].GetProb() < 1.) {
          hw.display.setTextSize(0);
          hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
          if (outputs[i].GetProb() >= 0.5)
            hw.display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
          else
            hw.display.setTextColor(SSD1306_WHITE);
          hw.display.println(int(outputs[i].GetProb() * 100.));
        }

        break;
      case 1:  // FLIP
        // Draw output probability bar.
        hw.display.drawRect(left, top, barWidth, barHeight, 1);

        // Fill current bar probability.
        probFill = (float(barHeight) * outputs[i].GetProb());
        probTop = top + barHeight - probFill;
        hw.display.fillRect(left, top, barWidth, barHeight - probFill, 1);

        if (outputs[i].GetProb() > 0. && outputs[i].GetProb() < 1.) {
          hw.display.setTextSize(1);
          hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
          if (outputs[i].GetProb() >= 0.5)
            hw.display.setTextColor(SSD1306_WHITE);
          else
            hw.display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
          hw.display.println(int(outputs[i].GetProb() * 100.));
        }

        break;
      case 2:  // CLOCK_DIV
        hw.display.drawRect(left, top, barWidth, barHeight, 1);

        hw.display.setTextSize(1);
        hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
        hw.display.setTextColor(SSD1306_WHITE);
        hw.display.println(outputs[i].DisplayDivision());
        break;
      case 3:  // CLOCK_DIV_FLIP
        hw.display.fillRect(left, top, barWidth, barHeight, 1);

        hw.display.setTextSize(1);
        hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
        hw.display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        hw.display.println(outputs[i].DisplayDivision());
        break;
      case 4:  // PREV_COPY
        hw.display.drawRect(left, top, barWidth, barHeight, 1);
        break;
      case 5:  // PREV_BERNOULLI
      case 6:  // PREV_BERNOULLI_FLIP
        hw.display.fillRect(left, top, barWidth, barHeight, 1);
        break;
        hw.display.fillRect(left, top, barWidth, barHeight, 1);
        break;
      case 7:  // PREV_FOLLOW
        // Draw output probability bar.
        hw.display.drawRect(left, top, barWidth, barHeight, 1);

        // Fill current bar probability.
        probFill = (float(barHeight) * outputs[i].GetProb());
        probTop = top + barHeight - probFill;
        hw.display.fillRect(left, probTop, barWidth, probFill, 1);

        if (outputs[i].GetProb() > 0. && outputs[i].GetProb() < 1.) {
          hw.display.setTextSize(1);
          hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
          if (outputs[i].GetProb() >= 0.5)
            hw.display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
          else
            hw.display.setTextColor(SSD1306_WHITE);
          hw.display.println(int(outputs[i].GetProb() * 100.));
        }

        break;
      case 8:  // PREV_FOLLOW_FLIP
        // Draw output probability bar.
        hw.display.drawRect(left, top, barWidth, barHeight, 1);

        // Fill current bar probability.
        probFill = (float(barHeight) * outputs[i].GetProb());
        probTop = top + barHeight - probFill;
        hw.display.fillRect(left, top, barWidth, barHeight - probFill, 1);

        if (outputs[i].GetProb() > 0. && outputs[i].GetProb() < 1.) {
          hw.display.setTextSize(1);
          hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
          if (outputs[i].GetProb() >= 0.5)
            hw.display.setTextColor(SSD1306_WHITE);
          else
            hw.display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
          hw.display.println(int(outputs[i].GetProb() * 100.));
        }

        break;
      case 9:  // PREV_CLOCK_DIV
        hw.display.drawRect(left, top, barWidth, barHeight, 1);

        hw.display.setTextSize(1);
        hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
        hw.display.setTextColor(SSD1306_WHITE);
        hw.display.println(outputs[i].DisplayDivision());
        break;
      case 10:  // PREV_CLOCK_DIV_FLIP
        hw.display.fillRect(left, top, barWidth, barHeight, 1);

        hw.display.setTextSize(1);
        hw.display.setCursor(left + 3, SCREEN_HEIGHT - 48);
        hw.display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        hw.display.println(outputs[i].DisplayDivision());
        break;
      case 11:  // OFF
        break;
    }

    hw.display.setTextColor(SSD1306_WHITE);
    // Show selected output.
    if (i == selected_out) {
      (selected_param == 0)
        ? hw.display.drawChar(left + 5, SCREEN_HEIGHT - 26, 0x18, 1, 0, 1)
        : hw.display.drawChar(left + 5, SCREEN_HEIGHT - 26, 0x25, 1, 0, 1);
    }

    hw.display.setCursor(left + 5, SCREEN_HEIGHT - 17);
    hw.display.println(outputs[i].DisplayModeShort());
    // DISPLAY CLOCK A/B/*
    hw.display.setCursor(left + 5, SCREEN_HEIGHT - 8);
    hw.display.println(outputs[i].DisplayClock());
    //}
    left += barWidth + padding;
  }
}

void DisplayModePage() {
  // Show mode edit page.
  hw.display.setTextSize(1);
  byte lineheight = 12;
  byte padding = 2;
  switch (selected_param) {
    case 0:
      hw.display.setCursor(6, lineheight);
      hw.display.println("Output >" + String(selected_out + 1) + "<");
      hw.display.setCursor(6, 2 * lineheight + padding);
      hw.display.println("Mode " + outputs[selected_out].DisplayMode());
      hw.display.setCursor(6, 3 * lineheight + 2 * padding);
      hw.display.println("Clock " + String(outputs[selected_out].DisplayClock()));
      break;
    case 1:
      hw.display.setCursor(6, lineheight);
      hw.display.println("Output " + String(selected_out + 1));
      hw.display.setCursor(6, 2 * lineheight + padding);
      hw.display.println("Mode >" + outputs[selected_out].DisplayMode() + "<");
      hw.display.setCursor(6, 3 * lineheight + 2 * padding);
      hw.display.println("Clock " + String(outputs[selected_out].DisplayClock()));
      break;
    case 2:
      hw.display.setCursor(6, lineheight);
      hw.display.println("Output " + String(selected_out + 1));
      hw.display.setCursor(6, 2 * lineheight + padding);
      hw.display.println("Mode " + outputs[selected_out].DisplayMode());
      hw.display.setCursor(6, 3 * lineheight + 2 * padding);
      hw.display.println("Clock >" + String(outputs[selected_out].DisplayClock()) + "<");
      break;
    case 3:
      hw.display.setCursor(6, lineheight);
      hw.display.println("Output " + String(selected_out + 1));
      hw.display.setCursor(6, 2 * lineheight + padding);
      hw.display.println("Mode " + outputs[selected_out].DisplayMode());
      hw.display.setCursor(6, 3 * lineheight + 2 * padding);
      hw.display.println("Clock " + String(outputs[selected_out].DisplayClock()));
      break;
  }

  last_mode = outputs[selected_out].GetMode();
}
