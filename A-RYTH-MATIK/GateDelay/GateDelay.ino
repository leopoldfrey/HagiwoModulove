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
  outputs[0].Init(hw.outputs[0], 50, CLOCK_A);
  outputs[1].Init(hw.outputs[1], 100, CLOCK_A);
  outputs[2].Init(hw.outputs[2], 1000, CLOCK_A);
  outputs[3].Init(hw.outputs[3], 500, CLOCK_B);
  outputs[4].Init(hw.outputs[4], 1000, CLOCK_B);
  outputs[5].Init(hw.outputs[5], 5000, CLOCK_BOTH);

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
      selected_param = 0;
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
  update_display = true;
}

void UpdateOutput(int dir) {
  if (dir == 1 && selected_out < OUTPUT_COUNT - 1)
    selected_out++;
  if (dir == -1 && selected_out > 0)
    selected_out--;
  update_display = true;
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
  update_display = true;
}

void UpdateClock(int dir) {
  if (dir == 1) outputs[selected_out].IncClock();
  if (dir == -1) outputs[selected_out].DecClock();
  update_display = true;
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
