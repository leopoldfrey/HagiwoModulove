#ifndef OUTPUT_H
#define OUTPUT_H

// Include the Modulove library.
#include <arythmatik.h>

using namespace modulove;

const int maxDelay = 5000;
const int incDelay = 50;
const int incDelay2 = 5;
const int duration = 1000;

enum DelayStage {
  DELAY,
  GATE,
  WAIT,
};

enum Clock {
  CLOCK_A,
  CLOCK_B,
  CLOCK_BOTH
};

/*
  Class for handling delayed triggers.
*/
class DelayedOutput {
public:
  DelayedOutput() {}
  ~DelayedOutput() {}

  /**
    Initializes the probablistic cv output object with a given digital cv and
    led pin pair.
      \param cv_pin gpio pin for the cv output.
      \param led_pin gpio pin for the LED.
      \param delay delay in ms.
    */
  void Init(DigitalOutput output, int delay, Clock clock) {
    output_ = output;
    SetDelay(delay);
    SetClock(clock);
  }

  // Turn the CV and LED High according to the probability value.
  inline void On() {
    if (onStage_ == WAIT) {  // ignore events during DELAY & GATE
      trig_start_ = current_;
      onStage_ = DELAY;
    }
    //high();
  }

  // Turn off CV and LED.
  inline void Off() {
    if (onStage_ != GATE)  // ignore events while onStage is not GATE
      return;
    if (offStage_ == WAIT) {  // ignore events during DELAY & GATE
      trig_stop_ = current_;
      offStage_ = DELAY;
    }
    //low();
  }

  inline void current(unsigned long c) {
    current_ = c;
    switch (onStage_) {
      case WAIT:
        // wait for rising edge on clock
        break;
      case DELAY:
        if (current_ - trig_start_ >= del_) {  // DELAY IS OVER
          onStage_ = GATE;
          trig_start_ = current_;
          high();
        }
        break;
      case GATE:
        // wait for falling edge on clock
        /*if (current_ - trig_start_ >= duration) {  // DELAY IS OVER
          onStage_ = WAIT;
          low();
        }*/
        break;
    }
    switch (offStage_) {
      case WAIT:
        // do nothing
        break;
      case DELAY:
        if (current_ - trig_stop_ >= del_) {  // DELAY IS OVER
          offStage_ = WAIT;
          onStage_ = WAIT;
          low();
        }
        break;
      case GATE:
        // not possible
        break;
    }
  }

  inline bool State() {
    return output_.On();
  }
  inline int GetDelay() {
    return del_;
  }
  inline float GetDelayF() {
    return (float)del_ / (float)maxDelay;
  }
  inline void IncDelay() {
    del_ = constrain(del_ + incDelay, 0, maxDelay);
  }
  inline void DecDelay() {
    del_ = constrain(del_ - incDelay, 0, maxDelay);
  }
  inline void IncDelay2() {
    del_ = constrain(del_ + incDelay2, 0, maxDelay);
  }
  inline void DecDelay2() {
    del_ = constrain(del_ - incDelay2, 0, maxDelay);
  }
  inline void SetDelay(int d) {
    del_ = constrain(d, 0, maxDelay);
  }
  inline Clock GetClock() {
    return clock_;
  }
  inline String DisplayClock() {
    switch (clock_) {
      case CLOCK_A:
        return F("a");
      case CLOCK_B:
        return F("b");
      case CLOCK_BOTH:
        return F("*");
    }
  }
  inline void SetClock(Clock c) {
    clock_ = c;
  }
  inline void IncClock() {
    switch (clock_) {
      case CLOCK_A:
        clock_ = CLOCK_B;
        break;
      case CLOCK_B:
        clock_ = CLOCK_BOTH;
        break;
      case CLOCK_BOTH:
        clock_ = CLOCK_A;
        break;
    }
  }
  inline void DecClock() {
    switch (clock_) {
      case CLOCK_A:
        clock_ = CLOCK_BOTH;
        break;
      case CLOCK_B:
        clock_ = CLOCK_A;
        break;
      case CLOCK_BOTH:
        clock_ = CLOCK_B;
        break;
    }
  }


private:
  DigitalOutput output_;
  Clock clock_;
  DelayStage onStage_;
  DelayStage offStage_;
  unsigned long trig_start_;
  unsigned long trig_stop_;
  int del_;
  unsigned long current_;

  inline void high() {
    output_.High();
  }

  inline void low() {
    output_.Low();
  }
};

#endif
