/*
  Noiasca Tool Kit - Timer and Buttons
  
  copyright 2023 noiasca noiasca@yahoo.com
  
  Version
  2023-07-10 0.3.1  initial previousMillis = millis()
  2023-05-01        curly breakets
  2022-12-17        update with optional millis()
  2022-07-27        renaming of parameters
  2022-01-16        updated comments for doxygen  
*/

#pragma once

/**
   \brief a simple timer
   
   The LittleTimer can be used to fire in a specific interval.
   Additionally you can limit the timer by a specific amount of intervals.
   If necessary you can define callback functions.
   
   @todo tbd methods for pause and restart
*/
class LittleTimer {
  protected:
    byte state = 1;                    // 0 off; 1 on; 2 paused (rfu)
    uint32_t previousMillis = millis();// time management
    using CallBack = void (*)(void);
    CallBack cbOnStart, cbOnInterval, cbOnStop;
    uint32_t interval = 1000;          // interval in ms
    uint32_t limit = 0;                // 0 = infinte (no limit)
    uint32_t iteration = 0;            // counts the current loops
    uint16_t missedIteration = 0;      // unfetched intervals
    bool ended = false;                // the iterations are ended

  public:
    LittleTimer(uint32_t interval, uint32_t limit = 0) :
      interval {interval}, limit{limit}
    {}

    LittleTimer(CallBack cbOnInterval, uint32_t interval, uint32_t limit = 0) :
      cbOnInterval (cbOnInterval), interval {interval}, limit{limit}
    {}
    
    // MISSING tbd: call it attach or set
    /**
       \brief attach callback for on start event
       
    */       
    void attachOnStart(const CallBack cbOnStart) {
      (*this).cbOnStart = cbOnStart;
    }
    /**
       \brief attach callback for on interval event
       
    */    
    void attachOnInterval(const CallBack cbOnInterval) {
      (*this).cbOnInterval = cbOnInterval;
    }
    /**
       \brief attach callback for on stop event
       
    */    
    void attachOnStop(const CallBack cbOnStop) {
      (*this).cbOnStop = cbOnStop;
    }  
    
    /**
       \brief get the state of the timer
       
       @return the current state of the timer
    */ 
    byte getState() {
      return state;
    }

   /**
       \brief set limit
       
       Set a new limit for the timer.
       
       @param limit how often should the timer fire
    */ 
    void setLimit(uint32_t limit) {
      this->limit = limit;
    }

    /**
       \brief set the iteration
       
       Modify the counter of iterations.
       
       @param iteration how often has the timer fired
    */
    void setIteration(uint32_t iteration) {
      this->iteration = iteration;
    }
    
    /**
       \brief set the interval
       
       Modify the interval of the timer.
       
       @param interval interval in milliseconds
    */
    void setInterval(uint32_t interval) {
      this->interval = interval;
    }    
    
    /**
       \brief start timer
       
       starts the timer.
    */  
    void start() {
      if (state != 1) {      // avoid "restart"
        state = 1;
        previousMillis = millis();
        if (cbOnStart) cbOnStart();
      }
    }
    
    /**
       \brief stop timer
       
       Stops the timer.
    */ 
    void stop() {     
      if (state != 0) {
        state = 0;
        if (cbOnStop) cbOnStop();
      }
    }
    

    /**
       \brief get the iteration
       
       Returns the current number of ioterations.
       
       @return interations of how often the timer has fired.
    */
    uint32_t getIteration() {
      return iteration;
    }
    
    /**
       \brief indicate if timer has been triggered
       
       @return > 0 if interval time is passed since last call. Returns the number of "missed" intervals.
    */ 
    uint16_t hasTriggered() {
      update();
      uint16_t result = missedIteration;
      missedIteration = 0;
      return result;
    }

    /**
       \brief indicate if timer has been ended
       
       @return true if interval time is over.
    */       
    bool hasEnded() {
      update();
      bool result = ended;
      ended = false;
      return result;
    }
    
/**
   \brief run 
   
   call this member function in your loop()
*/ 
    void update(uint32_t currentMillis = millis()) {
      if (state) {
        if (currentMillis - previousMillis >= interval) {
          iteration++;
          missedIteration++;
          ended = false;
          previousMillis = currentMillis; 
          if (limit == 0 || (limit > 0 && limit > iteration)) {      // running endless or limit not reached
            if (cbOnInterval) cbOnInterval();
          }
          else {                                                     // if there is a defined CB for the end, call it otherwise call the cb for interval the last time
            if (cbOnStop) 
              cbOnStop(); 
            else if (cbOnInterval) 
              cbOnInterval();
            state = 0;                 // stop the FSM 
            iteration = 0;             // reset iteration for next run
            missedIteration = 0;       // reset missed iterations for next run
            ended = true;
          }
        }
      }
    }
};


/** 
   \brief a very simple timer
   
   I just wanted a very simple minimalistic timer.
   But I propose to use the LittleTimer instead.
   The main difference is that this timere has no callback functions
   \see LittleTimer
*/
class MiniTimer {
  protected:
    byte state = 1;                    // 0 off; 1 on; 2 paused (rfu)
    uint32_t previousMillis = millis();// time management       
    uint32_t interval = 1000;          // interval in ms
    uint32_t limit = 0;                // 0 = infinte (no limit)
    uint32_t iteration = 0;            // counts the current intervals
    uint16_t missedIteration = 0;      // how many intervals where not requested by the sketch
    bool ended = false;                // all iterations are ended but the end was not requested so far

  public:
    MiniTimer(uint32_t interval, uint32_t limit = 0) :
      interval {interval}, limit{limit}
    {}

    /**
       \brief indicate if timer has been triggered
       
       @return > 0 if interval time is passed since last call. Returns the number of "missed" intervals.
    */ 
    uint16_t hasTriggered() {
      update();
      uint16_t result = missedIteration;
      missedIteration = 0;
      return result;
    }
    
    /**
       \brief indicate if timer has been ended
       
       @return true if interval time is over.
    */    
    bool hasEnded() {
      update();
      bool result = ended;
      ended = false;
      return result;
    }
    
    /**
       \brief start timer
       
       starts the timer.
    */ 
    void start() {
      if (state != 1) {      // avoid "restart"
        state = 1;
        previousMillis = millis();
      }
    }

    /**
       \brief stop timer
       
       stops the timer.
    */   
    void stop() {     
      if (state != 0) {
        state = 0;
      }
    }
    
   // and these are really just optional member functions
    /**
       \brief get the state of the timer
       
       @return the current state of the timer
    */ 
   byte getState() {
      return state;
    }

    /**
       \brief get the limit of the timer
       
       @param limit how often should the timer fire
    */     
    void setLimit(uint32_t limit) {
      this->limit = limit;
    }

    /**
       \brief set the iteration
       
       Modify the counter of iterations.
       
       @param iteration how often has the timer fired
    */
    void setIteration(uint32_t iteration) {
      this->iteration = iteration;
    }

    /**
       \brief get the iteration
       
       Returns the current number of ioterations.
       
       @return interations of how often the timer has fired.
    */
    uint32_t getIteration() {
      return iteration;
    }

    /**
       \brief run method
       
       call this function in your loop()
    */ 
    void update(uint32_t currentMillis = millis()) {
      if (state) {
        if (currentMillis - previousMillis >= interval) {
          iteration++;
          missedIteration++;
          ended = false;
          previousMillis = currentMillis; 
          if (limit > 0 && iteration >= limit) { // limit reached
            state = 0;                           // stop the FSM 
            iteration = 0;                       // reset iteration for next run
            missedIteration = 0;                 // reset missed iterations for next run
            ended = true;
          }
        }
      }
    }    
};

/*! Some words to the Noiasca Tool Kit
  
  \section timer2_sec Timer
  There are two timer classes which can be used for simple tasks by time.

*/
