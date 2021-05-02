#include <LSR32IO.h>

/*
// WeAct BlackPill F4

#define sck_pin   PA5
#define miso_pin  PA6
#define mosi_pin  PA7

#define cs_pin    PA15
#define latch_pin PB5
#define en_pin    PB4

LSR32IO expansion(cs_pin, latch_pin, en_pin);
*/


// Arduino Nano
#define sck_pin   13
#define miso_pin  12
#define mosi_pin  11

#define cs_pin    10
#define latch_pin 9
#define en_pin    8

LSR32IO expansion(cs_pin, latch_pin, en_pin);

void setup() {
    //expansion.setSPI(sck_pin, miso_pin, mosi_pin); // Optionally change SPI pins if needed  (not working with AVR) 
    expansion.begin(); // Begin with default 1 module connected
    //expansion.begin(2); // Number of modules stacked between 1 and 8, default is 1
}

void loop() {
    expansion.TEST_mapInputsToOutputs(); // Map detected input directly to opposite output
    expansion.loop(); // Run this function as fast as possible 
}