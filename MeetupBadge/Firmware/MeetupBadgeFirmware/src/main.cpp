// ============================================================================
//
// BurbSec MeetupBadge Firmware
//
// main.cpp
//
// Darren Young [youngd24@gmail.com]
//
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

// ----------------------------------------------------------------------------
// Includes
// ----------------------------------------------------------------------------

// Standard includes
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// add-on/local includes


// ----------------------------------------------------------------------------
// Defines
// ----------------------------------------------------------------------------

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
#define SETUP_READ_DELAY 300
#define LOOP_READ_DELAY 250

// OLED settings
#define SCREEN_WIDTH    128 // OLED display width, in pixels
#define SCREEN_HEIGHT    64 // OLED display height, in pixels
#define OLED_RESET       -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


// ----------------------------------------------------------------------------
// Object casts
// ----------------------------------------------------------------------------

// Hardware SPI
Adafruit_PN532 nfc(PN532_SS);

// Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------
uint8_t uid[]           = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t uidPlaying[]    = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the playing UID
uint8_t uidLength;                                  // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
uint8_t success         = 0;
uint32_t timeoutNfc     = 0;

bool interruptTriggered = false;
bool readerDisabled     = false;

int btn1State = HIGH;
int btn2State = HIGH;

// ----------------------------------------------------------------------------
// Local methods/functions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// NAME        : handleInterrupt
// DESCRIPTION : NFC interrupt handler
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR handleInterrupt() {
    detachInterrupt(PN532_IRQ);
    interruptTriggered = true;
    readerDisabled = true;
}


// ----------------------------------------------------------------------------
// NAME        : processUid
// DESCRIPTION : Process a card read uid
// ----------------------------------------------------------------------------
void processUid(uint8_t* uid, uint8_t uidLength) {
    Serial.println("processUid(): entering");
    String mp3Url;

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
                Serial.println("NDEF - Well know record");

                if (headerPage[2] == 0x55) {
                Serial.println("NDEF - Well know URI");
                switch(headerPage[3]) {
                    case 0x01:
                    mp3Url = "http://www.";
                    break;
                    case 0x02:
                    mp3Url = "https://www.";
                    break;
                    case 0x03:
                    mp3Url = "http://";
                    break;
                    case 0x04:
                    mp3Url = "https://";
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
                mp3Url += String((char *) data);
                Serial.print("NDEF - URL: ");
                Serial.println(mp3Url);
                }
            }
        }
    }

    if (mp3Url.length() == 0) {
        Serial.println("processUid(): length 0");

        String url = String("/nfcAudio/uid/0x");
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

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("setup(): SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(2000); // Pause for 2 seconds

    // Clear the buffer
    display.clearDisplay();

    Serial.println("loop: setting BTN1/BTN2 to INPUT_PULLUP");
    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
  
    // NFC
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
    Serial.println("setup(): Waiting for an ISO14443A Card ...");

    // Register IRQ
    Serial.println("setup(): Setting pinMode on IRQ line");
    pinMode(PN532_IRQ, INPUT_PULLUP);

    // Start looking for reads
    Serial.println("setup(): nfc start passive detection");
    nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
    Serial.print("setup(): setting delay => ");Serial.println(SETUP_READ_DELAY);
    delay(SETUP_READ_DELAY);

    Serial.println("setup(): attaching interrupt");
    attachInterrupt(digitalPinToInterrupt(PN532_IRQ), handleInterrupt, FALLING);

    Serial.println("setup(): leaving");
}

// ----------------------------------------------------------------------------
// Main loop
// ----------------------------------------------------------------------------
void loop() {

    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)

    //Serial.println("loop(): entering");

    btn1State = digitalRead(BTN1);
    if (btn1State == LOW) {
      Serial.println("loop(): BTN1 pressed");
    }

    btn2State = digitalRead(BTN2);
    if (btn2State == LOW) {
      Serial.println("loop(): BTN2 pressed");
    }

    if (interruptTriggered == true) {
        //Serial.println("interrupt triggered");
        success = nfc.readDetectedPassiveTargetID(uid, &uidLength);
        interruptTriggered = false;
    }
  
    if (success) {
        Serial.println("loop(): entering success");
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
        success = 0;
    
  }



  //if (readerDisabled == true) {
    // Reactivate reader after timeout
    readerDisabled = false;
    nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
    attachInterrupt(digitalPinToInterrupt(PN532_IRQ), handleInterrupt, FALLING);

    // reset button states on the way out of the loop
    btn1State = HIGH;
    btn2State = HIGH;

    delay(LOOP_READ_DELAY);

    //Serial.println("loop(): leaving");
  //}

}