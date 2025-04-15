//#include <cstdint>
#pragma once

using namespace modulove;

byte divisions[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 24, 32, 48, 64, 96 };

const byte MaxDiv = 18;
const byte inc = 1;

enum Clock {
  A,
  B,
  BOTH,  // == *
};

enum Mode {
  // Follow the state of the input clock.
  TRIGGER,
  // Toggle between on/off with each clock input rising edge.
  FLIP,
  // clock division from clock input 1/2/3/4/5/6/7/8/9/10/11/12/13/16/24/32/48/64/128 (reset input?)
  CLOCK_DIV,
  // clock division flip from clock input 1/2/3/4/5/6/7/8/9/10/11/12/13/16/24/32/48/64/128 (reset input?)
  CLOCK_DIV_FLIP,
  // copy previous output (not possible on chan1) // IN PREV MODES WE DON'T CARE ABOUT CLOCK A/B
  PREV_COPY,
  //  invert previous output (if previous output high > low...) == coin toss == bernoulli gate (not possible on chan1)
  PREV_BERNOULLI,
  //  invert previous output and flip (if previous output high > low...) == coin toss == bernoulli gate (not possible on chan1)
  PREV_BERNOULLI_FLIP,
  // decreasing probability with previous output (not possible on chan1)
  PREV_FOLLOW,
  // decreasing probability with previous output (not possible on chan1)
  PREV_FOLLOW_FLIP,
  // clock division from previous output 1/2/3/4/5/6/7/8/9/10/11/12/13/16/24/32/48/64/128
  PREV_CLOCK_DIV,
  // clock division flip from previous output 1/2/3/4/5/6/7/8/9/10/11/12/13/16/24/32/48/64/128
  PREV_CLOCK_DIV_FLIP,
  // OFF
  OFF,
};

const byte MaxRandRange = 100;

/**
  Class for handling probablistic triggers.
*/
class ProbablisticOutput {
public:
  ProbablisticOutput() {}
  ~ProbablisticOutput() {}

  /**
    Initializes the probablistic cv output object with a given digital cv and
    led pin pair.
      \param cv_pin gpio pin for the cv output.
      \param led_pin gpio pin for the LED.
      \param probability percentage chance to trigger as a float from 0 to 1.
    */
  void Init(DigitalOutput output, byte probability) {
    Init(output, probability, TRIGGER, Clock::A, 0);
  }

  /**
    Initializes the probablistic cv output object with a given digital cv and
    led pin pair.
      \param cv_pin gpio pin for the cv output.
      \param led_pin gpio pin for the LED.
      \param probability percentage chance to trigger as a float from 0 to 1.
      \param mode defines the behavior of the rising / falling clock input on this output.
    */
  void Init(DigitalOutput output, byte probability, byte mode, byte clock, byte div) {
    output_ = output;
    SetProb(probability);
    SetMode((Mode)mode);
    SetClock(clock);
    Reset();
    div_ = div;
  }

  // Turn the CV and LED High according to the probability value.
  inline bool On(Clock c, bool prev) {
    if (mode_ == OFF) {
      low(c);
      return prev;
    }
    if (clock_ != BOTH and clock_ != c)
      return State();

    switch (mode_) {
      case TRIGGER:
        // Coin toss trigger
        if (random(0, MaxRandRange) > prob_)
          return State();
        high(c);
        return State();
      case FLIP:
        // Coin toss flip
        if (random(0, MaxRandRange) > prob_)
          return State();
        flip(c);
        return State();
      case CLOCK_DIV:
        // Clock division trigger
        count_ = (count_ + 1) % divisions[div_];
        if (count_ == 0) { high(c); }
        return State();
      case CLOCK_DIV_FLIP:
        // Clock division flip
        count_ = (count_ + 1) % divisions[div_];
        if (count_ == 0) { flip(c); }
        return State();
      case PREV_COPY:
        // IF previous is HIGH we copy
        if (prev == HIGH)
          high(c);
        else
          low(c);
        return State();
      case PREV_BERNOULLI:
      case PREV_BERNOULLI_FLIP:
        // IF previous is HIGH we invert
        if (prev == HIGH)
          low(c);
        else
          high(c);
        return State();
      case PREV_FOLLOW:
        if (prev == LOW) { return State(); }
        // IF previous is HIGH we try a coin toss
        if (random(0, MaxRandRange) > prob_)
          return State();
        high(c);
        return State();
      case PREV_FOLLOW_FLIP:
        if (prev == LOW) { return State(); }
        // IF previous is HIGH we try a coin toss
        if (random(0, MaxRandRange) > prob_)
          return State();
        flip(c);
        return State();
      case PREV_CLOCK_DIV:
        if (prev == LOW) { return State(); }
        // IF previous is HIGH we divide
        count_ = (count_ + 1) % divisions[div_];
        if (count_ == 0) { high(c); }
        return State();
      case PREV_CLOCK_DIV_FLIP:
        if (prev == LOW) { return State(); }
        // IF previous is HIGH we divide
        count_ = (count_ + 1) % divisions[div_];
        if (count_ == 0) { flip(c); }
        return State();
    }
  }

  // Turn off CV and LED.
  inline bool Off(Clock c, bool prev) {
    if (mode_ == OFF) {  // If off, we pass the previous value to the next output
      low(c);
      return prev;
    }
    if (clock_ != BOTH and clock_ != c)  // If it's not our clock or clock is not BOTH
      return State();

    if (clock_ == BOTH && prevClock_ != c)  // If both but state previously changed by other clock
      return State();

    switch (mode_) {
      case TRIGGER:
      case CLOCK_DIV:
      case PREV_FOLLOW:
      case PREV_CLOCK_DIV:
      case PREV_BERNOULLI:
        low(c);
        return State();
      case FLIP:
      case CLOCK_DIV_FLIP:
      case PREV_FOLLOW_FLIP:
      case PREV_CLOCK_DIV_FLIP:
        return State();
      case PREV_COPY:
        if (prev == HIGH)
          high(c);
        else
          low(c);
        return State();
      case PREV_BERNOULLI_FLIP:
        if (prev == HIGH)
          low(c);
        else
          high(c);
        return State();
    }
  }

  inline bool State() {
    return output_.On();
  }

  inline char DisplayClock() {
    if (mode_ == OFF)
      return ' ';
    switch (clock_) {
      case A:
        return 'a';
      case B:
        return 'b';
      case BOTH:
        return '*';
    }
  }
  inline byte GetClock() {
    switch (clock_) {
      case A:
        return 0;
      case B:
        return 1;
      case BOTH:
        return 2;
    }
  }
  inline void SetClock(byte c) {
    switch (c) {
      case 0:
        clock_ = A;
        break;
      case 1:
        clock_ = B;
        break;
      case 2:
        clock_ = BOTH;
        break;
    }
  }
  /*inline void SetClock(Clock c) {
    clock_ = c;
  }*/

  inline String DisplayMode() {
    switch (mode_) {
      case TRIGGER:
        return F("Trigger");
      case FLIP:
        return F("Flip");
      case CLOCK_DIV:
        return F("ClkDiv");
      case CLOCK_DIV_FLIP:
        return F("ClkDiv (f)");
      case PREV_COPY:
        return F("Copy (p)");
      case PREV_BERNOULLI:
        return F("Bernoul (p)");
      case PREV_BERNOULLI_FLIP:
        return F("Bernoul (f/p)");
      case PREV_FOLLOW:
        return F("Follow (p)");
      case PREV_FOLLOW_FLIP:
        return F("Follow (f/p)");
      case PREV_CLOCK_DIV:
        return F("ClkDiv (p)");
      case PREV_CLOCK_DIV_FLIP:
        return F("ClkDiv (f/p)");
      case OFF:
        return F("Off");
    }
  }
  inline String DisplayModeShort() {
    switch (mode_) {
      case TRIGGER:
        return F("T");
      case FLIP:
        return F("F");
      case CLOCK_DIV:
        return F("/");
      case CLOCK_DIV_FLIP:
        return F("/f");
      case PREV_COPY:
        return F("=");
      case PREV_BERNOULLI:
        return F("B");
      case PREV_BERNOULLI_FLIP:
        return F("Bf");
      case PREV_FOLLOW:
        return F(">");
      case PREV_FOLLOW_FLIP:
        return F(">f");
      case PREV_CLOCK_DIV:
        return F(">/");
      case PREV_CLOCK_DIV_FLIP:
        return F(">/f");
      case OFF:
        return F("0");
    }
  }
  inline byte GetMode() {
    switch (mode_) {
      case TRIGGER:
        return 0;
      case FLIP:
        return 1;
      case CLOCK_DIV:
        return 2;
      case CLOCK_DIV_FLIP:
        return 3;
      case PREV_COPY:
        return 4;
      case PREV_BERNOULLI:
        return 5;
      case PREV_BERNOULLI_FLIP:
        return 6;
      case PREV_FOLLOW:
        return 7;
      case PREV_FOLLOW_FLIP:
        return 8;
      case PREV_CLOCK_DIV:
        return 9;
      case PREV_CLOCK_DIV_FLIP:
        return 10;
      case OFF:
        return 11;
    }
  }
  inline void SetMode(byte m) {
    switch (m) {
      case 0:
        mode_ = TRIGGER;
        break;
      case 1:
        mode_ = FLIP;
        break;
      case 2:
        mode_ = CLOCK_DIV;
        break;
      case 3:
        mode_ = CLOCK_DIV_FLIP;
        break;
      case 4:
        mode_ = PREV_COPY;
        break;
      case 5:
        mode_ = PREV_BERNOULLI;
        break;
      case 6:
        mode_ = PREV_BERNOULLI_FLIP;
        break;
      case 7:
        mode_ = PREV_FOLLOW;
        break;
      case 8:
        mode_ = PREV_FOLLOW_FLIP;
        break;
      case 9:
        mode_ = PREV_CLOCK_DIV;
        break;
      case 10:
        mode_ = PREV_CLOCK_DIV_FLIP;
        break;
      case 11:
        mode_ = OFF;
        break;
    }
  }
  inline void Reset() {
    count_ = 0;
  }
  inline byte DisplayDivision() {
    return divisions[div_];
  }
  inline byte GetDivision() {
    return div_;
  }
  inline float GetProb() {
    if (mode_ == CLOCK_DIV || mode_ == CLOCK_DIV_FLIP || mode_ == PREV_CLOCK_DIV || mode_ == PREV_CLOCK_DIV_FLIP)
      return divisions[div_];
    return float(prob_) / float(MaxRandRange);
  }
  inline byte GetProbInt() {
    return prob_;
  }
  inline void IncProb() {
    if (mode_ == CLOCK_DIV || mode_ == CLOCK_DIV_FLIP || mode_ == PREV_CLOCK_DIV || mode_ == PREV_CLOCK_DIV_FLIP) {
      div_ = constrain(div_ + 1, 0, MaxDiv);
      return;
    }
    prob_ = constrain(prob_ + 1, 0, MaxRandRange);
  }
  inline void DecProb() {
    if (mode_ == CLOCK_DIV || mode_ == CLOCK_DIV_FLIP || mode_ == PREV_CLOCK_DIV || mode_ == PREV_CLOCK_DIV_FLIP) {
      div_ = constrain(div_ - 1, 0, MaxDiv);
      return;
    }
    prob_ = constrain(prob_ - 1, 0, MaxRandRange);
  }
  inline void SetProb(byte probability) {
    prob_ = constrain(probability, 0, MaxRandRange);
  }

private:
  DigitalOutput output_;
  byte prob_;
  Mode mode_;
  byte clock_;      // a / b / * (both)
  byte div_;        // clock division in the clock division array
  byte count_;      // count for clock division
  bool prevClock_;  // store which clock changed state

  inline void flip(Clock c) {
    prevClock_ = c;
    output_.Update(!output_.On());
  }

  inline void high(Clock c) {
    prevClock_ = c;
    output_.High();
  }

  inline void low(Clock c) {
    prevClock_ = c;
    output_.Low();
  }
};
