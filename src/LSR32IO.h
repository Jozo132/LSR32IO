// LSR 32IO Expansion Library
// Author: J.Vovk <jozo132@gmail.com>

#ifndef __VOVK_LSR32IO_H
#define __VOVK_LSR32IO_H

#include <Arduino.h>
#include <SPI.h>

#ifndef LSR32IO_SPI_CONF
#define LSR32IO_SPI_CONF SPISettings(4000000UL, MSBFIRST, SPI_MODE0)
#endif // LSR32IO_SPI_CONF


#ifndef LSR32IO_MAX_STACK_SIZE
#define LSR32IO_MAX_STACK_SIZE 8
#endif // LSR32IO_MAX_STACK_SIZE

class LSR32IO {
private:
    int LSR_LATCH = -1;     // PB5
    int LSR_CS = -1;        // PA15
    int LSR_CLK_EN = -1;    // PB4
    int LSR_RESET = -1;     // PB2
    SPIClass* spi;
    bool pins_ready = false;
    bool spi_set = false;
    int i = 0;
    int index = 0;
    bool sizeSet = false;
    int size = 1;
    int segmentByteCount = 4;
    int maxSegments = 32;
    int maxAddress = 31;
    long t = 0;
    byte tempByte;
    byte input[4 * LSR32IO_MAX_STACK_SIZE] = { 0x00 };
    byte output[4 * LSR32IO_MAX_STACK_SIZE] = { 0x00 };
    long interval = 10;
    long interval_last = 0;
    void latch();

    byte setBit(byte b, unsigned int bit);
    byte resetBit(byte b, unsigned int bit);
    byte toggleBit(byte b, unsigned int bit);
    void setPins(int cs_pin, int latch_pin, int en_pin = -1, int reset_pin = -1);
public:
    LSR32IO(int cs_pin, int latch_pin, int en_pin = -1, int reset_pin = -1);
    LSR32IO(int cs_pin, int sck_pin, int miso_pin, int mosi_pin, int latch_pin, int en_pin = -1, int reset_pin = -1);
    setSPI(int sck_pin, int miso_pin, int mosi_pin);
    void setInterval(int interval_us);
    bool begin(int new_size = 1);
    void loop();

    int availableBits();
    int availableBytes();

    bool read(int bit);
    bool readOutput(int bit);
    void write(int bit, bool state);
    void toggle(int bit);

    byte readByte(int segment);
    byte readOutputByte(int segment);
    void writeByte(int segment, byte value);
    byte* readBytes();
    byte* readOutputBytes();
    void writeBytes(byte* value, int length);

    void clear();
    void reset();

    void TEST_mapInputsToOutputs();
};

#endif // __VOVK_LSR32IO_H