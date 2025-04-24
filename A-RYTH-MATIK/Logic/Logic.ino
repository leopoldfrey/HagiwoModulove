#include <arythmatik.h>

//#define DEBUG
#define ENCODER_REVERSED

using namespace modulove;
using namespace arythmatik;

// Declare A-RYTH-MATIK hardware variable.
Arythmatik hw;

bool clockA = false;
bool clockB = false;

bool update_display = true;

bool logicAnd = false;
bool logicOr = false;
bool logicXor = false;

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

void loop() {
  hw.ProcessInputs();
  UpdateDisplay();
}

void UpdatePress(EncoderButton &eb) {
  //Serial.println("Short Press");
}

void UpdateLongPress(EncoderButton &eb) {
  //Serial.println("Long Press");
}

void UpdateRotate(EncoderButton &eb) {
  Direction dir = hw.EncoderDirection();
  //Serial.print("Rotate ");
  //Serial.println(dir);
  //if (dir == DIRECTION_UNCHANGED) return;
}

void UpdateLeds() {
  digitalWrite(CLOCK_LED, clockA || clockB);
}

void UpdateOutputs() {
  logicAnd = clockA && clockB;
  logicOr = clockA || clockB;
  logicXor = clockA ^ clockB;
  hw.outputs[0].Update(logicAnd);
  hw.outputs[1].Update(logicOr);
  hw.outputs[2].Update(logicXor);
  hw.outputs[3].Update(!logicAnd);
  hw.outputs[4].Update(!logicOr);
  hw.outputs[5].Update(!logicXor);
}

void HandleClock() {
  // Input clock has gone high, call each output's On() for a chance to trigger that output.
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
  //Serial.println("Display loop");
  update_display = false;
  hw.display.clearDisplay();

  hw.display.setTextSize(1);
  byte lineheight = 12;
  byte padding = 2;
  hw.display.setCursor(6, lineheight);
  hw.display.println("A");
  hw.display.setCursor(30, lineheight);
  hw.display.print(clockA ? "[x]" : "[ ]");
  hw.display.setCursor(70, lineheight);
  hw.display.println("B");
  hw.display.setCursor(100, lineheight);
  hw.display.print(clockB ? "[x]" : "[ ]");
  hw.display.setCursor(6, 2 * lineheight + padding);
  hw.display.println(F("AND"));
  hw.display.setCursor(30, 2 * lineheight + padding);
  hw.display.print(logicAnd ? "[x]" : "[ ]");
  hw.display.setCursor(70, 2 * lineheight + padding);
  hw.display.println(F("NAND"));
  hw.display.setCursor(100, 2 * lineheight + padding);
  hw.display.print(!logicAnd ? "[x]" : "[ ]");
  hw.display.setCursor(6, 3 * lineheight + 2 * padding);
  hw.display.println(F("OR"));
  hw.display.setCursor(30, 3 * lineheight + 2 * padding);
  hw.display.println(logicOr ? "[x]" : "[ ]");
  hw.display.setCursor(70, 3 * lineheight + padding);
  hw.display.println(F("NOR"));
  hw.display.setCursor(100, 3 * lineheight + padding);
  hw.display.print(!logicOr ? "[x]" : "[ ]");
  hw.display.setCursor(6, 4 * lineheight + 2 * padding);
  hw.display.println(F("XOR"));
  hw.display.setCursor(30, 4 * lineheight + 2 * padding);
  hw.display.println(logicXor ? "[x]" : "[ ]");
  hw.display.setCursor(70, 4 * lineheight + padding);
  hw.display.println(F("NXOR"));
  hw.display.setCursor(100, 4 * lineheight + padding);
  hw.display.print(!logicXor ? "[x]" : "[ ]");
  hw.display.display();
}
