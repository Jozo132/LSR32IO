// LSR 32IO Expansion Library
// Author: J.Vovk <jozo132@gmail.com>

#ifndef __VOVK_LSR32IO_H
#define __VOVK_LSR32IO_H

#include <Arduino.h>
#include <SPI.h>
#include "stdint.h"

// Cursed libraries that define these macros names, breaking my code. I'm looking at you, SPIMemory
#undef setBit
#undef resetBit
#undef toggleBit


#ifndef LSR32IO_SPI_CONF
#define LSR32IO_SPI_CONF SPISettings(4000000UL, MSBFIRST, SPI_MODE0)
#endif // LSR32IO_SPI_CONF


#ifndef LSR32IO_MAX_STACK_SIZE
#define LSR32IO_MAX_STACK_SIZE 8
#endif // LSR32IO_MAX_STACK_SIZE




class LSR32IO {
private:

    class Debounce {
    private:
        int count = 0;
        bool previous_state = false;
        bool debounced_state = false;

    public:
        int count_target = 50;
        Debounce(bool initialState = false, int debounce_count = 1) {
            setCount(debounce_count);
            debounced_state = initialState;
        }
        void setCount(int debounce_count) {
            count_target = debounce_count > 0 ? debounce_count : count_target;
        }
        bool update(bool state) {
            if (state == previous_state && count < count_target)
                count++;
            if (state != previous_state)
                count = 0;
            if (count >= count_target)
                debounced_state = state;
            previous_state = state;
            return debounced_state;
        }
    };

    int LSR_LATCH = -1;     // PB5
    int LSR_CS = -1;        // PA15
    int LSR_CLK_EN = -1;    // PB4
    int LSR_RESET = -1;     // PB2
    SPIClass* spi;
    bool spi_set = false;

    int index = 0;
    int maxSegments = 4 * LSR32IO_MAX_STACK_SIZE;
    uint32_t interval = 10;
    uint32_t interval_last = 0;

    long latency_total;
    long latency_start;
    long latency_transfer;
    long latency_end;

    int i, j = 0;
    bool sizeSet = false;
    int size = 1;
    int segmentByteCount = 4;
    int maxAddress = 32 - 1;
    long t = 0;
    uint8_t tempByte;
    uint8_t input[4 * LSR32IO_MAX_STACK_SIZE] = { 0x00 };
    uint8_t output[4 * LSR32IO_MAX_STACK_SIZE] = { 0x00 };
    bool input_bit[8 * 4 * LSR32IO_MAX_STACK_SIZE] = { false };
    bool output_bit[8 * 4 * LSR32IO_MAX_STACK_SIZE] = { false };
    bool useDebounce[8 * 4 * LSR32IO_MAX_STACK_SIZE] = { false };
    bool invertedInput[8 * 4 * LSR32IO_MAX_STACK_SIZE] = { false };
    bool invertedOutput[8 * 4 * LSR32IO_MAX_STACK_SIZE] = { false };
    Debounce* debounce[8 * 4 * LSR32IO_MAX_STACK_SIZE];
    bool usePWM[8 * 4 * LSR32IO_MAX_STACK_SIZE] = { false };
    int pwm[8 * 4 * LSR32IO_MAX_STACK_SIZE] = { 0 };
    int pwm_count = 0;
    int pwm_count_overflow = 100;

    void latch();

    uint8_t setBit(uint8_t b, int bit);
    uint8_t resetBit(uint8_t b, int bit);
    uint8_t invertBit(uint8_t b, int bit);
    void map_io_pointers();



public:
    static constexpr const char* version = "1.0.2";
    LSR32IO(int cs_pin, int latch_pin, int en_pin, int reset_pin = -1);

    void setSPI(int sck_pin = -1, int miso_pin = -1, int mosi_pin = -1);
    void setInterval(int interval_us);
    void setPWMOverflow(int overflow);
    int getPWMOverflow();
    void resetPWMCounter();
    bool begin(int new_size = 1);
    void loop();

    int availableBits();
    int availableBytes();

    bool read(int bit);
    bool readOutput(int bit);
    void write(int bit, bool state);
    void writePWM(int bit, int value);
    void toggle(int bit);

    uint8_t readByte(int segment);
    uint8_t readOutputByte(int segment);
    void writeByte(int segment, uint8_t value);
    uint8_t* readBytes();
    uint8_t* readOutputBytes();
    void writeBytes(uint8_t* value, int length);


    bool& attachInputBit(int bit);
    bool& attachInputBit(int bit, int debounce_cycles, bool inverted = false);
    bool& attachInputBit(int bit, bool inverted, int debounce_cycles = 1);

    bool& attachOutputBit(int bit, bool inverted = false);
    int& attachOutputPWM(int bit, bool inverted = false);

    void clear();
    void reset();

    void TEST_mapInputsToOutputs();
};

#endif // __VOVK_LSR32IO_H