/*
  Noiasca Tool Kit - Outputs for LEDs and Relais
  
  The hardware pins are abstracted from the styles.
  Wrapper make the usage easier for the use
  
  copyright 2023 noiasca noiasca@yahoo.com
  
  Todo for this Test version
  - examples for general purpose effect 
  - add effect to other HW classes
  
  Version
  2023-08-04 0.3.1  reduced upate() to one signature @todo: remove finally in 0.4.0
  2023-07-10 0.3.1  initial previousMillis = millis()
  2023-02-18 0.3.0  added HT16K33
  2022-12-18 0.2.0  Split discrete pins from led, multi effect class
  2022-07-19 0.0.10 Heartbeat: setCurrentBrightness
  2022-05-20 0.0.9  Smooth: correct dim down 
  2022-04-11        Blink: onInterval accepts 32bit values, LedPin Callback for onStateChange
  2022-03-18 0.0.6  Blink: rework of states
  2022-03-12 0.0.5  getCurrentBrightness for pwm effects
  2022-03-11 0.0.4  Smooth reacts on maxBrightness if new value is lower
  2022-02-26 0.0.3  update can take a timestamp/millis()
  2022-02-15        OnOffPin
  2022-02-12 0.0.2  split Neopixel from main library     
  2022-01-22        fixes for ESP8266 and ESP32
  2022-01-17 0.0.1  initial version
*/

#pragma once
/*  **************************************************
    a base class 
    ************************************************** */ 

/*
   an abstract base class for 
   basic functions used in several classes
   needs a reference to the HW object, otherwise the unified functions can't be called.
   If you need direct access to a pin use the class onOff 
*/
template<class T>
class LedBase {
  protected:
    T &obj;                                      // a reference to an output object (pin, pixel, whatever...)
    uint8_t state = 1;                           // 0 OFF, 1 ON - default to on state, but might be overridden in some implementations
    using Callback = void (*)(uint8_t value);    // signature of a callback function
    Callback cbStateChange;                      // a callback function if the state changes
  
  public:
    LedBase(T &obj) : obj(obj) {}

/**
   \brief start hardware
   
   Will do the necessary steps to initialize the hardware pins.
   Call this function in your setup().
*/
    void begin() {
      obj.begin(); // call HW begin 
    }
    
/**
   \brief return the current state
   \return current state
*/    
    uint8_t getState() {
      return state;
    }
    
/**
   \brief switch off
*/    
    virtual void off() {
      uint8_t previousState = state;   // remember current state
      state = 0;
      if (cbStateChange && previousState != state) cbStateChange(state);
    }    
    
/**
   \brief switch on
*/
    virtual void on() {
      uint8_t previousState = state;   // remember current state
      state = 1;
      if (cbStateChange && previousState != state) cbStateChange(state);
    }
    
/**
   \brief switch between on or off state
*/
    virtual void toggle() {
      if (state == 0) on(); else off();
      // callback is done by on() or off() - if necessary
    } 
    
/** 
    \brief set the callback function onStateChange

    a callback function receives state changes from the effect 
    \param funcPtr the callback function
*/ 
    void setOnStateChange(Callback funcPtr) {
      cbStateChange = funcPtr;
    }

/**
   \brief set the color for a LED in on state
*/    
    void setOnColor(uint32_t _onColor) {
      obj.setOnColor(_onColor);
    }
    
/**
   \brief set the color for a LED in off state
*/    
    void setOffColor(uint32_t _offColor) {
      obj.setOffColor(_offColor);
    }
};


/*  **************************************************
    One class to combine all effects on a single LED
    Not usable for Multi LED effects like
      Bounce5
      Alternating
      Trafficlight
      Turnsignal
    ************************************************** */  
/**
   \brief Effect LED 
   
   A class with different effects for one LED.
   Uses a unified hw interface
*/
// this class (and it members) hould be the pattern for new single classes
template<class T>
class Effect : public LedBase <T> {
  protected:
    // state 0 off, 1 ON, 2 a different state within effect - from LedBase class
    enum Mode {ONOFF, BLINK, FLICKER, FLUORESCENT, HEARTBEAT, PULSE, RHYTHM, SMOOTH};
    Mode mode = ONOFF;                           // current mode of operation
    uint32_t previousMillis = millis();          // time management
    uint32_t previousMillisEffect = millis();    // timestamp for the effect (used in fluorescent)
   
    using brightness_t = uint8_t;                // for Arduino is 8 bit enough. As some code uses hardcoded 255         
    //typedef uint8_t brightness_t;              // for Arduino is 8 bit enough. As some code uses hardcoded 255         
    brightness_t maxBrightness = 255;            // maximum target brightness ("on")
    brightness_t minBrightness = 0;              // minimum target brightness ("off")
    brightness_t currentBrightness = 0;          // actual brightness of Pin

    uint8_t lengthOfPattern = 4;       // we have blink patterns with 2, 4, 6 or 8 times
    // the interval pattern are always in pairs of ON and OFF
    //                       ON  OFF   ON   OFF
    //                        1    2    3     4
    uint16_t interval[8] = {150,  60,  20,  270};          // ECE 2 uses 4 slots (but we need 8 in total
    //uint16_t onInterval = 500;       // interval[0] will be used instead of onInterval
    //uint16_t offInterval = 500;      // interval[1] will be used instead of offInterval
    
  public:
    Effect(T &obj) : LedBase<T>(obj) {}
 
/**
   \brief switch output off
   
   Switch the effect to off state.
*/
    void off() override {
      uint8_t previousState = LedBase<T>::state;     
      LedBase<T>::state = 0;
      if (mode != SMOOTH) {
        LedBase<T>::obj.digWrite(LOW);
        currentBrightness = 0; 
      }
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
    
/**
   \brief switch on
   
   If effect is off, switch the output on.
   \param force set to true if new state must be forced to the first ON state.
                set to false if only the OFF state should be switched ON (default).
*/  
    void on(bool force = false)        // signature differs hence no overwrite any more
    {
      uint8_t previousState = LedBase<T>::state; 
      if (LedBase<T>::state == 0 || force) 
        LedBase<T>::state = 1; 
      if (mode == ONOFF || mode == PULSE) {        
        LedBase<T>::obj.digWrite(HIGH);
        previousMillis = millis();     // needed for pulse
      }
      else if (mode == FLUORESCENT) {  
        currentBrightness = 0;
        previousMillis = millis();
        LedBase<T>::obj.pwmWrite(2);             // "glimm" after start
        interval[1] = random(50, 500);        // modify the first interval
        interval[0] = random(500, 5000);
      }
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);      
    }
    
///**
//   \brief switch on
//   
//   If effect is off, switch the output on.
//   \param force set to true if new state must be forced to the first ON state.
//                set to false if only the OFF state should be switched ON (default).
//*/  
//    void on(bool force) 
//    {
//      uint8_t previousState = LedBase<T>::state; 
//      if (force) 
//        LedBase<T>::state = 1;
//      else if (LedBase<T>::state == 0) 
//        LedBase<T>::state = 1;
//      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);            
//    }

/**
   \brief force switch off
   
   switches off imidiatly without smooth effect
*/  
//  ex smooth ...  tbd: replace with optional parameter in function off 
    void offForced() {
      uint8_t previousState = LedBase<T>::state; // remember current state
      LedBase<T>::state = 0;
      currentBrightness = 0; 
      LedBase<T>::obj.pwmWrite(currentBrightness);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }  

/**
   \brief get the current brightness
   
   The current brightness/level of this output
   @return the current brigthness
*/    
    brightness_t getCurrentBrightness() {
      return currentBrightness;
    }
    
/**
   \brief set the current brightness
   
   The current brightness/level of this output
*/    
    void setCurrentBrightness(brightness_t brightness) {
      currentBrightness = brightness;
      LedBase<T>::obj.pwmWrite(currentBrightness);
    }  
    
/**
   \brief set the maximum brightness
   
   The output will dimm up to this maximum level.
   \param maxBrightness the maximum brigthness (upper end of range) [0..255]
*/
    void setMaxBrightness(brightness_t maxBrightness) {
      this->maxBrightness = maxBrightness;
    }

/**
   \brief set on interval
   
   Set a new interval / time for how long the LED should be on.
   \param newInterval new interval in milliseconds
*/ 
    void setOnInterval(uint16_t newInterval) {
      interval[0] = newInterval;
    }

/**
   \brief set off interval
   
   Set a new interval / time for how long the LED should be off.
   \param newInterval new interval in milliseconds
*/     
    void setOffInterval(uint16_t newInterval) {
      interval[1] = newInterval;
    }    

/**
   \brief switch between on or off state
   
   If the LED is on - switch it off.
   If the LED is off - switch it on.
*/
    void toggle() override {
      if (LedBase<T>::state == 0) on(); else off();
      // callback is done by on() and off()
    }  

/**
   \brief check if update is necessary
   
   This is the "run" function. 
   Call this function in loop() to make the effect visible.
   This run function calls the necessary effect updates
   
   \param currentMillis you can handover a millis timestamp
   \todo not all effects are currently supported
*/     
    void update(uint32_t currentMillis = millis()) {
      switch (mode) {
        case ONOFF : break;                      // nothing to do - avoid compiler warning
        case BLINK : blink(currentMillis); break;// could be replaced by RYHTM with one pair
        case FLICKER: flicker(currentMillis); break;
        case FLUORESCENT : fluorescent(currentMillis); break;
        case HEARTBEAT : heartbeat(currentMillis); break;
        case PULSE : pulse(currentMillis); break;
        case RHYTHM : rhythm(currentMillis); break;
        case SMOOTH : smooth(currentMillis); break;
      }
    }

    // this functions takes either 2, 4, 6 or 8 parameters
    // it is mainly used for the rhythm pattern
/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on interval 
   \param _interval1 the off interval
*/
    void setInterval(uint16_t _interval0, uint16_t _interval1) {     
      interval[0] = _interval0;
      interval[1] = _interval1;
      lengthOfPattern = 2;
    }
/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on  interval 
   \param _interval1 the off interval
   \param _interval2 the on  interval
   \param _interval3 the off interval
*/
    void setInterval(uint16_t _interval0, uint16_t _interval1, uint16_t _interval2, uint16_t _interval3) {
      interval[0] = _interval0;
      interval[1] = _interval1;
      interval[2] = _interval2;
      interval[3] = _interval3;
      lengthOfPattern = 4;
    }
/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on  interval 
   \param _interval1 the off interval
   \param _interval2 the on  interval
   \param _interval3 the off interval
   \param _interval4 the on  interval
   \param _interval5 the off interval
*/   
    void setInterval(uint16_t _interval0, uint16_t _interval1, uint16_t _interval2, uint16_t _interval3, uint16_t _interval4, uint16_t _interval5) {
      interval[0] = _interval0;
      interval[1] = _interval1;
      interval[2] = _interval2;
      interval[3] = _interval3;
      interval[4] = _interval4;
      interval[5] = _interval5;
      lengthOfPattern = 6;
    }

/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on  interval
   \param _interval1 the off interval
   \param _interval2 the on  interval
   \param _interval3 the off interval
   \param _interval4 the on  interval
   \param _interval5 the off interval
   \param _interval6 the on  interval
   \param _interval7 the off interval
*/
    void setInterval(uint16_t _interval0, uint16_t _interval1, uint16_t _interval2, uint16_t _interval3, uint16_t _interval4, uint16_t _interval5, uint16_t _interval6, uint16_t _interval7) {
      interval[0] = _interval0;
      interval[1] = _interval1;
      interval[2] = _interval2;
      interval[3] = _interval3;
      interval[4] = _interval4;
      interval[5] = _interval5;
      interval[6] = _interval6;
      interval[7] = _interval7;
      lengthOfPattern = 8;      
    }

/**
   \brief activate OnOff mode
   
   a simple on off output
*/    
    void setModeOnOff() {
      mode = ONOFF;
    }

/**
   \brief activate Blink mode
   
   sets also the default intervals
*/     
    void setModeBlink() {
      mode = BLINK;
      setOnInterval(500);
      setOffInterval(500);
    }

/**
   \brief activate Flicker mode
   
*/  
    void setModeFlicker() {
      mode = FLICKER;
      
      // previousMillis = 0;       // time management
      interval[0] = 100;
      // maxBrightness = 255;
      if( LedBase<T>::state) LedBase<T>::state = 1;       
    }  

/**
   \brief activate Fluorescent mode
   
   sets also the default intervals
*/ 
    void setModeFluorescent() {
      mode = FLUORESCENT;
      interval[0] = random(500, 5000);           // is also in on()
      interval[1] = random(50, 500);          // modify the first interval
      if( LedBase<T>::state) LedBase<T>::state = 1; 
    } 

/**
   \brief activate Heartbeat mode
   
   sets also the default intervals
*/    
    void setModeHeartbeat() {
      mode = HEARTBEAT;
      interval[0] = 5;
      currentBrightness = 0;      // the current brightness
      minBrightness = 0;          // minimum brightness
      maxBrightness = 255;        // maximum brigthness
      if( LedBase<T>::state) LedBase<T>::state = 1; 
    } 

/**
   \brief activate Pulse mode
   
   sets also the default intervals
*/
    void setModePulse() {
      mode = PULSE;
      setOnInterval(500);
      if( LedBase<T>::state) LedBase<T>::state = 1; 
    } 

/**
   \brief activate Rhythm mode
   
   sets also the default intervals to ECE2.
   \see setInterval
*/    
    void setModeRhythm() {
      mode = RHYTHM;
      setInterval(150,  60,  20,  270);                    // ECE 2             
      //setInterval(180, 320);                             // ECE 1
      //setInterval(25, 25, 25, 25, 25, 375);              // HELLA 3
      //setInterval(40, 40, 40, 40, 40, 40, 40, 220);      // HELLA 4 
    } 

/**
   \brief activate Smooth mode
   
   sets also the default intervals
*/    
    void setModeSmooth() {
      mode = SMOOTH;
      interval[0] = 25;                // delay for each step upwards
      interval[1] = 15;                // delay for each step downwards
    }  
    
/**
   \brief "run" function for blink effect
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/      
    void blink(uint32_t currentMillis) {
      if (LedBase<T>::state != 0) {
        //uint32_t currentMillis = millis();
        if (LedBase<T>::state == 2) {
          if (currentMillis - previousMillis >= interval[0]) {
            LedBase<T>::state = 1;
            LedBase<T>::obj.digWrite(LOW);
            previousMillis = currentMillis;
            if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
          }
        }
        else {
          if (currentMillis - previousMillis >= interval[1]) {
            LedBase<T>::state = 2;
            LedBase<T>::obj.digWrite(HIGH);
            previousMillis = currentMillis;
            if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);            
          }
        }
      }
    }

/**
   \brief "run" function for flicker effect
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void flicker(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state == 1) {                                  // flicker in low wind
        if (currentMillis - previousMillis > interval[0])
        {
          //uint8_t value = (25 - random(22)) * 10 + 5;
          int value = (random(maxBrightness) / 10) * 10 + 5;
          LedBase<T>::obj.pwmWrite(value);
          interval[0] = random(20, 150);
          previousMillis = currentMillis;
        }
      }
    }

/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void fluorescent(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state == 1) 
      {
        if (currentMillis - previousMillisEffect > interval[1]) {    // former variable was intervalEffect
          if (currentBrightness >= 200) {                  // alles ab diesem Wert ist "ein" daher müssen wir nun "aus" Schalten
            currentBrightness = random(0, 5);              // Ein leichtes Glimmen der "Enden" ... könnte man auch ganz auf 0 setzen
            interval[1] = random(400, 2000);               // unregeläßige Dunkelphasen (zwischen dem Aufblitzen)
          }
          else {
            currentBrightness = random(200, 255);
            interval[1] = random(20, 40);                  // flash shortly
          }
          LedBase<T>::obj.pwmWrite(currentBrightness);
          previousMillisEffect = currentMillis;
        }
        if (currentMillis - previousMillis > interval[0]) {
          currentBrightness = 200;     // after the tube is stable "on" it will take some time to reach 100%
          interval[0] = 100;           // we need 55 steps to get full 255 PWM = aprox 55 Seconds till the tube has full brightness
          LedBase<T>::obj.pwmWrite(currentBrightness);
          previousMillis = currentMillis;
          LedBase<T>::state = 2;
          if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
        }
      }
      if (LedBase<T>::state == 2) {
        if (currentMillis - previousMillis >= interval[0]) {
          previousMillis = currentMillis;
          currentBrightness++;
          LedBase<T>::obj.pwmWrite(currentBrightness);
          if (currentBrightness >= 255) {
            LedBase<T>::state = 3;
            if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
          }
        }
      }
    }    
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/ 
// idea:  use interval[1] for down interval
    void heartbeat(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state != 0 && currentMillis - previousMillis > interval[0]) {
        //Serial.print(F("D628 ")); Serial.println(currentBrightness);
        previousMillis = currentMillis;
        if (LedBase<T>::state == 1) {  // going upwards
          if (currentBrightness < maxBrightness) currentBrightness++;
          else LedBase<T>::state = 2;  // turn direction to downwards
        }
        else {                         // going downwards
          if (currentBrightness > minBrightness) currentBrightness--;
          else LedBase<T>::state = 1;  // turn direction to upwards
        }
        LedBase<T>::obj.pwmWrite(currentBrightness);
      }
    } 

// old logic with odd/even state
    void heartbeatOLD(uint32_t currentMillis = millis()) 
    {
      if (LedBase<T>::state != 0 && currentMillis - previousMillis > interval[0]) {
        //Serial.print(F("D654 ")); Serial.println(currentBrightness);
        previousMillis = currentMillis;
        if (currentBrightness % 2)  {                       // odd - going upwards
          if (currentBrightness < maxBrightness - 1 ) 
            currentBrightness = currentBrightness + 2;
          else 
            currentBrightness = currentBrightness - 1;  // turn direction to downwards
        }
        else  {                                             // even - going downwards
          if (currentBrightness > minBrightness) currentBrightness = currentBrightness - 2;
          else currentBrightness = currentBrightness + 1;  // turn direction to upwards
        }
        LedBase<T>::obj.pwmWrite(currentBrightness);
      }
    }    
    
    /**
   \brief check if update for pulse is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/ 
    void pulse(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state != 0) {
        if (LedBase<T>::state == 1) {
          if (currentMillis - previousMillis >= interval[0]) {
            LedBase<T>::obj.digWrite(LOW);
            LedBase<T>::state = 0;
            if(LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
          }
        }
      }
    }

/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void rhythm(uint32_t currentMillis = millis()) {
      // LedBase<T>::state   0 OFF, 1 ON, 2 OFF, 3 ON, 4 OFF, 
      if (LedBase<T>::state > 0) {     // state 0 is reserved to "switch off" the light
        if (currentMillis - previousMillis > interval[LedBase<T>::state - 1]) {
          if (LedBase<T>::state % 2 != 0) {      // all odd states are ON, at the end of an interval we switch to the oposite pin state
            LedBase<T>::obj.digWrite(LOW);
          }
          else {
            LedBase<T>::obj.digWrite(HIGH);
          }
          LedBase<T>::state++;
          if (LedBase<T>::state > lengthOfPattern) 
            LedBase<T>::state = 1;      // rollover
          previousMillis = currentMillis;
        }
      }
    }    
   
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void smooth(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state == 1 && currentBrightness < maxBrightness && currentMillis - previousMillis > interval[0]) {
        currentBrightness++;    
        LedBase<T>::obj.pwmWrite(currentBrightness);
        previousMillis = currentMillis;
      }
      else if (LedBase<T>::state == 1 && currentBrightness > maxBrightness && currentMillis - previousMillis > interval[1]) {
        currentBrightness--;
        LedBase<T>::obj.pwmWrite(currentBrightness);
        previousMillis = currentMillis;
      }
      else if (LedBase<T>::state == 0 && currentBrightness > 0 && currentMillis - previousMillis > interval[1]) {
        currentBrightness--;
        LedBase<T>::obj.pwmWrite(currentBrightness);
        previousMillis = currentMillis;
      }
    }    
    
};


/*  **************************************************
    The different styles of "blinking"
    ************************************************** */  

/**
   \brief alternate blinking of to LEDs in a specific rhythm
   
   two LEDs blinking alternating
   
   doesn't inherit from LedBase.
*/
template<class T>
class Alternating { 
  protected:
    static const uint8_t noOfPins = 2;
    uint32_t previousMillis = millis();          // last blink timestamp
    T &obj;
    uint16_t onInterval = 500;                   // milliseconds on time for the first LED = off time for the second LED
    uint16_t offInterval = 500;                  // milliseconds off time for the first LED = on time for the secon LED
    //bool active = HIGH;                        // is the pin active HIGH
    uint8_t state = 1;                           // 0 idle, 1 blinkA, 2blinkB
    using Callback = void (*)(uint8_t value);    // signature of a callback function
    Callback cbStateChange;                      // a callback function if the state changes

  public:
/*
   \brief alternate blinking of two LEDs in a specific rhythm
   
   \param obj a object to connect LEDs
   \param on   the on  time of pin 1 in ms (vice versa the off time of pin2). This parameter is optional.
   \param off  the off time of pin 1 in ms (vice versa the on  time of pin2). This parameter is optional.
*/
    Alternating(T &obj, uint16_t on = 500, uint16_t off = 500) :
      obj(obj),
      onInterval {on},
      offInterval {off}
    {}

/**
   \brief start hardware
   
   Will do the necessary steps to initialize the hardware pins.
   Call this function in your setup().
*/
    void begin() {
      obj.begin();
    }
    
    void setOnColor(uint32_t _onColor) {
      obj.setOnColor(_onColor);
    }
/**
   \brief set on/off times 
   
   Set the on intervals of both LEDs during runtime.
   \param onA the on time of pinA in ms (vice versa the off time of pinB).
   \param onB the on time of pinB in ms (vice versa the off time of pinB).
*/
    void setOnInterval(uint16_t onA, uint16_t onB) {
      onInterval = onA;
      offInterval = onB;
    }
    
/**
   \brief set on/off times 
   
   Set the interval during runtime.
   \param onA the on time of pinA and pinB in ms.
*/   
    void setOnInterval(uint16_t onA) {
      onInterval = onA;
      offInterval = onA;
    }
    
/** 
    \brief set the callback function onStateChange

    a callback function receives state changes from the effect 
    \param funcPtr the callback function
*/ 
    void setOnStateChange(Callback funcPtr) {
      cbStateChange = funcPtr;
    }
    
/**
   \brief switch output on
   
   Switch the output to on state.
*/
    void on() {
      uint8_t previousState = state;
      state = 1;
      if (cbStateChange && previousState != state) cbStateChange(state);
    }
 
/**
   \brief switch output off
   
   Switch the output to off state.
*/
    void off() {
      uint8_t previousState = state;
      state = 0;
      obj.digWrite(0, LOW);
      obj.digWrite(1, LOW);
      if (cbStateChange && previousState != state) cbStateChange(state);
    }

/**
   \brief switch between on or off state
*/
    void toggle() {
      if (state == 0) on(); else off();
      // callback is done by on() / off()
    } 

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//   void update() {
//     update(millis()); 
//   }
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/    
    void update(uint32_t currentMillis = millis()) {
      if (state) {
        //uint32_t currentMillis = millis();
        if (state == 1 && currentMillis - previousMillis >= onInterval) {
          previousMillis = currentMillis;        // save the last time you blinked the LED
          obj.digWrite(0, LOW);
          obj.digWrite(1, HIGH);
          state = 2;
          if (cbStateChange) cbStateChange(state);
        }
        else if (state == 2 && currentMillis - previousMillis >= offInterval) {
          previousMillis = currentMillis;        // save the last time you blinked the LED
          obj.digWrite(0, HIGH);
          obj.digWrite(1, LOW);
          state = 1;
          if (cbStateChange) cbStateChange(state);
        }
      }
      else {       // switch off both LEDs             MISSING tbd: check against previousState?
        obj.digWrite(0, LOW);
        obj.digWrite(1, LOW);
      }
    }
};

/**
   \brief blink a LED (an output)
   
   a class to blink a object
   uses a unified hw interface
*/
template<class T>
class Blink : public LedBase <T> {
  protected:
    uint32_t previousMillis = millis();          // time management
    uint16_t onInterval = 500;
    uint16_t offInterval = 500;
    // state 0 off, 1 Blink-OFF, 2 Blink-ON

  public:
    Blink(T &obj) : LedBase<T>(obj) {}
 
/**
   \brief set on interval
   
   Set a new interval / time for how long the LED should be on.
   \param newInterval new interval in milliseconds
*/ 
    void setOnInterval(uint16_t newInterval) {
      onInterval = newInterval;
    }

/**
   \brief set off interval
   
   Set a new interval / time for how long the LED should be off.
   \param newInterval new interval in milliseconds
*/     
    void setOffInterval(uint16_t newInterval) {
      offInterval = newInterval;
    }
    
/**
   \brief switch output off
   
   Switch the effect to off state.
*/
    void off() override {
      uint8_t previousState = LedBase<T>::state;     
      LedBase<T>::state = 0;
      LedBase<T>::obj.digWrite(LOW);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
    
/**
   \brief switch on
   
   If effect is off, switch the output on.
*/  
    void on() override {
      uint8_t previousState = LedBase<T>::state; 
      if (LedBase<T>::state == 0) 
        LedBase<T>::state = 1; 
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);      
    }
    
/**
   \brief switch on
   
   If effect is off, switch the output on.
   \param force set to true if new state must be forced to the first ON state.
                set to false if only the OFF state should be switched ON (default).
*/  
    void on(bool force) {
      uint8_t previousState = LedBase<T>::state; 
      if (force) 
        LedBase<T>::state = 1;
      else if (LedBase<T>::state == 0) 
        LedBase<T>::state = 1;
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);            
    }
  
/**
   \brief switch between on or off state
   
   If the LED is on - switch it off.
   If the LED is off - switch it on.
*/
    void toggle() override {
      if (LedBase<T>::state == 0) LedBase<T>::on(); else off();
      // callback is done by on() and off()
    }  

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//      update(millis());
//    }
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state != 0) {
        //uint32_t currentMillis = millis();
        if (LedBase<T>::state == 2) {
          if (currentMillis - previousMillis >= onInterval) {
            LedBase<T>::state = 1;
            LedBase<T>::obj.digWrite(LOW);
            previousMillis = currentMillis;
            if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
          }
        }
        else {
          if (currentMillis - previousMillis >= offInterval) {
            LedBase<T>::state = 2;
            LedBase<T>::obj.digWrite(HIGH);
            previousMillis = currentMillis;
            if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);            
          }
        }
      }
    }
};

/**
   \brief Bounce 5 LEDs between left and right
   
   running lights like a KITT/Larson Scanner
   states are: 0 off; 1 run and on, 2 run but off
   
   doesn't inherit from LedBase
*/
template<class T>
class Bounce5 {  
    unsigned long previousMillis;                // last blink timestamp
    uint16_t onInterval = 200;                   // milliseconds on time for the LEDs
    uint16_t offInterval = 20;                   // milliseconds off time for the LEDs
    uint8_t current = 0;                         // current position in pattern
    T &obj;
    uint8_t state = 2;
    using Callback = void (*)(uint8_t value);    // signature of a callback function
    Callback cbStateChange;                      // a callback function if the state changes
 

  public:
/**
   \brief Bounce 5 LEDs between left and right
   
   \param obj a object with 5 LEDs
*/
    Bounce5(T &obj) : obj(obj) {}
    
/**
   \brief start hardware
   
   Will do the necessary steps to initialize the hardware pins.
   Call this function in your setup().
*/
    void begin() {
      obj.begin();
    }
    
// modify on/off times during runtime
    void setOffInterval(uint16_t _off) {         
      offInterval = _off;
    }
    
    // modify on/off times during runtime
    void setOnInterval(uint16_t _on) {           
      onInterval = _on;
    }
    
    void setOnColor(uint32_t _onColor) {
      obj.setOnColor(_onColor);
    }

/** 
    \brief set the callback function onStateChange

    a callback function receives state changes from the car traffic light
    \param funcPtr the callback function
    @todo clearify usage of this vs onSequnceChange
*/ 
  
    void setOnStateChange(Callback funcPtr) {
      cbStateChange = funcPtr;
    }
    
/**
   \brief switch output on
   
    Switch the output to on state.
*/
    void on() {
      uint8_t previousState = state;      
      state = 1;
      if (cbStateChange && previousState != state) cbStateChange(state);
    }

/**
   \brief switch output off
   
   Switch the output to off state.
*/
    void off() {
      uint8_t previousState = state;      
      state = 0;
      for (size_t i = 0; i < 5; i++)
      {
        obj.digWrite(i, LOW);
      }
      if (cbStateChange && previousState != state) cbStateChange(state);
    }
/**
   \brief switch between on or off state
*/
    void toggle() {
      if (state == 0) on(); else off();
    }

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//     update(millis()); 
//    }
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (state > 0) {
        const uint8_t ledPattern[] = {0, 1, 2, 3, 4, 3, 2, 1};       // the order how to light the LEDs
        const uint8_t totalPatterns = sizeof(ledPattern) / sizeof(ledPattern[0]);
        uint8_t actualLed = ledPattern[current];

        if (state == 1 && currentMillis - previousMillis >= onInterval) {
          // time to switch off
          previousMillis = currentMillis;
          for (size_t i = 0; i < 5; i++)
          {
            obj.digWrite(i, LOW);
          }
          state = 2;
          // no callback as states change from 1 to 2
        }
        else if (state == 2 && currentMillis - previousMillis >= offInterval) {
          // time to switch on next LED
          previousMillis = currentMillis;
          obj.digWrite(actualLed, HIGH);
          current++;
          if (current >= totalPatterns) current = 0;
          state = 1;
          // no callback as states change from 1 to 2
        }
      }
      else {       // switch off all LEDs  MISSING: tbd do only when previousState wasn't 0
        for (size_t i = 0; i < 5; i++)
        {
          obj.digWrite(i, LOW);
        }
      }
    }
};

/**
   \brief simulate a flickering light like a fire
   
   state 0 off, 1 flicker_on
*/
template<class T>
class Flicker : public LedBase<T> {
  protected:
    uint32_t previousMillis = millis();// time management
    uint8_t interval = 100;
    uint16_t maxBrightness = 255;      // native PWM on Arduino is only 8 bit. But could be used for other processors also.

  public:
/**
   \brief simulate a flickering light like a fire
   
   \param obj the pin to connect
*/
    Flicker (T &obj) : LedBase<T>(obj) {}

/**
   \brief switch off
*/     
    void off() override {
      uint8_t previousState = LedBase<T>::state;
      LedBase<T>::state = 0;
      LedBase<T>::obj.pwmWrite(0);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
    
/**
   \brief set the maximum brightness
   
   The output will dimm up to this maximum level.
   \param _maxBrightness the maximum brigthness (upper end of range) [0..255]
*/
    void setMaxBrightness(uint8_t _maxBrightness) {
      maxBrightness = _maxBrightness;
    }    
    
/**
   \brief switch between on or off state
*/
    void toggle() {
      if (LedBase<T>::state == 0) LedBase<T>::on(); else off();
    }

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//     update(millis()); 
//    } 

/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state == 1) {                                     // flicker in low wind
        if (currentMillis - previousMillis > interval) {
          //uint8_t value = (25 - random(22)) * 10 + 5;
          int value = (random(maxBrightness) / 10) * 10 + 5;
          LedBase<T>::obj.pwmWrite(value);
          interval = random(20, 150);
          previousMillis = currentMillis;
        }
      }
    }
};

/**
  \brief Simulation of a fluorescent lamp or fluorescent tube

  Turns on and off light emitting diodes (LED) connected to a digital pin,
  without using the delay() function. This means that other code can run at the
  same time without being interrupted by the LED code.
*/
template<class T>
class Fluorescent : public LedBase<T> {
    uint32_t previousMillis;           // timestamp for states
    uint32_t previousMillisEffect;     // timestamp for the effect
    //uint8_t state = 1;                  // 0 OFF, 1 START, 2 RUNNING, 3 FULL
    uint16_t interval  = 10;           // how fast should the led be dimmed (=milliseconds between steps)
    uint16_t intervalEffect = 10;      // an interval for the flicker effect
    uint8_t actual = 0;                // actual PWM
    uint16_t startTimeMin = 500;       // how long will it take from off to stable on; "lower" faster
    uint16_t startTimeMax = 5000;

  public:
/**
   \brief simulate a fluroescent lamp or flurescent tube
   
   \param obj the pin to connect
*/
    Fluorescent(T &obj) : LedBase<T>(obj) {}
    
/**
   \brief get the current brightness
   
   The current brightness/level of this output
   @return the current brigthness
*/    
    uint16_t getCurrentBrightness() {
      return actual;
    }

/**
   \brief switch on
*/ 
    void on() override  {
      uint8_t previousState = LedBase<T>::state;
      LedBase<T>::state = 1;                     // State::START;
      previousMillis = millis();
      LedBase<T>::obj.pwmWrite(2);               // "glimm" after start
      intervalEffect = random(50, 500);          // modify the first interval
      interval = random(startTimeMin, startTimeMax);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }

/**
   \brief switch off
*/ 
    void off() override {
      uint8_t previousState = LedBase<T>::state;
      LedBase<T>::state = 0;                     // State::OFF;
      actual = 0;
      LedBase<T>::obj.pwmWrite(0);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
    
/**
   \brief switch between on or off state
*/
    void toggle() override {
      if (LedBase<T>::state == 0) on(); else off();
    }

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//     update(millis()); 
//    } 

/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state == 1) {
        if (currentMillis - previousMillisEffect > intervalEffect) {
          if (actual >= 200) {                   // alles ab diesem Wert ist "ein" daher müssen wir nun "aus" Schalten
            actual = random(0, 5);               // Ein leichtes Glimmen der "Enden" ... könnte man auch ganz auf 0 setzen
            intervalEffect = random(400, 2000);  // unregeläßige Dunkelphasen (zwischen dem Aufblitzen)
          }
          else {
            actual = random(200, 255);
            intervalEffect = random(20, 40);     // flash shortly
          }
          LedBase<T>::obj.pwmWrite(actual);
          previousMillisEffect = currentMillis;
        }
        if (currentMillis - previousMillis > interval) {
          actual = 200;      // after the tube is stable "on" it will take some time to reach 100%
          interval = 100;    // we need 55 steps to get full 255 PWM = aprox 55 Seconds till the tube has full brightness
          LedBase<T>::obj.pwmWrite(actual);
          previousMillis = currentMillis;
          LedBase<T>::state = 2;
          if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
        }
      }
      if (LedBase<T>::state == 2) {
        uint32_t currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
          previousMillis = currentMillis;
          actual++;
          LedBase<T>::obj.pwmWrite(actual);
          if (actual >= 255)
          {
            LedBase<T>::state = 3;
            if (LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
          }
        }
      }
    }
};

/**
   \brief heart beat - dims up and down permanentely
   
   The output will dimm up and down. You can define threashold for min and max dim level. 
*/
template<class T>
class Heartbeat : public LedBase<T> {
  protected :
    //uint8_t state = 1; 
    uint8_t interval = 25;
    uint32_t previousMillis = millis();// time management
    uint8_t pwm = 0;                   // the current brightness
    uint8_t start = 0;                 // minimum brightness
    uint8_t end =  255;                // maximum brigthness

  public :
/**
   \brief heart beat LED
   
   \param obj a obj representing a LED
*/
    Heartbeat (T &obj) : LedBase<T>(obj) {}

    void setInterval(uint8_t newInterval) 
    {
      interval = newInterval;
    }
    
/**
   \brief get the current brightness
   
   The current brightness/level of this output
   @return the current brigthness
*/    
    uint16_t getCurrentBrightness() {
      return pwm;
    }    

/**
   \brief set the current brightness
   
   Set a new current brightness/level of this output
   @param brightness the new current brigthness
*/    
    void setCurrentBrightness(uint8_t brightness) {
      pwm = brightness;
    }  

    
/**
   \brief set the maximum brightness
   
   The output will dimm up to this maximum level.
   \param maxBrightness the maximum brigthness (upper end of range) [0..255]
*/
    void setMaxBrightness( uint8_t maxBrightness) {
      if (maxBrightness > start) {
        end = maxBrightness;
      }
    }
    
/**
   \brief set the minium brightness
   
   The output will dimm down to this minium level.
   \param minBrightness the minium brigthness (lower end of range) [0..255]
*/    
    void setMinBrightness( uint8_t minBrightness) {
      if (minBrightness < end) {
        start = minBrightness;
      }
    }
    
    // here we need to take care, that these on/off are used.
/**
   \brief switch on
*/  
    void on() override {
      uint8_t previousState = LedBase<T>::state;
      pwm = start;
      LedBase<T>::state = 1;
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }

/**
   \brief switch off
*/     
    void off() override {
      uint8_t previousState = LedBase<T>::state;
      LedBase<T>::obj.pwmWrite(0);
      pwm = start;           // if configured as "allways dimmed on" we have to set the start value
      LedBase<T>::state = 0;
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
    
/**
   \brief switch between on or off state
*/
    void toggle() {
      if (LedBase<T>::state == 0) on(); else off();
    }

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//     update(millis()); 
//    }    

/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/ 
    void update(uint32_t currentMillis = millis()) {
      if (currentMillis - previousMillis > interval && LedBase<T>::state == 1) {
        previousMillis = currentMillis;
        if (pwm % 2) {                           // odd - going upwards
          if (pwm < end - 1 ) pwm = pwm + 2;
          else pwm = pwm - 1;                    // turn direction to downwards
        }
        else {                                   // even - goint downwards
          if (pwm > start) pwm = pwm - 2;
          else pwm = pwm + 1;                    // turn direction to upwards
        }
        LedBase<T>::obj.pwmWrite(pwm);
      }
    }
};

/**
   \brief OnOff
   
   This class provides a simple on/off interface for an output.
   There is no effect during runtime.
   The class just provides access to hardware with an unified interface.
*/
// rename to "Output" and also add PWM???
template<class T>
class OnOff : public LedBase <T> {
    // state 0 = off; on = 255 or any other value = PWM
  public:
    OnOff(T &obj) : LedBase<T>(obj) {}
    
/**
   \brief switch output off
   
   Switch the output to off state.
*/
    void off() override {
      uint8_t previousState = LedBase<T>::state;      
      LedBase<T>::state = 0;
      LedBase<T>::obj.digWrite(LOW);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
    
/**
   \brief switch output off
   
   Switch the output to off state.
*/
    void on() override {
      uint8_t previousState = LedBase<T>::state;      
      LedBase<T>::state = 255;
      LedBase<T>::obj.digWrite(HIGH);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }

/**
   \brief get the current brightness
   
   The current brightness/level of this output
   @return the current brigthness
*/    
    uint16_t getCurrentBrightness() {
      return LedBase<T>::state;
    }
    
/**
   \brief set the maximum brightness
   
   The output will output this maximum level.
   \param newBrightness the new brigthness [0..255]
*/
    void setMaxBrightness(uint8_t newBrightness) {   
      LedBase<T>::state = newBrightness;
      LedBase<T>::obj.pwmWrite(newBrightness);
    }
    
/**
   \brief switch between on or off state
*/
    void toggle() {
      if (LedBase<T>::state == 0) on(); else off();
    }  

/*
    the update() function has "nothing" to do as the pin can only be switched on or off.
    Kept for compatibily only.
*/
//   void update() {}
    void update(uint32_t currentMillis = millis()) {(void)currentMillis;}
};

/**
  \brief pulse an output for a period of time and than switches off

  Turns on a LED for a defined period of time without using the delay() function (monoflop). 
  This means that other code can run at the same time without being interrupted by the LED code.
  The output acts like a monoflop.
*/
template<class T>
class Pulse : public LedBase<T> {
  protected:
    uint32_t previousMillis;           // last blink timestamp
    uint32_t onInterval = 500;         // how many milliseconds is the LED on
    //uint8_t state = 0;                  // 0 OFF, 1 ON
    public:
/**
   \brief pulse an output for a period of time and than switches of
   
   \param obj the pin to connect
*/  
    Pulse(T &obj) : LedBase<T>(obj) {
      LedBase<T>::state = 0;           // pulse should not start on startup
    }     

/**
   \brief set interval times 
   
   Set the on interval of the LED during runtime.
   \param _onInterval the new on interval
*/
    void setOnInterval(uint32_t _onInterval) {
      onInterval = _onInterval;
    }

/**
   \brief switch output off
   
   Switch the output to off state.
*/
    void off() override {
      uint8_t previousState = LedBase<T>::state;    // remember current state    
      LedBase<T>::state = 0;
      LedBase<T>::obj.digWrite(LOW);
      previousMillis = millis();
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
    
    void on() override {
      uint8_t previousState = LedBase<T>::state;    // remember current state
      LedBase<T>::state = 1;
      LedBase<T>::obj.digWrite(HIGH);
      previousMillis = millis();
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }

/**
   \brief switch between on or off state
*/
    void toggle() {
      if (LedBase<T>::state == 0) on(); else off();
      // callback is done by on() / off()
    }
    
/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//   void update() {
//     update(millis());
//   }

/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/ 
    void update(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state != 0) {
        if (LedBase<T>::state == 1) {
          if (currentMillis - previousMillis >= onInterval) 
          {
            LedBase<T>::obj.digWrite(LOW);
            LedBase<T>::state = 0;
            if(LedBase<T>::cbStateChange) LedBase<T>::cbStateChange(LedBase<T>::state);
          }
        }
      }
    }
};

/**
   \brief show a specific rhythm.
   
   blinks a LED in a specific rhythm.
   Supports 1, 2 and 4 pairs of on/off times.
   This can be used for police cars, ambulances and similar vehicles.
   default sequence is ECE2 (150, 60, 20, 270). 
   But you can set any other sequence also (ECE1, HELLA 3, HELLA 4,...)
*/
template<class T>
class Rhythm : public LedBase<T> {
  protected:
    //uint8_t state = 1;               // 0 off; 1 on 
    uint32_t previousMillis;           // time management
    uint8_t current = 0;               // current position in pattern
    uint8_t lengthOfPattern = 4;       // we have blink patterns with 2, 4, 6 or 8 times
    // the interval pattern are always in pairs of ON and OFF
    //                        ON  OFF  ON  OFF
    //                        1    2    3   4
    uint16_t interval[8] = {150,  60,  20,  270};                // ECE 2
    //uint16_t interval[8] = {180, 320};                         // ECE 1
    //uint16_t interval[8] = {25, 25, 25, 25, 25, 375};          // HELLA 3
    //uint16_t interval[8] = {40, 40, 40, 40, 40, 40, 40, 220 }; // HELLA 4
    
  public:
/**
   \brief blink a specific rhythm 
   
   \param obj the object to connect
*/
    //Rhythm(T &obj) : obj(obj){}
    Rhythm(T &obj) : LedBase<T>(obj) {}

    // this functions takes either 2, 4, 6 or 8 parameters
/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on interval 
   \param _interval1 the off interval
*/
    void setInterval(uint16_t _interval0, uint16_t _interval1) {     
      interval[0] = _interval0;
      interval[1] = _interval1;
      lengthOfPattern = 2;
    }
/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on  interval 
   \param _interval1 the off interval
   \param _interval2 the on  interval
   \param _interval3 the off interval
*/
    void setInterval(uint16_t _interval0, uint16_t _interval1, uint16_t _interval2, uint16_t _interval3) {
      interval[0] = _interval0;
      interval[1] = _interval1;
      interval[2] = _interval2;
      interval[3] = _interval3;
      lengthOfPattern = 4;
    }
/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on  interval 
   \param _interval1 the off interval
   \param _interval2 the on  interval
   \param _interval3 the off interval
   \param _interval4 the on  interval
   \param _interval5 the off interval
*/   
    void setInterval(uint16_t _interval0, uint16_t _interval1, uint16_t _interval2, uint16_t _interval3, uint16_t _interval4, uint16_t _interval5) {
      interval[0] = _interval0;
      interval[1] = _interval1;
      interval[2] = _interval2;
      interval[3] = _interval3;
      interval[4] = _interval4;
      interval[5] = _interval5;
      lengthOfPattern = 6;
    }

/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _interval0 the on  interval
   \param _interval1 the off interval
   \param _interval2 the on  interval
   \param _interval3 the off interval
   \param _interval4 the on  interval
   \param _interval5 the off interval
   \param _interval6 the on  interval
   \param _interval7 the off interval
*/
    void setInterval(uint16_t _interval0, uint16_t _interval1, uint16_t _interval2, uint16_t _interval3, uint16_t _interval4, uint16_t _interval5, uint16_t _interval6, uint16_t _interval7) {
      interval[0] = _interval0;
      interval[1] = _interval1;
      interval[2] = _interval2;
      interval[3] = _interval3;
      interval[4] = _interval4;
      interval[5] = _interval5;
      interval[6] = _interval6;
      interval[7] = _interval7;
      lengthOfPattern = 8;      
    }
    
/**
   \brief switch output off
   
   Switch the output to off state.
*/
    void off() override {   
      uint8_t previousState = LedBase<T>::state;
      LedBase<T>::state = 0;
      LedBase<T>::obj.digWrite(LOW);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }
/**
   \brief switch between on or off state
*/
    void toggle() {
      if (LedBase<T>::state == 0) LedBase<T>::on(); else off();
    }

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//      update(millis());
//    }
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state > 0)  {              // state 0 is reserved to "switch off" the light
        if (currentMillis - previousMillis > interval[current]) {
          if (current % 2 == 0) {      // all even states are ON, at the end of an interval we switch to the oposite pin state
            LedBase<T>::obj.digWrite(LOW);
          }
          else {
            LedBase<T>::obj.digWrite(HIGH);
          }
          current++;
          if (current >= lengthOfPattern) current = 0;
          previousMillis = currentMillis;
        }
      }
    }
};

/**
   \brief dim up / down a LED dimms smoothly
   
   @note use a PWM pin for a nice effect  
*/
template<class T>
class Smooth : public LedBase<T> {
  protected:
    //uint8_t state = 1;               // state = 0 off or decrease, 1 on or still increasing
    uint32_t previousMillis = millis();// last timestamp
    uint16_t currentBrightness = 0;    // actual brightness of Pin
    uint16_t maxBrightness = 255;      // native PWM on Arduino is only 8 bit. But could be used for other processors also.
    uint8_t onInterval = 25;           // delay for each step upwards
    uint8_t offInterval = 15;          // delay for each step downwards

  public:
/**
   \brief dim up / down a LED dimms smoothly
   
   \param obj the pin to connect
*/
    Smooth(T &obj) : LedBase<T>(obj) {}
    
    
/**
   \brief force switch off
   
   switches off imidiatly without smooth effect
*/    
    void offForced() {
      uint8_t previousState = LedBase<T>::state;           // remember current state
      LedBase<T>::state = 0;
      currentBrightness = 0; 
      LedBase<T>::obj.pwmWrite(currentBrightness);
      if (LedBase<T>::cbStateChange && previousState != LedBase<T>::state) LedBase<T>::cbStateChange(LedBase<T>::state);
    }  
    
/**
   \brief get the current brightness
   
   The current brightness/level of this output
   @return the current brigthness
*/    
    uint16_t getCurrentBrightness() {
      return currentBrightness;
    }
    
/**
   \brief set the current brightness
   
   The current brightness/level of this output
*/    
    void getCurrentBrightness(uint16_t brightness) {
      currentBrightness = brightness;
      LedBase<T>::obj.pwmWrite(currentBrightness);
    }

/**
   \brief set the maximum brightness
   
   The output will dimm up to this maximum level.
   \param newValue the maximum brigthness (upper end of range) [on UNO 0..255]
*/
    void setMaxBrightness(uint16_t newValue) {
      maxBrightness = newValue;
    }

    void setOffInterval(uint8_t newValue) {
      offInterval = newValue;
    }
    
    void setOnInterval(uint8_t newValue) {
      onInterval = newValue;
    }
    
/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//   void update() {
//     update(millis());
//   }
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (LedBase<T>::state == 1 && currentBrightness < maxBrightness && currentMillis - previousMillis > onInterval) {
        currentBrightness++;    
        LedBase<T>::obj.pwmWrite(currentBrightness);
        previousMillis = currentMillis;
      }
      else if (LedBase<T>::state == 1 && currentBrightness > maxBrightness && currentMillis - previousMillis > offInterval) {
        currentBrightness--;
        LedBase<T>::obj.pwmWrite(currentBrightness);
        previousMillis = currentMillis;
      }
      else if (LedBase<T>::state == 0 && currentBrightness > 0 && currentMillis - previousMillis > offInterval) {
        currentBrightness--;
        LedBase<T>::obj.pwmWrite(currentBrightness);
        previousMillis = currentMillis;
      }
    }
};

/**
   \brief traffic light
   
   state // 0 OFF, 1 RED, 2 REDYELLOW, 3 GREEN, 4 YELLOW, 5 YELLOWBLINK, 6 GREENBLINK
   
   mode  // 0 manual // 1 automatic   // 2 redgreenonly / 3 automaticAT
   
   interval as array
   sequence (order, interval)  
   doesn't inherit from LedBase   
*/
// notepad++ don't close this class
template<class T>
class Trafficlight {
  protected:
    T &obj;                                      // needs an object with 3 (or 2) LEDs
    uint32_t previousMillis = millis();          // last state timestamp
    uint32_t previousMillisBlink = millis();     // last blink timestamp
    uint16_t onInterval = 500;                   // milliseconds on time for the first LED = off time for the second LED
    uint8_t currentBlink = 0;                    // current position in pattern (on or off)
    uint8_t currentSequence = 0;
    uint8_t state = 5;
    uint8_t mode = Mode::AUTOMATIC;
    using Callback = void (*)(uint8_t value);
    Callback cbStateChange, cbSequenceChange;
    struct Sequence {
      uint8_t state;
      uint16_t interval;
    } sequence[8];
    uint8_t noOfSequence = 4;

  public:
/**
   \brief constructor for a traffic light
   
   \param obj a object with 3 lamps 
*/
    Trafficlight(T &obj) : obj(obj) {
      sequence[0].state = State::RED;
      sequence[0].interval = 5000;
      sequence[1].state = State::REDYELLOW;
      sequence[1].interval = 3000;
      sequence[2].state = State::GREEN;
      sequence[2].interval = 2500;
      sequence[3].state = State::YELLOW;
      sequence[3].interval = 3000;
      //sequence[3].state = State::GREENBLINK;
      //sequence[3].interval = 2500;
    }
    
    // the faces of the traffic light
    //          0   1     2          3       4       5             6
    enum State{OFF, RED, REDYELLOW, GREEN, YELLOW, YELLOWBLINK, GREENBLINK};
    enum Mode{MANUAL, AUTOMATIC, /*REDGREEN, AUTOMATIC_AT */}; 
    
/**
   \brief start hardware
   
   Will do the necessary steps to initialize the hardware pins.
   Call this function in your setup().
*/
    void begin() {
      obj.begin();
    }

/**
   \brief set on times 
   
   Set the on interval.
   \param _on the on interval 
*/    
    void setInterval(uint16_t _on) { // modify Blink interval on/off times during runtime
      onInterval = _on;
    }

/** 
    \brief set the sequence parameters

    By default there are 4 predefined sequences - you can define up to 8.
    You can modify each entry including the durance of each interval (in ms).
    For example: in Austria or Ukraine you can add (insert) a green blinking light sequnce
    
    \param index the number of the sequnce.
    \param newState the state (the face) of the sequence.
    \param newInterval the interval (durance) of the sequence in ms.
*/
    int setSequenceIndex(uint8_t index, uint8_t newState, uint16_t newInterval) {
      constexpr size_t maxSequence = sizeof(sequence)/sizeof(sequence[0]);
      if (index < maxSequence) {
        sequence[index].state = newState;
        sequence[index].interval = newInterval;
        return 0;  // success
      }
      return 1;    // error
    }
    
/** 
    \brief set the number of sequences 

    By default there are 4 predefined sequences - you can define up to 8.   
    \param newMax the maximum number of sequences
*/    
    int setSequenceMax(uint8_t newMax) {
      constexpr size_t maxSequence = sizeof(sequence) / sizeof(sequence[0]);
      if (newMax <= maxSequence && newMax > 0) {
        noOfSequence = newMax;
        return 0;  // success
      }
      return 1;    // error
    }
    
    void off() {
      state = 0;
      obj.digWrite(0, LOW);
      obj.digWrite(1, LOW);
      obj.digWrite(2, LOW);
    }

/** 
    \brief set the current Mode of the traffic light 

    default state is Trafficlight::Mode::AUTOMATIC, but you can change it to Trafficlight::Mode::AUTOMATIC 
    \param newMode set to Trafficlight::Mode::AUTOMATIC or Trafficlight::Mode::Manual
*/     
    void setMode(Mode newMode) {
       mode = newMode;    
    }

/** 
    \brief set the callback function onStateChange

    a callback function receives state changes from the car traffic light
    \param funcPtr the callback function
    @todo clearify usage of this vs onSequnceChange
*/ 
  
    void setOnStateChange(Callback funcPtr) {
      (*this).cbStateChange = funcPtr;
    }

/** 
    \brief set the callback function onSequnceChange

    a callback function receives changes of the sequence (faces).
    For example to keep another traffic light in sync, like a pedestrain trafficlight or the crossing line.
    \param funcPtr the callback function
*/   
    void setOnSequenceChange(Callback funcPtr) {
      (*this).cbSequenceChange = funcPtr;
    }

/** 
    \brief set the color for a traffic light

    \param actual 0 for red, 1 for yellow, 2 for green
    \param _on the color code in HEX
*/ 
    void setOnColor(uint16_t actual, uint32_t _on) {
      obj.setOnColor(actual, _on);
    }

/** 
    \brief set the color for a traffic light

    \param _off the color code in HEX
*/    
    void setOffColor(uint32_t _off) {
      obj.setOffColor(_off);
    }    
    
/** 
    \brief set the state of the traffic light

    \param newState the new state 
*/     
    void setState(uint8_t newState) {
      //Serial.print(F("newState=")); Serial.println(newState);
      switch(newState) {
        case State::OFF :
          obj.digWrite(0, LOW);
          obj.digWrite(1, LOW);
          obj.digWrite(2, LOW);
        break;
        case State::RED :
          obj.digWrite(0, HIGH);
          obj.digWrite(1, LOW);
          obj.digWrite(2, LOW);
        break; 
        case State::YELLOW :
          obj.digWrite(0, LOW);
          obj.digWrite(1, HIGH);
          obj.digWrite(2, LOW);
        break;
        case State::REDYELLOW :
          obj.digWrite(0, HIGH);
          obj.digWrite(1, HIGH);
          obj.digWrite(2, LOW);
        break;       
        case State::GREEN :
          obj.digWrite(0, LOW);
          obj.digWrite(1, LOW);
          obj.digWrite(2, HIGH);
        break;  
      }
      if(cbStateChange && state != newState) cbStateChange(newState);
      state = newState;
    }   
    
    void green() {
      setState(State::GREEN);
    }
    
    void greenBlink() {
      setState(State::GREENBLINK);
    }
    
    void red() {
      setState(State::RED);
    }
    
    void yellow() {
      setState(State::YELLOW);
    }
    
    void yellowBlink() {
      setState(State::YELLOWBLINK);
    }

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//      update(millis());
//    }
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (mode != Mode::MANUAL) {
        if (currentMillis - previousMillis >= sequence[currentSequence].interval) {
          previousMillis = currentMillis;
          currentSequence++;
          if (currentSequence >= noOfSequence) currentSequence = 0;
          if(cbSequenceChange) cbSequenceChange(currentSequence);
          setState(sequence[currentSequence].state);
        }
      }
      if (state == State::GREENBLINK) {
        if (currentMillis - previousMillisBlink >= onInterval) {
          previousMillisBlink = currentMillis;
          if (currentBlink) {
            obj.digWrite(2, LOW);
            currentBlink  = 0;
          }
          else {
            obj.digWrite(2, HIGH);
            currentBlink  = 1;
          }
        }
      }
      if (state == State::YELLOWBLINK) {
        if (currentMillis - previousMillisBlink >= onInterval) {
          previousMillisBlink = currentMillis;
          if (currentBlink) {
            obj.digWrite(1, LOW);
            currentBlink  = 0;
          }
          else {
            obj.digWrite(1, HIGH);
            currentBlink  = 1;
          }
        }
      }
    }
};

/**
    \brief turnsignals for a car
    
    this class needs 3 LEDs (left, right and a hazard warning light in the dashboard).
*/
template <class T>
class Turnsignal {
  protected:
    uint32_t previousMillis = millis();// last blink timestamp
    uint16_t onInterval = 500;         // milliseconds on time for the first LED = off time for the second LED
    uint16_t offInterval = 500;        // milliseconds off time for the first LED = on time for the secon LED
    uint8_t current = 0;               // current position in pattern (on or off)
    uint8_t state = 1;                 // 0 OFF, 1 LEFT, 2 RIGHT, 3 HAZARD
    T &obj;
    using Callback = void (*)(uint8_t value);
    Callback cbStateChange;
  public:
/**
   \brief car Turn signal
   
   This can be used for turning signals on a car. 
   Additionally you can define a hazard indicator on the dashboard which will blink in the rhythm of the turning signals.
   
   \param obj a object with 2 or 3 LEDs for turning signals
*/  
    Turnsignal(T &obj) : obj(obj) {}
    
/**
   \brief start hardware
   
   Will do the necessary steps to initialize the hardware pins.
   Call this function in your setup().
*/
    void begin() {
      obj.begin();
    }
    
/**
   \brief set on/off times 
   
   Set the on and off interval.
   \param _on the on interval 
   \param _off the off interval
*/   
    void setInterval(uint16_t _on, uint16_t _off) { // modify on/off times during runtime
      onInterval = _on;
      offInterval = _off;
    }

/**
   \brief set on/off color 
   
   Set the on and off colors.
   \param actual the LED number 0 left, 1 right, 2 hazard
   \param _on the on color
*/    
    void setOnColor(uint16_t actual, uint32_t _on) {
      obj.setOnColor(actual, _on);
    }
    
    void setOffColor(uint32_t _off) {
      obj.setOffColor(_off);
    }
    
    void off() {
      uint8_t previousState = state;
      state = 0;
      obj.digWrite(0, LOW);
      obj.digWrite(1, LOW);
      obj.digWrite(2, LOW);
      if (cbStateChange && previousState != state) cbStateChange(state);
    }
    
    void setState(uint8_t newState) {
      if (newState != state) off();
      state = newState;
      previousMillis = millis() - (onInterval + offInterval);
      current = 0;
      // the user should know, that he has called setState ... so no callback
    }

/**
   \brief left turning signal on
   
   Turn on the left signal.
*/    
    void left() {
      uint8_t previousState = state;
      setState(1);
      if (cbStateChange && previousState != state) cbStateChange(state);
    }

/**
   \brief right turning signal on
   
   Turn on the right signal.
*/     
    void right() {
      uint8_t previousState = state;
      setState(2);
      if (cbStateChange && previousState != state) cbStateChange(state);
    }

/**
   \brief hazard turning signal on
   
   will activate left and right turning signal and the hazarad light in the dashboard (if defined).
*/ 
    void hazard() {
      uint8_t previousState = state;
      setState(3);
      if (cbStateChange && previousState != state) cbStateChange(state);
    }

/** 
    \brief set the callback function onStateChange

    a callback function receives state changes from the effect 
    \param funcPtr the callback function
*/ 
    void setOnStateChange(Callback funcPtr) {
      cbStateChange = funcPtr;
    }    

/*
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   This function uses by default the actual millis() to determine, if an action is necessary.
*/     
//    void update() {
//      update(millis());
//    }
    
/**
   \brief check if update is necessary
   
   This is the "run" function. Call this function in loop() to make the effect visible.
   \param currentMillis you can handover a millis timestamp
*/     
    void update(uint32_t currentMillis = millis()) {
      if (state) {
        if (current == 0 && currentMillis - previousMillis >= offInterval) {
          previousMillis = currentMillis;        // save the last time you blinked the LED
          current = 1;
          switch(state) {
            case 1 : 
              obj.digWrite(0, HIGH); 
              break;
            case 2 : 
              obj.digWrite(1, HIGH); 
              break;
            case 3 : 
              obj.digWrite(0, HIGH);
              obj.digWrite(1, HIGH);
              obj.digWrite(2, HIGH);
              break;
          }
        }
        else if (current == 1 && currentMillis - previousMillis >= onInterval) {
          previousMillis = currentMillis;        // save the last time you blinked the LED
          current = 0;
          obj.digWrite(0, LOW);
          obj.digWrite(1, LOW);
          obj.digWrite(2, LOW);
        }
      }
    }
};
 
 /*
    for a short periode the HW layer for discrete pins will be included to make 
    existing sketch not breaking
    
    to be deleted 2023-07
*/
#include <utility/Noiasca_discrete.h>  // to be deleted 2023-07
