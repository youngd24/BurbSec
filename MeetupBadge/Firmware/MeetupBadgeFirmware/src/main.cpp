// ============================================================================
//
// BurbSec MeetupBadge Firmware
//
// main.cpp
//
// Main entry file
//
// Darren Young [youngd24@gmail.com]
//
// ============================================================================
// LICENSE
// ============================================================================
//
// BSD 3-Clause License
//
// Copyright (c) 2024, Darren Young
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ============================================================================

// ============================================================================
// INCLUDES
// ============================================================================

// ----------------------------------------------------------------------------
// Standard includes
// ----------------------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#include <Adafruit_PN532.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

// ----------------------------------------------------------------------------
// add-on/local includes
// ----------------------------------------------------------------------------
#include <Noiasca_led.h>
#include <utility/Noiasca_neopixel.h>

#include "bitmaps.h"

// ============================================================================
// DEFINES
// ============================================================================

// Input buttons
#define BTN1 33
#define BTN2 32

// PN532 SPI pins
#define PN532_SCK   (18)
#define PN532_MISO  (19)
#define PN532_MOSI  (23)
#define PN532_SS    (5)
#define PN532_IRQ   (4)
#define PN532_RESET (17)

// various delay timers
#define PN532_ACK_DELAY 100
#define LOOP_READ_DELAY 250

// OLED settings
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT    64 // OLED display height, in pixels
#define OLED_RESET       -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// WS2812 LED
#define LED_PIN    27
#define LED_COUNT  2

// LED colors
#define LED_RED 0,255,0
#define LED_GRN 255,0,0
#define LED_BLU 0,0,255

// LED assignments
#define SOUTH_LED 0
#define NORTH_LED 1
#define EAST_LED  2
#define WEST_LED  3
#define PRIME_LED 4
#define NW_LED    5
#define GAL_LED   6

// Prefs
#define prefReadOnly FALSE


// ============================================================================
// Object casts
// ============================================================================

// Hardware SPI
Adafruit_PN532 nfc(PN532_SS);

// Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WS2812 LED strip
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

// 
BlinkPixel blinkPixelA(strip, SOUTH_LED);      // a blink LED on a Neopixel strip using pixel 0
BlinkPixel blinkPixelB(strip, NORTH_LED);      // a blink LED on a Neopixel strip using pixel 1

// Data persistence using Preferences
Preferences prefs;

// ============================================================================
// Global variables
// ============================================================================
uint8_t uid[]              = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t uidLength;                                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
uint8_t nfcCardReadSuccess = 0;
uint32_t timeoutNfc        = 0;

bool nfcInterruptTriggered = false;
bool readerDisabled        = false;

int btn1State              = HIGH;
int btn2State              = HIGH;

// ============================================================================
// FUNCTIONS
// ============================================================================

// ----------------------------------------------------------------------------
// NAME        : nfcInterruptHandler
// DESCRIPTION : NFC interrupt handler
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR nfcInterruptHandler() {
    detachInterrupt(PN532_IRQ);
    nfcInterruptTriggered = true;
    readerDisabled = true;
}


// ----------------------------------------------------------------------------
// NAME        : processUid
// DESCRIPTION : Process a card read uid
// ----------------------------------------------------------------------------
void processUid(uint8_t* uid, uint8_t uidLength) {
    Serial.println("processUid(): entering");
    String lmUrl;

    // Try to read NTAG2xx memory and extract an URL
    // TODO: mifare classic/ultralight
    // TODO: Parsing from the app "NFC Reader" on Android Play store, not only the spec. To be validated
    // Read only the first record
  
    if (uidLength == 7) {

        uint8_t headerPage[4];
        // Read tag type, if any
        nfc.ntag2xx_ReadPage(4, headerPage);
        Serial.print("Header 4: ");
        nfc.PrintHex(headerPage, 4);
        Serial.println();

        if (headerPage[1] == 0x03) {
            Serial.println("NDEF RECORD");
            // Read header
            nfc.ntag2xx_ReadPage(5, headerPage);
            Serial.print("Header 5: ");
            nfc.PrintHex(headerPage, 4);
            Serial.println();

            if ((headerPage[3] && 0x07) == 0x01) {
                nfc.ntag2xx_ReadPage(6, headerPage);
                Serial.print("Header 6: ");
                nfc.PrintHex(headerPage, 4);
                Serial.println();
                Serial.println("NDEF - Well known record");

                if (headerPage[2] == 0x55) {
                Serial.println("NDEF - Well known URI");
                switch(headerPage[3]) {
                    case 0x01:
                    lmUrl = "http://www.";
                    break;
                    case 0x02:
                    lmUrl = "https://www.";
                    break;
                    case 0x03:
                    lmUrl = "http://";
                    break;
                    case 0x04:
                    lmUrl = "https://";
                    break;
                    default:
                    Serial.print("NDEF - Value: '0x");
                    Serial.print(headerPage[3], HEX);
                    Serial.println("' unknown.");
                }

                // Read URL
                uint8_t data[((headerPage[1]-1)/4+1)*4+1];
                for (uint8_t page = 7; page < ((headerPage[1]-1)/4+1) + 7; page ++) {
                    nfc.ntag2xx_ReadPage(page, data + (page - 7) * 4);
                }

                data[headerPage[1]-1] = '\0';
                nfc.PrintHex(data, 12);
                lmUrl += String((char *) data);
                Serial.print("NDEF - URL: ");
                Serial.println(lmUrl);
                }
            }
        }
    }

    if (lmUrl.length() == 0) {
        Serial.println("processUid(): length 0");

        String url = String("/bzImage/uid/0x");
        for (uint8_t i = 0; i< uidLength; i++) {
            url += String(uid[i], HEX);
        }
        Serial.print("processUid(): URL: ");
        Serial.println(url);

    }

    Serial.println("processUid(): leaving");
}

// ----------------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println("setup(): entering");

    // Load prefs
    prefs.begin("MeetupBadge", false);
    prefs.end();

    // LED strip
    Serial.println("setup(): Configure/start LED strip");
    strip.begin();
    strip.show();
    strip.setBrightness(50);
    blinkPixelA.setOnInterval(250);      // set the on time of a pixel
    blinkPixelB.setOffInterval(250);     // set the off time of a pixel
    blinkPixelB.setOnColor(0xFF0000);    // set the on color of a pixel

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("setup(): SSD1306 allocation failed"));

        // TODO: blink LED's to indicate failure here
        for(;;); // Don't proceed, loop forever
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    // TODO: make this a BurbSec logo
    //display.clearDisplay();
    delay(1000);
    display.clearDisplay();
    display.drawBitmap(0, 0, epd_bitmap_burbsec_interstate_shields, 128, 64, WHITE);
    display.display();
    delay(2000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();
    display.setTextSize(1);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(0, 0);              // Start at top-left corner
    display.println("= WAITING FOR CARD =");
    display.display();

    // Configure input pullup resistors
    Serial.println("setup: setting BTN1/BTN2/PN532_IRQ to INPUT_PULLUP");
    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
    pinMode(PN532_IRQ, INPUT_PULLUP);    
  
    // NFC
    Serial.println("setup(): Setting up NFC reader");
    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("setup(): Didn't find PN53x board");
        while (1); // halt
    } else {
        // Got ok data, print it out!
        Serial.print("setup(): Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
        Serial.print("setup(): Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
        Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    }   
  
    // configure board to read RFID tags
    Serial.println("setup(): calling nfc.SAMConfig");
    nfc.SAMConfig();
    delay(PN532_ACK_DELAY);

    // Start looking for reads
    // Most commands to the PN532 need a delay at the end to ensure completion
    Serial.println("setup(): nfc set passive detection");
    nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
    delay(PN532_ACK_DELAY);

    // Register IRQ
    Serial.println("setup(): attaching nfc interrupt");
    attachInterrupt(digitalPinToInterrupt(PN532_IRQ), nfcInterruptHandler, FALLING);

    Serial.println("setup(): Waiting for an ISO14443A Card ...");
    Serial.println("setup(): leaving");
}

// ============================================================================
// Main loop
// ============================================================================
void loop() {

    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)

    // read button 1
    // TODO: replace with interrupt?
    btn1State = digitalRead(BTN1);
    if (btn1State == LOW) {
        Serial.println("loop(): BTN1 pressed");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("BUTTON 1");
        display.display();
    }

    // read button 2
    // TODO: replace with interrupt?
    btn2State = digitalRead(BTN2);
    if (btn2State == LOW) {
        Serial.println("loop(): BTN2 pressed");
        display.clearDisplay();
        display.setCursor(0, 0);       
        display.println("BUTTON 2");
        display.display();
    }

    // Basic LED colors
    //strip.setPixelColor(SOUTH_LED, strip.Color(LED_RED));
    //strip.setPixelColor(NORTH_LED, strip.Color(LED_RED));
    //strip.show();
    blinkPixelA.update(); 
    blinkPixelB.update();  


    // Got an nfc passive (non-blocking) read interrupt
    if (nfcInterruptTriggered == true) {

        // Read the card and stash the results in uid
        nfcCardReadSuccess = nfc.readDetectedPassiveTargetID(uid, &uidLength);

        // reset the interrupt state indicating we're done reading the card
        nfcInterruptTriggered = false;
    }
  
    // If the card was read successfully, try to do something with it
    if (nfcCardReadSuccess) {
        Serial.println("loop(): nfcCardReadSuccess entering");

        // Display some basic information about the card
        Serial.println("loop(): Found an ISO14443A card");
        Serial.print("loop():  UID Length: ");
        Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("loop():  UID Value: ");
        nfc.PrintHex(uid, uidLength);

        display.clearDisplay();
        display.setTextSize(1);               // Normal 1:1 pixel scale
        display.setTextColor(SSD1306_WHITE);  // Draw white text
        display.setCursor(0, 0);              // Start at top-left corner

        display.println("Card Detected");
        display.print("Size of UID: "); display.print(uidLength, DEC);
        display.println(" bytes");
        display.print("UID: ");

        for (uint8_t i = 0; i < uidLength; i++)
        {
            display.print(" 0x"); display.print(uid[i], HEX);
        }

        display.display();
    
        if (uidLength == 4) {
            // We probably have a Mifare Classic card ... 
            processUid(uid, 4);
        }
        
        if (uidLength == 7) {
            // We probably have a Mifare Ultralight card or NTAG ...
            processUid(uid, 7);
        }

        // Rearm for next tag, 
        nfcCardReadSuccess = 0;
    
  }


    // Tell the reader to go back into passive detection mode
    // and reattach the intterupt handler
    readerDisabled = false;
    nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
    attachInterrupt(digitalPinToInterrupt(PN532_IRQ), nfcInterruptHandler, FALLING);

    // reset button states on the way out of the loop
    btn1State = HIGH;
    btn2State = HIGH;

    // spam loop
    delay(LOOP_READ_DELAY);
}