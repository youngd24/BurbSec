/*
  Noiasca Tool Kit - Outputs for LEDs and Relais
  
  PCF8547 I2C Port Expander
  
  The hardware interface is abstracted from the styles.
  wrapper make the usage easier
  
  copyright 2022 noiasca noiasca@yahoo.com
  
  Version
  2022-12-19       added multi effect
  2022-04-16       initial version
*/

#pragma once

/**  
    \brief PCF8574 Expander Hardware class
    
    Takes Wire and I2C address and administrates the port. 
    This connects the low level hardware. 
    The class is needed becauses each pin on the PCF8574 should be treated seperately
    but the IC accepts only one byte for all 8 hardware pins    
 */ 
class PCF8574expander 
{ 
  protected:
    TwoWire *i2cPort;				           // generic connection to user's chosen I2C hardware
    const uint8_t i2cAddr;             // I2C address of expander
    uint8_t pinStatus = 0;             // stores io state of expander pins to make them available in parallel instances

  public:
/**
   @param i2cAddr the I2C address (0x20 - 0x27 or 0x38 - 0x3F)
**/
    PCF8574expander(uint8_t i2cAddr) : i2cPort(&Wire), i2cAddr(i2cAddr) {}  // default Wire library
    
/**
   @param i2cAddr the I2C address (0x20 - 0x27 or 0x38 - 0x3F)
   @param i2cPort use Wire or any other TwoWire (I2C) interface you have available on your microcontroller
**/
    PCF8574expander(TwoWire &i2cPort, uint8_t i2cAddr) : i2cPort(&i2cPort), i2cAddr(i2cAddr) {}

/**
   \brief write to a pin
   
   set or unset an output pin.
   @param pin the pin to write
   @param val the value LOW or HIGH
*/
    void digitalWrite(uint8_t pin, uint8_t val)
    {
      if (val == LOW)
        bitClear(pinStatus, pin);
      else 
        bitSet(pinStatus, pin);
      i2cPort->beginTransmission(i2cAddr);
      i2cPort->write(pinStatus);
      i2cPort->endTransmission();
    }
    
/**
   \brief read pin status
   
   @return the value of the internal pin status
*/
    uint8_t getPinStatus()
    {
      return pinStatus;
    }

/**
   \brief set pin status
   
   This function only sets the internal variable - doesn't change pins
   
   @param _pinStatus the new value for the pin status
*/    
    void setPinStatus(uint8_t _pinStatus)
    {
      pinStatus = _pinStatus;
    }
};


/*  **************************************************
    Interface classes between Hardware and Effects
    to unify the access to outputs    
    ************************************************** */ 
    
/*
   interface class to encapsulate a pin on a PCF8574 into object
   and to offer a unified interface for effects
*/
class PCF8574_IF 
{ 
  protected:
    PCF8574expander &pcf8574;				   // generic connection to user's chosen I2C hardware
    const uint16_t startPixel;         // one pin of the expander

  public:
    PCF8574_IF(PCF8574expander &pcf8574, uint16_t startPixel) : pcf8574(pcf8574), startPixel(startPixel) {}
    
    void begin() // the PCF8574 needs no begin
    {}

    void digWrite(uint8_t val)
    {
      digWrite(0, val);
    }
    
    void digWrite(uint8_t pixel, uint8_t val)
    {
      pcf8574.digitalWrite(startPixel + pixel, val);
    }

    int digRead()
    {
      return LOW;                      // MISSING: tbd
    }    

    void pwmWrite(int pwm)
    {
      if (pwm > 127) 
        digWrite(HIGH);
      else
        digWrite(LOW);
    }

    void setOnColor(uint32_t _onColor)
    {
      (void)_onColor;
    }
   
    void setOffColor(uint32_t _offColor)
    {
      (void)_offColor;
    }
};

/*
   class to encapsulate several pins on a PCF8574expander into one object
   and to offer a unified interface
   Useable for 2, 3 or 5 pins. Other constructors are not implemented.
*/

template<size_t noOfPins>
class PCF8574_IFGroup {
    PCF8574expander &pcf8574;				   // generic connection to user's chosen I2C hardware
    const uint8_t pin[noOfPins];

  public:
    PCF8574_IFGroup(PCF8574expander &pcf8574, uint8_t pinA, uint8_t pinB) : 
      pcf8574(pcf8574), 
      pin{pinA, pinB} {}
      
    PCF8574_IFGroup(PCF8574expander &pcf8574, uint8_t pinA, uint8_t pinB, uint8_t pinC) : 
      pcf8574(pcf8574), 
      pin{pinA, pinB, pinC} {}  
      
    PCF8574_IFGroup(PCF8574expander &pcf8574, uint8_t pinA, uint8_t pinB, uint8_t pinC, uint8_t pinD, uint8_t pinE) : 
      pcf8574(pcf8574), 
      pin{pinA, pinB, pinC, pinD, pinE} {}  
    
    void begin() {}                    // no need for the PCF8547
    
    void digWrite(uint8_t newState)
    {
      digWrite(0, newState);
    }
    
    void digWrite(uint8_t actual, uint8_t val)
    {
      if (actual < noOfPins && pin[actual] < 255)
      {
       if (val == LOW) pcf8574.digitalWrite(pin[actual], LOW); else pcf8574.digitalWrite(pin[actual], HIGH);
      }
    }

    // a read for group of pixels doesn't make sense
    //int digRead(){}    

    void pwmWrite(uint8_t pwm)
    {
      pwmWrite(0, pwm);
    }

    void pwmWrite(size_t actual, uint8_t pwm)
    {
      if (pwm > 127)
        pcf8574.digitalWrite(pin[actual], HIGH);
      else
        pcf8574.digitalWrite(pin[actual], LOW);
    }

    void setOnColor(uint16_t actual, uint32_t _onColor)
    {
      (void)_onColor;
    }
   
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
   \brief alternate blinking of two PCF8547 pins
   
   wrapper to alternate two PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin.
*/
class AlternatingPCF8574 : public Alternating<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the first of two consecutive pixels on the expander to be used (also the next pixel will be used!)
**/
    AlternatingPCF8574(PCF8574expander &hardware, uint8_t pin) : Alternating(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief blink a PCF8547 pin.
   
   wrapper to blink a PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin.
*/
class BlinkPCF8574 : public Blink<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin of the expander to be used
**/
    BlinkPCF8574(PCF8574expander &hardware, uint8_t pin) : Blink(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief bounce 5 PCF8547 pin between left and right 
   
   wrapper to bounce a series of 5 PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin. 
   @note the 5 PCF8547 pins must be in sequence
   (could be changed one day using a PCF8574_IFGroup<5>)
*/
class Bounce5PCF8547 : public Bounce5<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the first of 5 pins on the expander to be used 
**/
    Bounce5PCF8547(PCF8574expander &hardware, uint8_t pin) : Bounce5(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief enable several effects on a PCF8547 output
   
   wrapper to do several effects on a PCF8547 output. 
   Inherits "style" class and composites PCF8547.
*/
class EffectPCF8547 : public Effect<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/  
    EffectPCF8547(PCF8574expander &hardware, uint8_t pin) : Effect(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief simulate a flickering light like a fire on a PCF8547 pin
   
   wrapper to flicker a PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin
   @note the PCF8547 is on/off only - no PWM. The effect will be limited.
*/
class FlickerPCF8547 : public Flicker<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/
    FlickerPCF8547(PCF8574expander &hardware, uint8_t pin) : Flicker(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
    \brief Imitiate fluorescent lamp with a PCF8547 pin
    
   wrapper for a Fluorescent imitating PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin.
   @note the PCF8547 is on/off only - no PWM. The effect will be limited.
*/
class FluorescentPCF8547 : public Fluorescent<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/
    FluorescentPCF8547(PCF8574expander &hardware, uint8_t pin) : Fluorescent(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief heart beat a PCF8547 pin.
   
   wrapper to heartbeat a PCF8547 pin.
   Inherits class heartbeat and composites PCF8547 pin
   @note the PCF8547 is on/off only - no PWM. The effect will be limited.
*/
class HeartbeatPCF8547 : public Heartbeat<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/
    HeartbeatPCF8547(PCF8574expander &hardware, uint8_t pin) : Heartbeat(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief a on/off one PCF8547 pin
   
   wrapper for an simple on/off on a PCF8547 pin.
   This class provides a simple on() off() interface for a PCF8547 pin.
   There is no effect during runtime.
   Inherits class pulse and composites PCF8547 pin
*/
class OnOffPCF8547 : public OnOff<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/
    OnOffPCF8547(PCF8574expander &hardware, uint8_t pin) : OnOff(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief pulse a PCF8547 pin (monoflop)
   
   wrapper to pulse a PCF8547 pin.
   Inherits class pulse and composites PCF8547 pin
*/
class PulsePCF8547 : public Pulse<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/
    PulsePCF8547(PCF8574expander &hardware, uint8_t pin) : Pulse(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief show a specific rhythm on a PCF8547 pin.
   
   wrapper to rhythm blink a PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin.
*/
class RhythmPCF8547 : public Rhythm<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/
    RhythmPCF8547(PCF8574expander &hardware, uint8_t pin) : Rhythm(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief Switch on/off a PCF8547 pin smoothly
   
   wrapper to smooth PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin.
   @note the PCF8547 is on/off only - no PWM. The effect will be limited.
*/
class SmoothPCF8547 : public Smooth<PCF8574_IF> {
    PCF8574_IF hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pin the pin on the expander to be used 
**/
    SmoothPCF8547(PCF8574expander &hardware, uint8_t pin) : Smooth(hardware_IF), hardware_IF(hardware, pin) {};
};

/**
   \brief a traffic light with 3 PCF8547 pin
   
   wrapper for a traffic light with 3 PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin.
*/
class TrafficlightPCF8547 : public Trafficlight<PCF8574_IFGroup<3>> {
    PCF8574_IFGroup<3> hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pinA the pin on the expander to be used for red
   @param pinB the pin on the expander to be used for yellow
   @param pinC the pin on the expander to be used for green
**/
    TrafficlightPCF8547(PCF8574expander &hardware, uint8_t pinA, uint8_t pinB, uint8_t pinC) : Trafficlight(hardware_IF), hardware_IF(hardware, pinA, pinB, pinC) {};
};

/**
   \brief turnsignals for a car with 3 PCF8547 pins.
   
   wrapper for a turning signal with 3 PCF8547 pin.
   Inherits "style" class and composites PCF8547 pin.
*/
class TurnsignalPCF8547 : public Turnsignal<PCF8574_IFGroup<3>> {
    PCF8574_IFGroup<3> hardware_IF;
  public:
/**
   @param hardware a reference to your PCF8574 object
   @param pinA the pin on the expander to be used for left
   @param pinB the pin on the expander to be used for right
   @param pinC the pin on the expander to be used for hazard light
**/
    TurnsignalPCF8547(PCF8574expander &hardware, uint8_t pinA, uint8_t pinB, uint8_t pinC) : Turnsignal(hardware_IF), hardware_IF(hardware, pinA, pinB, pinC) {};
}; 
 