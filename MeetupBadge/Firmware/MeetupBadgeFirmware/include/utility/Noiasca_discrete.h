/*
  Noiasca Tool Kit - Outputs for LEDs and Relais
  
  Discrete Pins on an Arduino or other Micrcocontroller
  
  The hardware pins are abstracted from the styles.
  wrapper make the usage easier for the use
   
  copyright 2022 noiasca noiasca@yahoo.com
  
  Version
  2022-12-19       added multi effect
  2022-12-19       extract discrete pins in separate file
*/

#pragma once

/*  **************************************************
    Hardware classes 
    to unify the access to outputs    
    ************************************************** */ 

/*
   class to encapsulate a discrete Arduino pin into a object
   and to offer a unified interface.
*/

class DiscretePin {
    const uint8_t pin;                 // the pin on the Arduino
    const bool active;                 // is the pin active HIGH
  public:
#ifdef ARDUINO_ARCH_ESP32
    static byte nextChannel;           // keep track of total number of pins
    const byte ledChannel;             // will be set to channel
 
    DiscretePin(uint8_t pin, bool active = HIGH) : 
      pin(pin), active(active), ledChannel(nextChannel++) {}
#else
    DiscretePin(uint8_t pin, bool active = HIGH) : 
      pin(pin), active(active) {}
#endif  
    
/**
   \brief start hardware
   
   Will do the necessary steps to initialize the hardware pins.
   Call this function in your setup().
*/
    void begin()
    {
      pinMode(pin, OUTPUT);
      //set pin to LOW
#ifdef ARDUINO_ARCH_ESP32
      //substitude "analogWrite" on ESP32
      if (ledChannel < 16)
      {
        //        ledChannel, freq, resolution
        ledcSetup(ledChannel, 5000, 8);
        ledcAttachPin(pin, ledChannel);
      }
#endif
    }
    
    void digWrite(uint8_t val)
    {
      if (val == LOW) digitalWrite(pin, !active); else digitalWrite(pin, active);
    }
    
    // int because int ditalRead(uint8_t)
    int digRead()
    {
      return digitalRead(pin);  
    }
    
    // takes int like analogWrite
    void pwmWrite(int pwm)    
    {
#ifdef ARDUINO_ARCH_ESP32
      //substitude "analogWrite" on ESP32
      if (ledChannel < 16)
      {
        //        ledChannel, dutycycle
        ledcWrite(ledChannel, pwm);
      }
#else
      analogWrite(pin, pwm);  // analogWrite(uint8_t, int)
#endif
    }
    
    // empty implementation - discrete pin has no color
    void setOnColor(uint32_t _onColor)
    {
      (void)_onColor;
    }
    
    // empty implementation - discrete pin has no color
    void setOffColor(uint32_t _offColor)
    {
      (void)_offColor;
    } 
};

#ifdef ARDUINO_ARCH_ESP32
byte DiscretePin::nextChannel = 0;  // initialize outside of class
#endif

/*
   class to encapsulate a group of discrete Arduino pins into a object
   and to offer a unified interface
   Useable for 2, 3 or 5 pins. Other constructors are not implemented.
*/
template<size_t noOfPins>
class DiscreteGroup {
    const uint8_t pin[noOfPins];
  public:
    DiscreteGroup(uint8_t pinA, uint8_t pinB) : 
      pin{pinA, pinB} {}
  
    DiscreteGroup(uint8_t pinA, uint8_t pinB, uint8_t pinC) : 
      pin{pinA, pinB, pinC} {}
  
    DiscreteGroup(uint8_t pinA, uint8_t pinB, uint8_t pinC, uint8_t pinD, uint8_t pinE) : 
      pin{pinA, pinB, pinC, pinD, pinE} {}
    
/**
   \brief start hardware
   
   Will do the necessary steps to initialize the hardware pins.
   Call this function in your setup().
*/
    void begin()
    {
      for(auto &i : pin)
      {
        pinMode(i, OUTPUT);
        digWrite(LOW);
      }
    }
    
    void digWrite(uint8_t val)
    {
      digWrite(0, val);
    }
    
    void digWrite(uint8_t i, uint8_t val)
    {
      if (i < noOfPins && pin[i] < 255)
      {
       if (val == LOW) digitalWrite(pin[i], LOW); else digitalWrite(pin[i], HIGH);
      }
    }
    
    int digRead(uint8_t i = 0)
    {
      if (i < noOfPins && pin[i] < 255)
        return digitalRead(pin[i]);
      else
        return LOW;
    }
    
    void pwmWrite(uint8_t pwm)
    {
      pwmWrite(0, pwm);
    }
    
    void pwmWrite(uint8_t i, uint8_t pwm)
    {
#ifdef ARDUINO_ARCH_ESP32
      // there is no substitude analogWrite for group of pins on a ESP32
      (void)i;
      (void)pwm;
#else
     if (i < noOfPins) analogWrite(pin[i], pwm);
#endif
    }
    
    // empty implementation
    void setOnColor(uint32_t _onColor)
    {
      (void)_onColor;
    }
    
    // empty implementation
    void setOffColor(uint32_t _offColor)
    {
      (void)_offColor;
    } 
};



/* **************************************************************
  wrapper make to make the interface more 
  userfriendly
  ************************************************************** */

/**
   \brief alternate blinking of two pins.
   
   wrapper to alternate blink two discrete Arduino pins. 
   Inherits "style" class and composites DiscretePin
*/
class AlternatingPin : public Alternating<DiscreteGroup<2>> {
    DiscreteGroup<2> discreteGroup;
  public:
/**
   \param pinA a discrete pin
   \param pinB a discrete pin
**/
    AlternatingPin(byte pinA, byte pinB) : Alternating(discreteGroup), discreteGroup(pinA, pinB) {};
};

/**
   \brief blink a discrete pin.
   
   wrapper to blink a discrete Arduino pin. 
   Inherits "style" class and composites DiscretePin
*/
class BlinkPin : public Blink<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin
**/
    BlinkPin(byte pin) : Blink(discretePin), discretePin(pin) {};
};

/**
   \brief bounce 5 LEDs between left and right 
   
   wrapper to bounce 5 a discrete Arduino pins in the stile of KITT/Larson scanner. 
   Inherits "style" class and composites DiscretePin
*/
class Bounce5Pin : public Bounce5<DiscreteGroup<5>> {
    DiscreteGroup<5> discreteGroup;
  public:
/**
   \param pinA first of 5 discrete pin
   \param pinB second of 5 discrete pin
   \param pinC third of 5 discrete pin
   \param pinD fourth of 5 discrete pin
   \param pinE fifth of 5 discrete pin
**/
    Bounce5Pin(byte pinA, byte pinB, byte pinC, byte pinD, byte pinE) : Bounce5(discreteGroup), discreteGroup(pinA, pinB, pinC, pinD, pinE) {};
};

/**
   \brief enable several effects on a discrete pin
   
   wrapper for severak effects on a discrete Arduino pin. 
   Inherits "style" class and composites DiscretePin
*/
class EffectPin : public Effect<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin
   \param active if the pin should be LOW active, set parameter to LOW. default is HIGH.
**/
    EffectPin(byte pin, uint8_t active = HIGH) : Effect(discretePin), discretePin(pin, active) {};
};

/**
   \brief a heart beat LED on a discrete pin
   
   wrapper to heartbeat a discrete pin. 
   Inherits from style class and composites DiscretePin
*/
class HeartbeatPin : public Heartbeat<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin (PWM). UNO/NANO PWM pins are 3, 5, 6, 9, 10, 11 
**/
    HeartbeatPin(byte pin) : Heartbeat(discretePin), discretePin(pin) {};
};

/**
   \brief Simulate a flickering light like a fire 
   
   wrapper to Flicker a discrete pin. 
   Inherits "style" class and composites DiscretePin
*/
class FlickerPin : public Flicker<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin (PWM). UNO/NANO PWM pins are 3, 5, 6, 9, 10, 11 
**/
    FlickerPin(byte pin) : Flicker(discretePin), discretePin(pin) {};
};

/**
   \brief simulate a fluorescent lamp
   
   wrapper to imitate Fluorescent on a discrete pin. 
   Inherits "style" class and composites DiscretePin
*/
class FluorescentPin : public Fluorescent<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin (PMW). UNO/NANO PWM pins are 3, 5, 6, 9, 10, 11 
**/
    FluorescentPin(byte pin) : Fluorescent(discretePin), discretePin(pin) {};
};

/**
   \brief a on/off Arduino pin
   
   wrapper for a simple on/off output on a discrete Arduino pin.
   This class provides a simple on/off interface for a pin.
   There is no effect during runtime.
   The class just provides access to a discrete pin with an unified interface.
   Inherits from style class and composites DiscretePin.
*/
class OnOffPin : public OnOff<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin
   \param active if the pin should be LOW active, set parameter to LOW. default is HIGH.
**/
    OnOffPin(byte pin, uint8_t active = HIGH) : OnOff(discretePin), discretePin(pin, active) {};
};

/**
   \brief pulse a discrete Arduino pin (monoflop)
   
   wrapper to pulse a discrete Arduino pin.
   Inherits from style class and composites DiscretePin.
*/
class PulsePin : public Pulse<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin
   \param active if the pin should be LOW active, set parameter to LOW. default is HIGH.
**/
    PulsePin(byte pin, uint8_t active = HIGH) : Pulse(discretePin), discretePin(pin, active) {};
};

/**
   \brief show a specific rhythm on a LED.
   
   wrapper to rhythm blink a discrete Arduino pin. 
   Inherits style class and composites DiscretePin
*/
class RhythmPin : public Rhythm<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin
**/
    RhythmPin(byte pin) : Rhythm(discretePin), discretePin(pin) {};
};

/**
   \brief switch on a LED smoothly
   
   wrapper to switch on a discrete Arduino pin smoothly. 
   Inherits style class and composites DiscretePin
*/
class SmoothPin : public Smooth<DiscretePin> {
    DiscretePin discretePin;
  public:
/**
   \param pin a discrete pin (PWM). UNO/NANO PWM pins are 3, 5, 6, 9, 10, 11 
**/
    SmoothPin(byte pin) : Smooth(discretePin), discretePin(pin) {};
};

/**
   \brief a traffic light with 3 LEDs.
   
   wrapper for a turning signal with 3 discrete pins.
   Inherits "style" class and composites DiscreteGroup.
*/
class TrafficlightPin : public Trafficlight<DiscreteGroup<3>> {
    DiscreteGroup<3> discreteGroup;
  public:
/**
   \param pinA a discrete pin for red
   \param pinB a discrete pin for yellow - set to 255 if not used
   \param pinC a discrete pin for green
**/
    TrafficlightPin(byte pinA, byte pinB, byte pinC) : Trafficlight(discreteGroup), discreteGroup(pinA, pinB, pinC) {};
};

/**
   \brief turnsignals for a car with 3 LEDs.
   
   wrapper for a turning signal with 3 discrete pins.
   Inherits "style" class and composites DiscreteGroup.
*/
class TurnsignalPin : public Turnsignal<DiscreteGroup<3>> {
    DiscreteGroup<3> discreteGroup;
  public:
/**
   \param pinA a discrete pin for left turn signal
   \param pinB a discrete pin for right turn signal
   \param pinC a discrete pin for a hazard light - set to 255 if not used
**/
    TurnsignalPin(byte pinA, byte pinB, byte pinC) : Turnsignal(discreteGroup), discreteGroup(pinA, pinB, pinC) {};
};

/*! Some words to the Noiasca Tool Kit
 
  \section led2_sec Pins to control LEDs and Relais
  This classes can be used to blink, pulse, heartbeat, LEDs in different pattern.
 */
 