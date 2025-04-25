#ifndef OUTPUT_H
#define OUTPUT_H

#define MODE_OFF 0
#define MODE_A 1
#define MODE_NOTA 2
#define MODE_B 3
#define MODE_NOTB 4
#define MODE_AND 5
#define MODE_NAND 6
#define MODE_OR 7
#define MODE_NOR 8
#define MODE_XOR 9
#define MODE_XNOR 10
#define NMODE 10

// Include the Modulove library.
#include <arythmatik.h>

using namespace modulove;

class LogicOutput {
public:
  LogicOutput() {}
  ~LogicOutput() {}

  void Init(DigitalOutput output, byte mode) {
    output_ = output;
    SetMode(mode);
  }

  inline void Update(bool clockA, bool clockB) {
    switch (mode_) {
      case MODE_OFF:
        Off();
        return;
      case MODE_A:
        if (clockA)
          On();
        else
          Off();
        return;
      case MODE_NOTA:
        if (!clockA)
          On();
        else
          Off();
        return;
      case MODE_B:
        if (clockB)
          On();
        else
          Off();
        return;
      case MODE_NOTB:
        if (!clockB)
          On();
        else
          Off();
        return;
      case MODE_AND:
        if (clockA && clockB)
          On();
        else
          Off();
        return;
      case MODE_NAND:
        if (!(clockA && clockB))
          On();
        else
          Off();
        return;
      case MODE_OR:
        if (clockA || clockB)
          On();
        else
          Off();
        return;
      case MODE_NOR:
        if (!(clockA || clockB))
          On();
        else
          Off();
        return;
      case MODE_XOR:
        if (clockA ^ clockB)
          On();
        else
          Off();
        return;
      case MODE_XNOR:
        if (!(clockA ^ clockB))
          On();
        else
          Off();
        return;
    }
  }

  inline void On() {
    high();
  }

  inline void Off() {
    low();
  }

  inline bool State() {
    return output_.On();
  }
  inline byte GetMode() {
    return mode_;
  }
  inline String GetModeName() {
    switch (mode_) {
      case MODE_OFF:
        return F(" off");
      case MODE_A:
        return F("   A");
      case MODE_NOTA:
        return F("notA");
      case MODE_B:
        return F("   B");
      case MODE_NOTB:
        return F("notB");
      case MODE_AND:
        return F(" And");
      case MODE_NAND:
        return F("nAnd");
      case MODE_OR:
        return F("  Or");
      case MODE_NOR:
        return F(" nOr");
      case MODE_XOR:
        return F(" Xor");
      case MODE_XNOR:
        return F("Xnor");
    }
  }
  inline void IncMode() {
    mode_ = constrain(mode_ + 1, 0, NMODE);
  }
  inline void DecMode() {
    mode_ = constrain(mode_ - 1, 0, NMODE);
  }
  inline void SetMode(byte b) {
    mode_ = constrain(b, 0, NMODE);
  }

private:
  DigitalOutput output_;
  byte mode_;

  inline void high() {
    output_.High();
  }

  inline void low() {
    output_.Low();
  }
};

#endif
