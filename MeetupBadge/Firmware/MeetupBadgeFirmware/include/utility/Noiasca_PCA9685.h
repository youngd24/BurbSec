/*
  Noiasca Tool Kit - Outputs for LEDs and Relais - PCA9685 - PWM Servo Driver
  
  the hardware pins are abstracted from the styles.
  wrapper make the usage easier
  
  copyright 2022 noiasca noiasca@yahoo.com
  
  Version
  2022-12-19      added multi effect
  2022-12-17      initial version (based on 2022-02-15 neopixel)
*/

#pragma once
#include <Adafruit_PWMServoDriver.h>

/*
   class to encapsulate a LED on a PCA9685 into object
   and to offer a unified interface
*/
class PCA9685 {
    Adafruit_PWMServoDriver &pwm;
    const uint16_t startPixel;          // one output on PCA9685

  public:
    PCA9685(Adafruit_PWMServoDriver &pwm, uint16_t startPixel) : pwm(pwm), startPixel(startPixel) {}

    void begin(){} // no need, the pwm needs one begin only.
    

    void digWrite(uint8_t val) {
      if (val == LOW) 
        pwm.setPWM(startPixel, 0, 4096);
      else
        pwm.setPWM(startPixel, 4096, 0);
    }
    
    void digWrite(uint8_t pixel, uint8_t val) {
      if (val == LOW) 
        pwm.setPWM(startPixel + pixel, 0, 4096);
      else
        pwm.setPWM(startPixel + pixel, 4096, 0);
    }

    int digRead() {
      return LOW;  // MISSING implementation
    }    
/*
    
    @todo rework according https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
*/
    void pwmWrite(uint16_t value) {
      pwm.setPWM(startPixel, 0, value * 16);
    }
};

/*
   class to encapsulate several LEDs on a PCA9685 into one object
   and to offer a unified interface
*/
template<size_t noOfPixel>
class PCA9685Group {
    Adafruit_PWMServoDriver &pwm;
    uint16_t pixel[noOfPixel];         // pixel index on pwm

  public:
    PCA9685Group(Adafruit_PWMServoDriver &pwm, uint16_t pixelA, uint16_t pixelB, uint16_t pixelC) : pwm(pwm), pixel{pixelA, pixelB, pixelC} {}

    void begin() {}   // no need, the pwm needs one begin only.
    
    void digWrite(uint8_t newState) {
      for (size_t i = 0; i < noOfPixel; i++)
        digWrite(pixel[i], newState);
    }
    
    void digWrite(size_t actual, uint8_t newState) {
      if (newState == LOW) 
        pwm.setPWM(pixel[actual], 0, 4096); 
      else 
        pwm.setPWM(pixel[actual], 4096, 0);
    }

    // a read for group of pixels doesn't make sense
    //int digRead(){}    
/*
    
    @todo rework according https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
*/
    void pwmWrite(uint8_t value) {
      for (size_t i = 0; i < noOfPixel; i++)
        pwmWrite(pixel[i], value);        
    }

    void pwmWrite(size_t actual, uint8_t value) {
      pwm.setPWM(pixel[actual], 0, value * 16);
    } 
};

/* **************************************************************
  wrapper make to make the interface more 
  userfriendly
  ************************************************************** */

/**
   \brief alternate blinking of two PCA9685 outputs.
   
   wrapper to alternate two PCA9685 outputs.
   Inherits "style" class and composites PCA9685
*/
class AlternatingPCA9685 : public Alternating<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your PCA9685 object
   @param pixel the first of two pixels on the PCA9685 to be used (also the next pixel will be used!)
**/
    AlternatingPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Alternating(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief blink a LED on a PCA9685 output.
   
   wrapper to blink a LED on a PCA9685 output.
   Inherits "style" class and composites PCA9685
*/
class BlinkPCA9685 : public Blink<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixel the pixel on the pwm to be used 
**/
    BlinkPCA9685(Adafruit_PWMServoDriver &pwm, byte pixel) : Blink(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief bounce 5 LEDs on PCA9685 between left and right 
   
   wrapper to bounce a series of 5 PCA9685 outputs.
   Inherits "style" class and composites PCA9685
   @note the 5 pca9685 must be in sequence
   (could be changed one day using PCA9685Group<5>)
*/
class Bounce5PCA9685 : public Bounce5<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your PCA9685 output
   @param pixel the first of 5 outputs on the PCA9685 to be used 
**/
    Bounce5PCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Bounce5(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief enable several effects on a PCA9685 output
   
   wrapper to do several effects on a PCA9685 output. 
   Inherits "style" class and composites PCA9685.
*/
class EffectPCA9685 : public Effect<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   \param pwm a reference to an IC object
   \param pixel the pin on the IC
**/
    EffectPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Effect(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief simulate a flickering light like a fire on a PCA9685
   
   wrapper to flicker an output on a PCA9685 output.
   Inherits "style" class and composites PCA9685
*/
class FlickerPCA9685 : public Flicker<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your PCA9685 output
   @param pixel the output on the PCA9685 to be used 
**/
    FlickerPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Flicker(pca9685), pca9685(pwm, pixel) {};
};

/**
    \brief Imitiate fluorescent lamp with a PCA9685 output
    
   wrapper for imitating Fluorescent on a PCA9685 output.
   Inherits "style" class and composites PCA9685
*/
class FluorescentPCA9685 : public Fluorescent<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your PCA9685 object
   @param pixel the output on the PCA9685 to be used 
**/
    FluorescentPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Fluorescent(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief heart beat a PCA9685.
   
   wrapper to heartbeat an output on a PCA9685.
   Inherits class heartbeat and composites PCA9685
*/
class HeartbeatPCA9685 : public Heartbeat<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixel the pixel on the pwm to be used 
**/
    HeartbeatPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Heartbeat(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief a on/off one output on PCA9685
   
   wrapper for an simple on/off on a PCA9685.
   This class provides a simple on() off() interface for a PCA9685.
   There is no effect during runtime.
   Inherits class pulse and composites PCA9685
*/
class OnOffPCA9685 : public OnOff<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixel the pixel on the pwm to be used 
**/
    OnOffPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : OnOff(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief pulse a PCA9685 output (monoflop)
   
   wrapper to pulse a PCA9685 output.
   Inherits class pulse and composites PCA9685
*/
class PulsePCA9685 : public Pulse<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixel the pixel on the pwm to be used 
**/
    PulsePCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Pulse(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief show a specific rhythm on a PCA9685 output.
   
   wrapper to rhythm blink a PCA9685 output.
   Inherits "style" class and composites PCA9685.
*/
class RhythmPCA9685 : public Rhythm<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixel the pixel on the pwm to be used 
**/
    RhythmPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Rhythm(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief Switch on/off a PCA9685 output smoothly
   
   wrapper to smooth PCA9685 output.
   Inherits "style" class and composites PCA9685.
*/
class SmoothPCA9685 : public Smooth<PCA9685> {
    PCA9685 pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixel the pixel on the pwm to be used 
**/
    SmoothPCA9685(Adafruit_PWMServoDriver &pwm, uint16_t pixel) : Smooth(pca9685), pca9685(pwm, pixel) {};
};

/**
   \brief a Traffic light with 3 PCA9685 outputs
   
   wrapper for a traffic light with 3 PCA9685 outputs.
   Inherits "style" class and composites PCA9685.
   @note don't forget to set colors for the three pca9685s (e.g. red, yellow, green).
*/
class TrafficlightPCA9685 : public Trafficlight<PCA9685Group<3>> {
    PCA9685Group<3> pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixelA the pixel on the pwm to be used for red
   @param pixelB the pixel on the pwm to be used for yellow
   @param pixelC the pixel on the pwm to be used for green
**/
    TrafficlightPCA9685(Adafruit_PWMServoDriver &pwm, byte pixelA, byte pixelB, byte pixelC) : Trafficlight(pca9685), pca9685(pwm, pixelA, pixelB, pixelC) {};
};

/**
   \brief turnsignals for a car with 3 PCA9685 outputs.
   
   wrapper for a turning signal with 3 PCA9685 outputs.
   Inherits "style" class and composites PCA9685.
   @note don't forget to set colors for the three pca9685s (e.g. orange, orange, red).
*/
class TurnsignalPCA9685 : public Turnsignal<PCA9685Group<3>> {
    PCA9685Group<3> pca9685;
  public:
/**
   @param pwm a reference to your pwm object
   @param pixelA the pixel on the pwm to be used for left
   @param pixelB the pixel on the pwm to be used for right
   @param pixelC the pixel on the pwm to be used for hazard light
**/
    TurnsignalPCA9685(Adafruit_PWMServoDriver &pwm, byte pixelA, byte pixelB, byte pixelC) : Turnsignal(pca9685), pca9685(pwm, pixelA, pixelB, pixelC) {};
};
