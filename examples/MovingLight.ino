#include <LSR32IO.h>

#define cs_pin    PA15
#define sck_pin   PA5
#define miso_pin  PA6
#define mosi_pin  PA7
#define latch_pin PB5
#define en_pin    PB4

LSR32IO expansion(cs_pin, sck_pin, miso_pin, mosi_pin, latch_pin, en_pin);

void setup() {
    expansion.begin(); // Begin with default 1 module connected
    //expansion.begin(2); // Number of modules stacked between 1 and 8, default is 1
}

long t, last_event;
int a, b = 0;

void loop() {
    t = millis();
    if (t < last_event || t > last_event + 30) {
        last_event = t;
        // Gets called every N milliseconds (accounting for longterm millis() overflow)
        expansion.write(a, LOW);
        expansion.write(b, HIGH);
        a = b;
        b++;
        if (b >= expansion.availableBits())
            b = 0;
    }
    expansion.loop(); // Run this function as fast as possible 
}