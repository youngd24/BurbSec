/*
  Noiasca Tool Kit - Timer and Buttons
  
  copyright 2022 noiasca noiasca@yahoo.com
  
  Version
  2022-01-16 updated comments for doxygen  
*/

#pragma once

#pragma message ("Warning: Don't include Noiasca_tool.h. Use Noiasca_button.h and Noiasca_timer.h instead" )
/*
   MISSING change to alert in 2023
   \todo
*/
#include <Noiasca_button.h>
#include <Noiasca_timer.h>

/*! \mainpage Some words to the Noiasca Tool Kit
 
  \section intro_sec Introduction
  The "Noiasca Tool Kit" library is a collection of serveral tools to handle pins, relais and LEDs in a typical beginner project. 
  
  \subsection led_sec Pins to control LEDs and Relais
  This classes can be used to blink, pulse, heartbeat, LEDs in different pattern.
  
  \subsection timer_sec Timer
  There are two timer classes which can be used for simple tasks by time.
  
  \subsection button_sec Button
  There is one button class can be used to read and debounce a momementary push button.
   
  \section example_sec Examples
  There are several examples please use the hello world for the beginning.
  
  \section install_sec Install the library
  Download the library from https://werner.rothschopf.net/microcontroller/202202_tools_led_en.htm
  
  In the Arduino IDE use the Menu <br>
   Sketch / Include Library / Add .ZIP Library <br>
  to install the library.
 */
 