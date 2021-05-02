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

long t, last_event;
bool state;

void loop() {
    t = millis();
    if (t < last_event || t > last_event + 500) {
        last_event = t;
        // Gets called every N milliseconds (accounting for longterm millis() overflow)
        state = !state; // Toggle state
        for (int i = 0; i < expansion.availableBytes(); i++) { // For every byte available in the module
            if (state) expansion.writeByte(i, 0xFF); // Either turn on all 8 bits of the byte at [i]
            else expansion.writeByte(i, 0x00); // Or turn off all 8 bits of the byte at [i]
        }
    }
    expansion.loop(); // Run this function as fast as possible 
}