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
    uint16_t LSR_LATCH = -1;     // PB5
    uint16_t LSR_CS = -1;        // PA15
    uint16_t LSR_CLK_EN = -1;    // PB4
    uint16_t LSR_RESET = -1;     // PB2
    SPIClass* spi;
    bool spi_set = false;
    uint16_t i = 0;
    uint16_t index = 0;
    bool sizeSet = false;
    uint16_t size = 1;
    uint16_t segmentByteCount = 4;
    uint16_t maxSegments = 32;
    uint16_t maxAddress = 31;
    uint32_t t = 0;
    uint8_t tempByte;
    uint8_t input[4 * LSR32IO_MAX_STACK_SIZE] = { 0x00 };
    uint8_t output[4 * LSR32IO_MAX_STACK_SIZE] = { 0x00 };
    uint32_t interval = 10;
    uint32_t interval_last = 0;
    void latch();

    uint8_t setBit(uint8_t b, uint16_t bit);
    uint8_t resetBit(uint8_t b, uint16_t bit);
    uint8_t invertBit(uint8_t b, uint16_t bit);

public:
    LSR32IO(uint16_t cs_pin, uint16_t latch_pin, uint16_t en_pin, uint16_t reset_pin = -1);

    void setSPI(uint16_t sck_pin = -1, uint16_t miso_pin = -1, uint16_t mosi_pin = -1);
    void setInterval(uint16_t interval_us);
    bool begin(uint16_t new_size = 1);
    void loop();

    uint16_t availableBits();
    uint16_t availableBytes();

    bool read(uint16_t bit);
    bool readOutput(uint16_t bit);
    void write(uint16_t bit, bool state);
    void toggle(uint16_t bit);

    uint8_t readByte(uint16_t segment);
    uint8_t readOutputByte(uint16_t segment);
    void writeByte(uint16_t segment, uint8_t value);
    uint8_t* readBytes();
    uint8_t* readOutputBytes();
    void writeBytes(uint8_t* value, uint16_t length);

    void clear();
    void reset();

    void TEST_mapInputsToOutputs();
};

#endif // __VOVK_LSR32IO_H