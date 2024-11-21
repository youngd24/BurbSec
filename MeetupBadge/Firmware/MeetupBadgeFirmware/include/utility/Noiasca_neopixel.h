/*
  Noiasca Tool Kit - Outputs for LEDs and Relais 
  
  the hardware pins are abstracted from the styles.
  wrapper make the usage easier for the use
  
  copyright 2022 noiasca noiasca@yahoo.com
  
  Version
  2022-12-19       added multi effect
  2022-02-15       OnOffPixel
  2022-02-12 0.0.2 split Neopixel from main library
  2022-01-22       fixes for ESP8266 and ESP32
  2022-01-17 0.0.1 initial version
*/

#pragma once
#include <Adafruit_NeoPixel.h> 

/*
   class to encapsulate a pixel on a Neostrip into object
   and to offer a unified interface
*/
class NeoPixel {
    Adafruit_NeoPixel &strip;
    const uint16_t startPixel;         // one pixel of the strip
    uint32_t onColor = 0x808080;       // color when pixel is on
    uint32_t offColor = 0x000000;      // color when pixel is "off" - could be another colour than "black"

  public:
    NeoPixel(Adafruit_NeoPixel &strip, uint16_t startPixel) : strip(strip), startPixel(startPixel) {}

    void begin() {} // no need, the strip needs one begin only.
    

    void digWrite(uint8_t val) {
      if (val == 0) strip.setPixelColor(startPixel, offColor); else strip.setPixelColor(startPixel, onColor);
      strip.show();
    }
    
    void digWrite(uint8_t pixel, uint8_t val) {
      if (val == 0) strip.setPixelColor(startPixel + pixel, offColor); else strip.setPixelColor(startPixel + pixel, onColor);
      strip.show();
    }

    int digRead() {
      if (strip.getPixelColor(startPixel) == offColor)
        return LOW;
      else
        return HIGH;
    }    
/*
    
    @todo rework according https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
*/
    void pwmWrite(int pwm) {
      uint8_t r = onColor >> 16;
      uint8_t g = (onColor & 0xFF00) >> 8;
      uint8_t b = onColor & 0xFF;
      r = r * pwm / 255;
      g = g * pwm / 255;
      b = b * pwm / 255;
      uint32_t color = strip.Color(r, g, b);
      strip.setPixelColor(startPixel, color);
      strip.show();
    }

    void setOnColor(uint32_t _onColor) {
      onColor = _onColor;
    }
   
    void setOffColor(uint32_t _offColor) {
      offColor = _offColor;
    }
};

/*
   class to encapsulate several pixel on a Neostrip into one object
   and to offer a unified interface
*/
template<size_t noOfPixel>
class NeoPixelGroup {
    Adafruit_NeoPixel &strip;          // the strip object to be used
    uint16_t pixel[noOfPixel];         // pixel index on strip
    uint32_t onColor[noOfPixel];       // on color for each pixel
    uint32_t offColor = 0x000000;      // one "off" color for all

  public:
    NeoPixelGroup(Adafruit_NeoPixel &strip, uint16_t pixelA, uint16_t pixelB, uint16_t pixelC) : strip(strip), pixel{pixelA, pixelB, pixelC} {
      for (auto &i : onColor) i = 0x808080;
    }

    void begin() {}   // no need, the strip needs one begin only.
    
    void digWrite(uint8_t newState) {
      digWrite(0, newState);
    }
    
    void digWrite(size_t actual, uint8_t newState) {
      if (newState == 0) strip.setPixelColor(pixel[actual], offColor); else strip.setPixelColor(pixel[actual], onColor[actual]);
      strip.show();
    }

    // a read for group of pixels doesn't make sense
    //int digRead(){}    
/*
    
    @todo rework according https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
*/
    void pwmWrite(uint8_t pwm) {
      pwmWrite(0, pwm);
    }

    void pwmWrite(size_t actual, uint8_t pwm) {
      uint8_t r = onColor[actual] >> 16;
      uint8_t g = (onColor[actual] & 0xFF00) >> 8;
      uint8_t b = onColor[actual] & 0xFF;
      r = r * pwm / 255;
      g = g * pwm / 255;
      b = b * pwm / 255;
      uint32_t color = strip.Color(r, g, b);
      strip.setPixelColor(pixel[actual], color);
      strip.show();
    }

    void setOnColor(uint16_t actual, uint32_t _onColor) {
      onColor[actual] = _onColor;
    }
   
    void setOffColor(uint32_t _offColor) {
      offColor = _offColor;
    }   
};

/* **************************************************************
  wrapper make to make the interface more 
  userfriendly
  ************************************************************** */

/**
   \brief alternate blinking of two Neopixels
   
   wrapper to alternate two Neopixel.
   Inherits "style" class and composites NeoPixel
*/
class AlternatingPixel : public Alternating<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the first of two pixels on the strip to be used (also the next pixel will be used!)
**/
    AlternatingPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Alternating(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief blink a Neopixel.
   
   wrapper to blink a Neopixel.
   Inherits "style" class and composites NeoPixel
*/
class BlinkPixel : public Blink<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    BlinkPixel(Adafruit_NeoPixel &strip, byte pixel) : Blink(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief bounce 5 Neopixel between left and right 
   
   wrapper to bounce a series of 5 Neopixel.
   Inherits "style" class and composites NeoPixel
   @note the 5 Neopixel must be in sequence
   (could be changed one day using NeoPixelGroup<5>)
*/
class Bounce5Pixel : public Bounce5<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the first of 5 pixels on the strip to be used 
**/
    Bounce5Pixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Bounce5(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief enable several effects on a Neopixel
   
   wrapper to do several effects on a Neopixel. 
   Inherits "style" class and composites Neopixel
*/
class EffectPixel : public Effect<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   \param strip a refernce to a Adafruit_Neopixel object (the "strip")
   \param pixel the pixel to be used
**/
    EffectPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Effect(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief simulate a flickering light like a fire on a Neopixel
   
   wrapper to flicker a Neopixel.
   Inherits "style" class and composites NeoPixel
*/
class FlickerPixel : public Flicker<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    FlickerPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Flicker(neoPixel), neoPixel(strip, pixel) {};
};

/**
    \brief Imitiate fluorescent lamp with a Neopixel
    
   wrapper for a Fluorescent imitating Neopixel.
   Inherits "style" class and composites NeoPixel
*/
class FluorescentPixel : public Fluorescent<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    FluorescentPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Fluorescent(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief heart beat a Neopixel.
   
   wrapper to heartbeat a Neopixel.
   Inherits class heartbeat and composites NeoPixel
*/
class HeartbeatPixel : public Heartbeat<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    HeartbeatPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Heartbeat(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief a on/off one Neopixel
   
   wrapper for an simple on/off on a Neopixel.
   This class provides a simple on() off() interface for a Neopixel.
   There is no effect during runtime.
   Inherits class pulse and composites NeoPixel
*/
class OnOffPixel : public OnOff<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    OnOffPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : OnOff(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief pulse a Neopixel (monoflop)
   
   wrapper to pulse a Neopixel.
   Inherits class pulse and composites NeoPixel
*/
class PulsePixel : public Pulse<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    PulsePixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Pulse(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief show a specific rhythm on a Neopixel.
   
   wrapper to rhythm blink a Neopixel.
   Inherits "style" class and composites NeoPixel.
*/
class RhythmPixel : public Rhythm<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    RhythmPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Rhythm(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief Switch on/off a Neopixel smoothly
   
   wrapper to smooth Neopixel.
   Inherits "style" class and composites NeoPixel.
*/
class SmoothPixel : public Smooth<NeoPixel> {
    NeoPixel neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
**/
    SmoothPixel(Adafruit_NeoPixel &strip, uint16_t pixel) : Smooth(neoPixel), neoPixel(strip, pixel) {};
};

/**
   \brief a Traffic light with 3 Neopixel
   
   wrapper for a traffic light with 3 Neopixel.
   Inherits "style" class and composites NeoPixel.
   @note don't forget to set colors for the three Neopixels (e.g. red, yellow, green).
*/
class TrafficlightPixel : public Trafficlight<NeoPixelGroup<3>> {
    NeoPixelGroup<3> neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixelA the pixel on the strip to be used for red
   @param pixelB the pixel on the strip to be used for yellow
   @param pixelC the pixel on the strip to be used for green
**/
    TrafficlightPixel(Adafruit_NeoPixel &strip, byte pixelA, byte pixelB, byte pixelC) : Trafficlight(neoPixel), neoPixel(strip, pixelA, pixelB, pixelC) {};
};

/**
   \brief turnsignals for a car with 3 Neopixels.
   
   wrapper for a turning signal with 3 Neopixel.
   Inherits "style" class and composites NeoPixel.
   @note don't forget to set colors for the three Neopixels (e.g. orange, orange, red).
*/
class TurnsignalPixel : public Turnsignal<NeoPixelGroup<3>> {
    NeoPixelGroup<3> neoPixel;
  public:
/**
   @param strip a reference to your strip object
   @param pixelA the pixel on the strip to be used for left
   @param pixelB the pixel on the strip to be used for right
   @param pixelC the pixel on the strip to be used for hazard light
**/
    TurnsignalPixel(Adafruit_NeoPixel &strip, byte pixelA, byte pixelB, byte pixelC) : Turnsignal(neoPixel), neoPixel(strip, pixelA, pixelB, pixelC) {};
};
