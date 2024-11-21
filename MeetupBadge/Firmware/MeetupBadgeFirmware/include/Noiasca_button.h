/*
  Noiasca Tool Kit - Timer and Buttons
  
  copyright 2023 noiasca noiasca@yahoo.com
  
  Version
  2023-07-10 0.3.1  initial previousMillis = millis(), time mangement on 16bit
  2022-12-06        added callbacks for onPress and onRelease
  2022-04-09        added getCurrentState and wasReleased
  2022-02-18        support active LOW and active HIGH
  2022-01-16        updated comments for doxygen  
*/

#pragma once

/**
    \brief read a button
    
    This is just a simple class to read button presses.
    The button will be debounced by software.
    You can replace this implementation with any other library.    
*/
class Button {                                   // a simple class for buttons based on the "state change detection" example
  protected:
    static constexpr uint8_t debounceDelay = 50; // the debounce time; increase if the output flickers
    const uint8_t buttonPin;                     // a GPIO for the button/switch
    const bool active;                           // is the pin active HIGH or active LOW (will also activate the pullups!)
    bool lastButtonState = HIGH;                 // the previous LOGICAL reading from the input pin
    uint16_t lastDebounceTime = millis() & 0xFFFF;         // the last time the output pin was toggled - we check only ONE byte, so I didn't mess around with unsigned long
    void (*cbOnPress)();                         // gets called if button was pressed "rising"
    void (*cbOnRelease)();                       // gets called if button was released "falling"
    
  public:
/**
    \brief constructor for a button
    
    The constructor takes the GPIO as parameter.
    If you omit the second parameter, the library will activate the internal pullup resistor
    and the button should connect to GND.
    If you set the second parameter to HIGH, the button is active HIGH. The button should 
    connect to VCC. The internal pullups will not be used.
    
    \param attachTo the GPIO for the button
    \param active LOW (default) - if button connects to GND, HIGH if button connects to VCC
*/
    Button(byte attachTo, bool active = LOW) : buttonPin(attachTo), active(active) {}

/**
    \brief set the pin to the proper state
    
    Call this function in your setup(). 
    The pinMode will be set according to your constructor.   
*/
    void begin() 
    {
      if (active == LOW)
        pinMode(buttonPin, INPUT_PULLUP);
      else
        pinMode(buttonPin, INPUT);
      if (digitalRead(buttonPin) == active) lastButtonState = HIGH;  // init variable in case of a permanent/latching switch
    }

/**
    \brief read if button is pressed
    
    This is similar to a native "digitalRead" but the returned logic depends on 
    the active status during creation of the the instance.
    If the button is active LOW (default), 
    this member function will return true if the button is closed to GND.
    
    @return true if button is currently pressed (without any debounce logic)
*/    
    bool isPressed() {
      bool result = false;
      if (digitalRead(buttonPin) == active) result = HIGH;
      return result;
    }    

/**
    \brief set the callback for on press events ("raising")
   
    @param cbOnPress the callback function to be called
*/    
    void setOnPress(void( *cbOnPress)()) {       // set callback function for on press
      (*this).cbOnPress = cbOnPress;
    }

/**
    \brief set the callback for on release events ("falling")
   
    @param cbOnRelease the callback function to be called
*/     
    void setOnRelease(void( *cbOnRelease)()) {   // set callback function for on release
      (*this).cbOnRelease = cbOnRelease;
    }
 
 
/**
    \brief indicate if button was pressed since last call
    
    @return true if button was pressed since last call - debounce
    @param currentMillis a millis() timestamp. Optional. If omitted actual millis() will be used
*/
    bool wasPressed(uint32_t currentMillis = millis()) {
      bool result = false;
      uint8_t reading = LOW;                                         // the current reading from the input pin
      if (digitalRead(buttonPin) == active) reading = HIGH;          // if we are using INPUT_PULLUP we are checking invers to LOW Pin
      if (((currentMillis & 0xFFFF ) - lastDebounceTime) > debounceDelay) {      // If the switch changed, AFTER any pressing or noise
        if (reading != lastButtonState && lastButtonState == LOW) {  // if there was a change and last state was LOW
          result = true;
          lastDebounceTime = currentMillis & 0xFFFF;
        }
        lastButtonState = reading;
      }
      return result;
    }
    
/**
    \brief indicate if button was released since last call
    
    @return TRUE if button was released since last call - debounce
    @param currentMillis a millis() timestamp. Optional. If omitted actual millis() will be used.
*/
    bool wasReleased(uint32_t currentMillis = millis()) {
      bool result = false;
      uint8_t reading = HIGH;                                        // the current reading from the input pin
      if (digitalRead(buttonPin) != active) reading = LOW;
      if (((currentMillis & 0xFFFF ) - lastDebounceTime) > debounceDelay) {    // If the switch changed, AFTER any pressing or noise
        if (reading != lastButtonState && lastButtonState == HIGH) { // if there was a change and last state was HIGH
          result = true;
          lastDebounceTime = currentMillis & 0xFFFF;
        }
        lastButtonState = reading;
      }
      return result;
    }

/**
    \brief run member function
    
    This is the "run" function. 
    Call this function in loop() to read the button. 
    This function uses by default the actual millis() to determine, if callback functions needs to be called. 
    
    @param currentMillis a millis() timestamp. Optional. If omitted actual millis() will be used.
*/    
    void update(uint32_t currentMillis = millis()) {                 // call the run function in loop()
      uint8_t reading = LOW;                                         // "translated" state of button LOW = released, HIGH = pressed, despite the electrical state of the input pint
      if (digitalRead(buttonPin) == active) reading = HIGH;          // if we are using INPUT_PULLUP we are checking invers to LOW Pin
      if ((currentMillis & 0xFF) - lastDebounceTime > debounceDelay) {         // If the switch changed, AFTER any pressing or noise
        lastDebounceTime = currentMillis & 0xFF;
        if (reading == LOW && lastButtonState == HIGH) {
          if (cbOnRelease) cbOnRelease();
        }
        else if (reading == HIGH && lastButtonState == LOW) {
          if (cbOnPress) cbOnPress();
        }
        lastButtonState = reading;
      }
    }    
};

/*! Some words to the Noiasca Tool Kit
 

  \section button2_sec Button Class
  This class can be used to read a button.
  
*/
