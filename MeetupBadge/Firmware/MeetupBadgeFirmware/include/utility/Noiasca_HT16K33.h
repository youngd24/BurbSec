/*
  Noiasca Tool Kit - Outputs for LEDs and Relais - HT16K33
  
  the hardware pins are abstracted from the styles.
  wrapper make the usage easier
  
  The HT16K33 provides multiplexed output for 128 LEDs.
  8 Common Cathodes for 16 Common Anodes.
  
  Each individual LED can be used with this library.
  There is no PWM on the HT16K33.
  
  todo
  - complete tests for one LED classes
  - untested for classes with PWM
  - untested for classes with several LEDs

  copyright 2023 noiasca noiasca@yahoo.com
  
  Version
  2023-02-18        added HT16K33
*/

#pragma once

/**  
    \brief HT16K33 Expander Hardware class
    
    Takes Wire and I2C address and administrates the port. 
    This connects the low level hardware. 
    The class is needed becauses each pin on the HT16K33 should be treated seperately
    but the IC accepts needs a bitmask for each common cathode.    
 */ 
 
 /* internal comments
    in case of the HT16K33 we use the term "pixel" for a multiplexed output of a cathode and anode
  */
class HT16K33expander { 
  protected:
    static const uint8_t HT16K33_OSCILATOR_ON  {0x21};     // System Setup Register 0x20 + 0x01
    static const uint8_t HT16K33_CMD_BRIGHTNESS  {0xE0};   // Dimming set (Datasheet says EF, but this is just with the set bits D12-D8 for full brightness
    TwoWire *i2cPort;				           // generic connection to user's chosen I2C hardware
    const uint8_t i2cAddr;             // I2C address of expander
    uint16_t pinStatus[8] {0};         // stores io state of expander pins to make them available in parallel instances
   
  public:
/**
   @param i2cAddr the I2C address (0x70 - 0x77)
**/
    HT16K33expander(uint8_t i2cAddr) : i2cPort(&Wire), i2cAddr(i2cAddr) {}  // default Wire library
    
/**
   @param i2cAddr the I2C address (0x70 - 0x77)
   @param i2cPort use Wire or any other TwoWire (I2C) interface you have available on your microcontroller
**/
    HT16K33expander(TwoWire &i2cPort, uint8_t i2cAddr) : i2cPort(&i2cPort), i2cAddr(i2cAddr) {}

  int begin() {
    Wire.beginTransmission(i2cAddr);
    Wire.write(HT16K33_OSCILATOR_ON);            // turn on oscillator
    int result = Wire.endTransmission();
    clear();
    setBrightness(15);                           // max brightness
    return result;
  }
  
/**
   reset all pins 
*/
  int clear() {   
    Wire.beginTransmission(i2cAddr);
    for (uint8_t j = 0; j < 16 + 1; j++) {       // for all 8 positions per device (1 startregister, 16 adresses)
      Wire.write(0);
    }
    int result = Wire.endTransmission();
    for (uint8_t j = 0; j < 8; j++) {            // for all 8 positions per device 
      pinStatus[j] = 0;
    } 
    return result;    
  }

/**
   set the brightness of the display 
*/
  int setBrightness(uint8_t b) {
    if (b > 15) b = 15;
    Wire.beginTransmission(i2cAddr);
    Wire.write(HT16K33_CMD_BRIGHTNESS | b);
    int result = Wire.endTransmission();
    return result;
  }  
    
/**
   \brief write to a pin
   
   set or unset an output pin. 
   The pin is 0..127 (8 common cathodes with 16 common anodes)
   @param pin the pin to write.
   @param val the value LOW or HIGH
*/
    void digitalWrite(uint8_t pin, uint8_t val) {
      if (pin > 127) return;
      byte cathode = pin / 16;
      byte anode = pin % 16;
          
      if (val == LOW)
        bitClear(pinStatus[cathode], anode);
      else 
        bitSet(pinStatus[cathode], anode);
    
      i2cPort->beginTransmission(i2cAddr);
      i2cPort->write(cathode * 2);
      i2cPort->write(pinStatus[cathode] & 0xFF);
      i2cPort->write(pinStatus[cathode] >> 8);
      i2cPort->endTransmission();
    }
    
///**
//   \brief read pin status
//   
//   @return the value of the internal pin status
//*/
//    uint8_t getPinStatus()
//    {
//      return pinStatus;
//    }

/**
   \brief set pin status
   
   This function only sets the internal variable - doesn't change pins
   
   @param _pinStatus the new value for the pin status
*/    
    void setPinStatus(uint8_t _pinStatus) {
      pinStatus[0] = _pinStatus;
    }
};


/*
   class to encapsulate a LED on a HT16K33 into object
   and to offer a unified interface
*/
class HT16K33 {
    HT16K33expander &ic;
    const uint8_t startPixel;          // one output on the expander IC

  public:
    HT16K33(HT16K33expander &ic, uint16_t startPixel) : ic(ic), startPixel(startPixel) {}

    void begin() // no need, the ic needs one begin only.
    {}

    void digWrite(uint8_t val) {
      if (val == LOW) 
        ic.digitalWrite(startPixel, 0);
      else
        ic.digitalWrite(startPixel, 1);
    }
    
    void digWrite(uint8_t pixel, uint8_t val)
    {
      ic.digitalWrite(startPixel + pixel, val);
    }
    
    int digRead() {
      return LOW;  // no implementation. Alternative - read back status
    } 
    
/* 
    The HT16K33 can't do PWM.
    Therefore a hack for a simple on/off
*/
    void pwmWrite(uint16_t value) {
      if (value < 127)
        digWrite(LOW);
      else
        digWrite(HIGH);
    }
};

/*
   class to encapsulate several LEDs on a HT16K33 into one object
   and to offer a unified interface
*/
template<size_t noOfPixel>
class HT16K33Group {
    HT16K33expander &ic;
    uint16_t pixel[noOfPixel];         // pixel index on ic

  public:
    HT16K33Group(HT16K33expander &ic, uint16_t pixelA, uint16_t pixelB, uint16_t pixelC) : ic(ic), pixel{pixelA, pixelB, pixelC} {}

    void begin() {}                    // no need, the ic needs one begin only.
    
    // all pins
    void digWrite(uint8_t newState) {
      for (size_t i = 0; i < noOfPixel; i++)
        digWrite(pixel[i], newState);
    }
    
    // one dedicated pin
    void digWrite(size_t actual, uint8_t newState) {
      if (newState == LOW)  
        ic.digitalWrite(pixel[actual], 0);
      else 
        ic.digitalWrite(pixel[actual], 1);
    }

    // a read for group of pixels doesn't make sense
    //int digRead(){}    
    void pwmWrite(uint8_t value) {
      for (size_t i = 0; i < noOfPixel; i++)
        pwmWrite(i, value);        
    }
  
    void pwmWrite(size_t actual, uint8_t value) {
      if (value < 127)
        digWrite(actual, LOW);
      else
        digWrite(actual, HIGH);
    } 
};

/* **************************************************************
  wrapper make to make the interface more 
  userfriendly
  ************************************************************** */

/**
   \brief alternate blinking of two HT16K33 outputs.
   
   wrapper to alternate two HT16K33 outputs.
   Inherits "style" class and composites HT16K33
*/
class AlternatingHT16K33 : public Alternating<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your HT16K33 object
   @param pixel the first of two pixels on the HT16K33 to be used (also the next pixel will be used!)
**/
    AlternatingHT16K33(HT16K33expander &ic, uint16_t pixel) : Alternating(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief blink a LED on a HT16K33 output.
   
   wrapper to blink a LED on a HT16K33 output.
   Inherits "style" class and composites HT16K33
*/
class BlinkHT16K33 : public Blink<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixel the pixel on the ic to be used 
**/
    BlinkHT16K33(HT16K33expander &ic, byte pixel) : Blink(ht16k33), ht16k33(ic, pixel) {};                    // old default way...
};

/**
   \brief bounce 5 LEDs on HT16K33 between left and right 
   
   wrapper to bounce a series of 5 HT16K33 outputs.
   Inherits "style" class and composites HT16K33
   @note the 5 ht16k33 pins must be in sequence
   (could be changed one day using HT16K33Group<5>)
*/
class Bounce5HT16K33 : public Bounce5<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your HT16K33 output
   @param pixel the first of 5 outputs on the HT16K33 to be used 
**/
    Bounce5HT16K33(HT16K33expander &ic, uint16_t pixel) : Bounce5(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief enable several effects on a HT16K33 output
   
   wrapper to do several effects on a HT16K33 output. 
   Inherits "style" class and composites HT16K33.
*/
class EffectHT16K33 : public Effect<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   \param ic a reference to a IC object
   \param pixel a cathode/anode index (0..127) on the IC
**/
    EffectHT16K33(HT16K33expander &ic, uint16_t pixel) : Effect(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief simulate a flickering light like a fire on a HT16K33
   
   wrapper to flicker an output on a HT16K33 output.
   Inherits "style" class and composites HT16K33
*/
class FlickerHT16K33 : public Flicker<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your HT16K33 output
   @param pixel the output on the HT16K33 to be used 
**/
    FlickerHT16K33(HT16K33expander &ic, uint16_t pixel) : Flicker(ht16k33), ht16k33(ic, pixel) {};
};

/**
    \brief Imitiate fluorescent lamp with a HT16K33 output
    
   wrapper for imitating Fluorescent on a HT16K33 output.
   Inherits "style" class and composites HT16K33
*/
class FluorescentHT16K33 : public Fluorescent<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your HT16K33 object
   @param pixel the output on the HT16K33 to be used 
**/
    FluorescentHT16K33(HT16K33expander &ic, uint16_t pixel) : Fluorescent(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief heart beat a HT16K33.
   
   wrapper to heartbeat an output on a HT16K33.
   Inherits class heartbeat and composites HT16K33
*/
class HeartbeatHT16K33 : public Heartbeat<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixel the pixel on the ic to be used 
**/
    HeartbeatHT16K33(HT16K33expander &ic, uint16_t pixel) : Heartbeat(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief a on/off one output on HT16K33
   
   wrapper for an simple on/off on a HT16K33.
   This class provides a simple on() off() interface for a HT16K33.
   There is no effect during runtime.
   Inherits class pulse and composites HT16K33
*/
class OnOffHT16K33 : public OnOff<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixel the pixel on the ic to be used 
**/
    OnOffHT16K33(HT16K33expander &ic, uint16_t pixel) : OnOff(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief pulse a HT16K33 output (monoflop)
   
   wrapper to pulse a HT16K33 output.
   Inherits class pulse and composites HT16K33
*/
class PulseHT16K33 : public Pulse<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixel the pixel on the ic to be used 
**/
    PulseHT16K33(HT16K33expander &ic, uint16_t pixel) : Pulse(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief show a specific rhythm on a HT16K33 output.
   
   wrapper to rhythm blink a HT16K33 output.
   Inherits "style" class and composites HT16K33.
*/
class RhythmHT16K33 : public Rhythm<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixel the pixel on the ic to be used 
**/
    RhythmHT16K33(HT16K33expander &ic, uint16_t pixel) : Rhythm(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief Switch on/off a HT16K33 output smoothly
   
   wrapper to smooth HT16K33 output.
   Inherits "style" class and composites HT16K33.
*/
class SmoothHT16K33 : public Smooth<HT16K33> {
    HT16K33 ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixel the pixel on the ic to be used 
**/
    SmoothHT16K33(HT16K33expander &ic, uint16_t pixel) : Smooth(ht16k33), ht16k33(ic, pixel) {};
};

/**
   \brief a Traffic light with 3 HT16K33 outputs
   
   wrapper for a traffic light with 3 HT16K33 outputs.
   Inherits "style" class and composites HT16K33.
*/
class TrafficlightHT16K33 : public Trafficlight<HT16K33Group<3>> {
    HT16K33Group<3> ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixelA the pixel on the ic to be used for red
   @param pixelB the pixel on the ic to be used for yellow
   @param pixelC the pixel on the ic to be used for green
**/
    TrafficlightHT16K33(HT16K33expander &ic, byte pixelA, byte pixelB, byte pixelC) : Trafficlight(ht16k33), ht16k33(ic, pixelA, pixelB, pixelC) {};
};

/**
   \brief turnsignals for a car with 3 HT16K33 outputs.
   
   wrapper for a turning signal with 3 HT16K33 outputs.
   Inherits "style" class and composites HT16K33.
*/
class TurnsignalHT16K33 : public Turnsignal<HT16K33Group<3>> {
    HT16K33Group<3> ht16k33;
  public:
/**
   @param ic a reference to your ic object
   @param pixelA the pixel on the ic to be used for left
   @param pixelB the pixel on the ic to be used for right
   @param pixelC the pixel on the ic to be used for hazard light
**/
    TurnsignalHT16K33(HT16K33expander &ic, byte pixelA, byte pixelB, byte pixelC) : Turnsignal(ht16k33), ht16k33(ic, pixelA, pixelB, pixelC) {};
};
