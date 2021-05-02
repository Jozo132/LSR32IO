// LSR 32IO Expansion Library
// Author: J.Vovk <jozo132@gmail.com>

#include "LSR32IO.h"


// Sets bit at position [bit] of byte [b] to 1 
byte LSR32IO::setBit(byte b, unsigned int bit) {
    b |= 1 << bit;
    return b;
}
// Sets bit at position [bit] of byte [b] to 0 
byte LSR32IO::resetBit(byte b, unsigned int bit) {
    b &= ~(1 << bit);
    return b;
}
// Toggles bit at position [bit] of byte [b]
byte LSR32IO::toggleBit(byte b, unsigned int bit) {
    b ^= 1UL << bit;
    return b;
}

// Function that sets target pins and prepares the SPIClass 
void LSR32IO::setPins(int cs_pin, int sck_pin, int miso_pin, int mosi_pin, int latch_pin, int en_pin, int reset_pin) {
    LSR_CS = cs_pin;
    LSR_SCK = sck_pin;
    LSR_MISO = miso_pin;
    LSR_MOSI = mosi_pin;
    LSR_LATCH = latch_pin;
    LSR_CLK_EN = en_pin;
    LSR_RESET = reset_pin;
#ifdef __AVR__
    spi = *SPI;
#else
    spi = new SPIClass(mosi_pin, miso_pin, sck_pin);
#endif
    pins_ready = true;
}

LSR32IO::LSR32IO(int cs_pin, int sck_pin, int miso_pin, int mosi_pin, int latch_pin, int en_pin, int reset_pin) {
    this->setPins(cs_pin, sck_pin, miso_pin, mosi_pin, latch_pin, en_pin, reset_pin);
}

// Function that sets the update interval used in the loop function
// Unit: microseconds [us]
// By default it's 10 [us]
// Example:   expansion.setInterval(150);   // Changes update interval to 150us per cycle
void LSR32IO::setInterval(int interval_us) {
    interval = interval_us >= 0 ? interval_us : interval;
}

// Function to trigger the LATCH of all connected shift registers
void LSR32IO::latch() {
    digitalWrite(LSR_CLK_EN, HIGH);
    digitalWrite(LSR_LATCH, LOW);
    digitalWrite(LSR_LATCH, HIGH);
    digitalWrite(LSR_CLK_EN, LOW);
}

// Initialize the module and optionaly choose the number of stacked expanison modules 
bool LSR32IO::begin(int new_size) {
    size = new_size > 0 && new_size <= LSR32IO_MAX_STACK_SIZE ? new_size : size;
    maxSegments = 4 * LSR32IO_MAX_STACK_SIZE;
    segmentByteCount = 4 * size;
    maxAddress = 8 * segmentByteCount - 1;
    if (!pins_ready) return false;
    pinMode(LSR_LATCH, OUTPUT);
    digitalWrite(LSR_LATCH, HIGH);
    pinMode(LSR_CLK_EN, OUTPUT);
    digitalWrite(LSR_CLK_EN, HIGH);
    pinMode(LSR_RESET, OUTPUT);
    digitalWrite(LSR_RESET, HIGH);
    pinMode(LSR_CS, OUTPUT);
    digitalWrite(LSR_CS, HIGH);
    (*spi).begin();
    this->reset();
    return true;
}

// Function which should be called as fast as possible!
// It is responsible for the data transmission between the controller and the expansion module(s) 
// The SPI communication is optimized for the LSRs to read and write at the same time
// Make sure you write non-blocking code or use FreeRTOS
// Every delay will increase the response latency  
void LSR32IO::loop() {
    t = micros();
    if (t < interval_last || t > interval_last + interval) {
        interval_last = t;
        digitalWrite(LSR_CS, LOW);
        delayMicroseconds(1);
        (*spi).beginTransaction(LSR32IO_SPI_CONF);
        for (i = 0; i < segmentByteCount; i++)
            input[i] = (*spi).transfer(output[segmentByteCount - i - 1]);
        (*spi).endTransaction();
        delayMicroseconds(1);
        this->latch(); // Trigger transfered data
        digitalWrite(LSR_CS, HIGH);
    }
}

// Returns the number of available bits
int LSR32IO::availableBits() {
    return maxAddress + 1;
}

// Returns the number of available bytes
int LSR32IO::availableBytes() {
    return segmentByteCount;
}

// Reads the boolean bit value at the index of the input
// Example:   bool button = expansion.read(28);  // Saves the last known input state at input 28 into bool variable [button]
bool LSR32IO::read(int bit) {
    if (bit < 0 || bit > maxAddress) return false; // Avoid overflow
    int segment = bit / 8;
    int target = bit - segment * 8;
    return ((input[segment] >> target) & 0x01);
}

// Reads the boolean bit value at the index of the output
// Example:   bool motor_is_on = expansion.readOutput(9);  // Saves the last known state at output 9 into bool variable [motor_is_on]
bool LSR32IO::readOutput(int bit) {
    if (bit < 0 || bit > maxAddress) return false; // Avoid overflow
    int segment = bit / 8;
    int target = bit - segment * 8;
    return ((output[segment] >> target) & 0x01);
}

// Writes the state value to the target output
// Example:   
//      expansion.write(17, true);  // Prepares output 17 to be ON next time the loop function makes a cycle
//      expansion.write(17, HIGH);  // Same as above, excapet using the value of HIGH
void LSR32IO::write(int bit, bool state) {
    if (bit < 0 || bit > maxAddress) return; // Avoid overflow
    int segment = bit / 8;
    int target = bit - segment * 8;
    output[segment] = state ? this->setBit(output[segment], target) : this->resetBit(output[segment], target);
}

// Toggles the state value of the target output
// Example:   
//      expansion.toggle(21);  // Prepares output 21 to be inverted (from 1 to 0 or vice versa) next time the loop function makes a cycle
void LSR32IO::toggle(int bit) {
    if (bit < 0 || bit > maxAddress) return; // Avoid overflow
    int segment = bit / 8;
    int target = bit - segment * 8;
    output[segment] = this->toggleBit(output[segment], target);
}

// Reads the byte value at requested input segment
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you can access between address 0 and 3
// Example:   
//      byte data = expansion.readByte(0);  // Reads out the last known input states into a byte of 8 bits 
byte LSR32IO::readByte(int segment) {
    if (segment < 0 || segment >= segmentByteCount) return 0; // Avoid overflow
    return input[segment];
}

// Reads the byte value at requested output segment
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you can access between address 0 and 3
// Example:   
//      byte data = expansion.readOutputByte(0);  // Reads out the last known output states into a byte of 8 bits 
byte LSR32IO::readOutputByte(int segment) {
    if (segment < 0 || segment >= segmentByteCount) return 0; // Avoid overflow
    return output[segment];
}

// Writes the given byte value to the requested output segment
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you can access between address 0 and 3
// Example:   
//      expansion.writeByte(0, 0xFF);  // Sets all first 8 [0-7] outputs to active (ON)
void LSR32IO::writeByte(int segment, byte value) {
    if (segment < 0 || segment >= segmentByteCount) return; // Avoid overflow
    output[segment] = value;
}

// Function which returns the byte array of all inputs of the expansion module
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you will get an array of 4 input bytes
byte* LSR32IO::readBytes() {
    return input;
}

// Function which returns the byte array of all outputs of the expansion module
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you will get an array of 4 output bytes
byte* LSR32IO::readOutputBytes() {
    return output;
}

// Function which writes a sequence of bytes into the outputs with a given length
void LSR32IO::writeBytes(byte* values, int length) {
    if (length < 0 || length >= segmentByteCount) return; // Avoid overflow
    for (i = 0; i < length; i++)
        output[i] = values[i];
}

// Function which clears all output bits
// Note: it executes the data transfer instantly,
// so it ignores the loop function
void LSR32IO::clear() {
    for (i = 0; i < segmentByteCount; i++)
        output[i] = 0;
    digitalWrite(LSR_CS, LOW);
    (*spi).beginTransaction(LSR32IO_SPI_CONF);
    for (i = 0; i < maxSegments; i++)
        input[i] = (*spi).transfer(0);
    (*spi).endTransaction();
    this->latch(); // Trigger transfered data
    digitalWrite(LSR_CS, HIGH);
}

// Function which resets all LSR modules, clears local output states and forces 0 to all outputs
// Note: it executes the data transfer instantly,
// so it ignores the loop function
void LSR32IO::reset() {
    digitalWrite(LSR_RESET, LOW);
    delayMicroseconds(1);
    digitalWrite(LSR_RESET, HIGH);
    this->clear();
}

// Function which maps all detected input bits directly to the outputs on the opposite side.
// This function is very useful for debugging the modules.
// Avoid every calling this function inside production code!
void LSR32IO::TEST_mapInputsToOutputs() {
    for (i = 0; i < segmentByteCount; i++)
        output[i] = input[i];
}