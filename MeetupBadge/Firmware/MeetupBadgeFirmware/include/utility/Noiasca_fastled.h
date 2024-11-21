/*
  Noiasca Tool Kit - Outputs for LEDs and Relais
  
  "Neopixel" WS2811 WS2812 effects with "FastLED" library
  
  The hardware interface is abstracted from the styles.
  wrapper make the usage easier
  
  copyright 2022 noiasca noiasca@yahoo.com
  
  Version
  2022-12-19       added multi effect
  2022-04-15       initial version
*/

#pragma once
#include <FastLED.h>

/*  **************************************************
    Hardware classes 
    to unify the access to outputs    
    ************************************************** */ 
    
/*
   class to encapsulate a pixel on a Fastled strip into object
   and to offer a unified interface
*/
class Fastled_IF 
{ 
    CFastLED &strip;                   // reference to the strip
    const uint16_t startPixel;         // one pixel of the strip
    struct CRGB *leds;                 // a pointer to the leds array
    uint32_t onColor = 0x808080;
    uint32_t offColor = 0x000000;

  public:
    Fastled_IF(CFastLED &strip, uint16_t startPixel, struct CRGB *leds) : strip(strip), startPixel(startPixel), leds(leds) {}
    
    void begin() // no need, the strip needs one begin only.
    {}

    void digWrite(uint8_t val)
    {
      if (val == 0) leds[startPixel] = offColor; else leds[startPixel] = onColor;
      strip.show();
    }
    
    void digWrite(uint8_t pixel, uint8_t val)
    {
      if (val == 0) leds[startPixel + pixel] = offColor; else leds[startPixel + pixel] = onColor;
      strip.show();
    }

    int digRead()
    {
      if ((uint32_t)leds[startPixel] == offColor)
        return LOW;
      else
        return HIGH;
    }    
/*
    @todo rework according https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
*/
    void pwmWrite(int pwm)
    {
      uint8_t r = onColor >> 16;
      uint8_t g = (onColor & 0xFF00) >> 8;
      uint8_t b = onColor & 0xFF;
      r = r * pwm / 255;
      g = g * pwm / 255;
      b = b * pwm / 255;
      uint32_t color = (uint32_t)r << 16 | (uint16_t)g << 8 | b;
      leds[startPixel] = color;
      strip.show();
    }

    void setOnColor(uint32_t _onColor)
    {
      onColor = _onColor;
    }
   
    void setOffColor(uint32_t _offColor)
    {
      offColor = _offColor;
    }
};

/*
   class to encapsulate several pixel on a Neostrip into one object
   and to offer a unified interface
*/

template<size_t noOfPixel>
class Fastled_IFGroup {
    CFastLED &strip;
    uint16_t pixel[noOfPixel];         // pixel index on strip
    struct CRGB *leds;                 // a pointer to the leds array
    uint32_t onColor[noOfPixel];       // on color for each pixel
    uint32_t offColor = 0x000000;      // one "off" color for all

  public:
    Fastled_IFGroup(CFastLED &strip, uint16_t pixelA, uint16_t pixelB, uint16_t pixelC, struct CRGB *leds) : strip(strip), pixel{pixelA, pixelB, pixelC}, leds{leds} 
    {
      for (auto &i : onColor) i = 0x808080;
    }

    void begin() {}   // no need, the strip needs one begin only.
    
    void digWrite(uint8_t newState)
    {
      digWrite(0, newState);
    }
    
    void digWrite(size_t actual, uint8_t newState)
    {
      if (newState == 0) leds[actual] = offColor; else leds[actual] = onColor[actual];
      strip.show();
    }

    // a read for group of pixels doesn't make sense
    //int digRead(){}    

    void pwmWrite(uint8_t pwm)
    {
      pwmWrite(0, pwm);
    }

    void pwmWrite(size_t actual, uint8_t pwm)
    {
      uint8_t r = onColor[actual] >> 16;
      uint8_t g = (onColor[actual] & 0xFF00) >> 8;
      uint8_t b = onColor[actual] & 0xFF;
      r = r * pwm / 255;
      g = g * pwm / 255;
      b = b * pwm / 255;
      uint32_t color = (uint32_t)r << 16 | (uint16_t)g << 8 | b;
      leds[actual] = color;
      strip.show();
    }

    void setOnColor(uint16_t actual, uint32_t _onColor)
    {
      onColor[actual] = _onColor;
    }
   
    void setOffColor(uint32_t _offColor)
    {
      offColor = _offColor;
    }   
};

/* **************************************************************
  wrapper make to make the interface more 
  userfriendly
  ************************************************************** */
 
/**
   \brief alternate blinking of two FastLED pixels.
   
   wrapper to alternate two FastLED pixel. 
   Inherits "style" class and composites FastLED_IF.
*/
class AlternatingFastLED : public Alternating<Fastled_IF> {
    Fastled_IF fastled_IF;   // wrapper needs a HW Interface
  public:
/**
   @param strip a reference to your strip object
   @param pixel the first of two pixels on the strip to be used (also the next pixel will be used!)
   @param *leds a pointer to your leds array
**/
    AlternatingFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Alternating(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief blink a FastLED pixel.
   
   wrapper to blink a FastLED pixel.
   Inherits "style" class and composites FastLED_IF.
*/
class BlinkFastLED : public Blink<Fastled_IF> {
    Fastled_IF fastled_IF;   // wrapper needs a HW Interface
  public:
/**
   @param strip a reference to your fastLED strip object
   @param pixel the pixel on the strip to be used 
   @param *leds a pointer to your leds array
**/
    BlinkFastLED(CFastLED &strip, byte pixel, struct CRGB *leds) : Blink(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief bounce 5 FastLED pixels between left and right 
   
   wrapper to bounce a series of 5 FastLED pixels.
   Inherits "style" class and composites FastLED
   @note the 5 FastLED pixels must be in sequence
   (could be changed one day using another fastLED_IFGroup)
*/
class Bounce5FastLED : public Bounce5<Fastled_IF> {
    Fastled_IF fastled_IF;   // wrapper needs a HW Interface
  public:
/**
   @param strip a reference to your strip object
   @param pixel the first of 5 pixels on the strip to be used
   @param *leds a pointer to your leds array   
**/
    Bounce5FastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Bounce5(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief enable several effects on a FastLED
   
   wrapper to do several effects on a FastLED. 
   Inherits "style" class and composites FastLED.
*/
class EffectFastLED : public Effect<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used
   @param *leds a pointer to your leds array
**/  
    EffectFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Effect(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief simulate a flickering light like a fire on a FastLED
   
   wrapper to flicker a FastLED.
   Inherits "style" class and composites FastLED
*/
class FlickerFastLED : public Flicker<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used
   @param *leds a pointer to your leds array
**/
    FlickerFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Flicker(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
    \brief Imitiate fluorescent lamp with a FastLED
    
   wrapper for a Fluorescent imitating FastLED.
   Inherits "style" class and composites FastLED
*/
class FluorescentFastLED : public Fluorescent<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used
   @param *leds a pointer to your leds array   
**/
    FluorescentFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Fluorescent(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief heart beat a FastLED.
   
   wrapper to heartbeat a FastLED.
   Inherits class heartbeat and composites FastLED
*/
class HeartbeatFastLED : public Heartbeat<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used
   @param *leds a pointer to your leds array    
**/
    HeartbeatFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Heartbeat(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief a on/off one FastLED
   
   wrapper for an simple on/off on a FastLED.
   This class provides a simple on() off() interface for a FastLED.
   There is no effect during runtime.
   Inherits class pulse and composites FastLED
*/
class OnOffFastLED : public OnOff<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used
   @param *leds a pointer to your leds array   
**/
    OnOffFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : OnOff(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief pulse a FastLED (monoflop)
   
   wrapper to pulse a FastLED.
   Inherits class pulse and composites FastLED
*/
class PulseFastLED : public Pulse<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used
   @param *leds a pointer to your leds array   
**/
    PulseFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Pulse(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief show a specific rhythm on a FastLED.
   
   wrapper to rhythm blink a FastLED.
   Inherits "style" class and composites FastLED.
*/
class RhythmFastLED : public Rhythm<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used 
   @param *leds a pointer to your leds array
**/
    RhythmFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Rhythm(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief Switch on/off a FastLED smoothly
   
   wrapper to smooth FastLED.
   Inherits "style" class and composites FastLED.
*/
class SmoothFastLED : public Smooth<Fastled_IF> {
    Fastled_IF fastled_IF;
  public:
/**
   @param strip a reference to your strip object
   @param pixel the pixel on the strip to be used
   @param *leds a pointer to your leds array   
**/
    SmoothFastLED(CFastLED &strip, uint16_t pixel, struct CRGB *leds) : Smooth(fastled_IF), fastled_IF(strip, pixel, leds) {};
};

/**
   \brief a Traffic light with 3 FastLED
   
   wrapper for a traffic light with 3 FastLED.
   Inherits "style" class and composites FastLED.
   @note don't forget to set colors for the three FastLEDs (e.g. red, yellow, green).
*/
class TrafficlightFastLED : public Trafficlight<Fastled_IFGroup<3>> {
    Fastled_IFGroup<3> fastled_IFGroup;
  public:
/**
   @param strip a reference to your strip object
   @param pixelA the pixel on the strip to be used for red
   @param pixelB the pixel on the strip to be used for yellow
   @param pixelC the pixel on the strip to be used for green
   @param *leds a pointer to your leds array
**/
    TrafficlightFastLED(CFastLED &strip, byte pixelA, byte pixelB, byte pixelC, struct CRGB *leds) : Trafficlight(fastled_IFGroup), fastled_IFGroup(strip, pixelA, pixelB, pixelC, leds) {};
};

/**
   \brief turnsignals for a car with 3 FastLEDs.
   
   wrapper for a turning signal with 3 FastLED.
   Inherits "style" class and composites FastLED.
   @note don't forget to set colors for the three FastLEDs (e.g. orange, orange, red).
*/
class TurnsignalFastLED : public Turnsignal<Fastled_IFGroup<3>> {
    Fastled_IFGroup<3> fastled_IFGroup;
  public:
/**
   @param strip a reference to your strip object
   @param pixelA the pixel on the strip to be used for left
   @param pixelB the pixel on the strip to be used for right
   @param pixelC the pixel on the strip to be used for hazard light
   @param *leds a pointer to your leds array
**/
    TurnsignalFastLED(CFastLED &strip, byte pixelA, byte pixelB, byte pixelC, struct CRGB *leds) : Turnsignal(fastled_IFGroup), fastled_IFGroup(strip, pixelA, pixelB, pixelC, leds) {};
};
