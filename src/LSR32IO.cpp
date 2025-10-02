// LSR 32IO Expansion Library
// Author: J.Vovk <jozo132@gmail.com>

#include "LSR32IO.h"
#include "stdint.h"


// Sets bit at position [bit] of uint8_t [b] to 1 
uint8_t LSR32IO::setBit(uint8_t b, int bit) {
    b |= 1 << bit;
    return b;
}
// Sets bit at position [bit] of uint8_t [b] to 0 
uint8_t LSR32IO::resetBit(uint8_t b, int bit) {
    b &= ~(1 << bit);
    return b;
}
// Toggles bit at position [bit] of uint8_t [b]
uint8_t LSR32IO::invertBit(uint8_t b, int bit) {
    b ^= 1UL << bit;
    return b;
}

void LSR32IO::map_io_pointers() {
    int bit;
    bool state;
    for (i = 0; i < segmentByteCount; i++)
        for (j = 0; j < 8; j++) {
            bit = 8 * i + j;
            // INPUTS
            state = (input[i] >> j) & 0x01;
            if (invertedInput[bit]) state = !state;
            if (useDebounce[bit]) input_bit[bit] = debounce[bit]->update(state);
            else input_bit[bit] = state;
            // OUTPUTS
            if (usePWM[bit]) state = pwm[bit] > pwm_count; // PWM
            else state = output_bit[bit]; // Default 0/1
            if (invertedOutput[bit]) state = !state;
            output[i] = state ? this->setBit(output[i], j) : this->resetBit(output[i], j);
        }
    pwm_count++;
    if (pwm_count >= pwm_count_overflow) pwm_count = 0;
}

void LSR32IO::setSPI(int sck_pin, int miso_pin, int mosi_pin) {
    if (spi_set) return;
#ifdef __AVR__
    * spi = SPI; // AVR: Use native SPI
#else
    if (sck_pin >= 0 && miso_pin >= 0 && sck_pin >= 0)
        spi = new SPIClass(mosi_pin, miso_pin, sck_pin);
    else
        spi = new SPIClass(MOSI, MISO, SCK); // Use default SPI
#endif
    spi_set = true;
}

LSR32IO::LSR32IO(int cs_pin, int latch_pin, int en_pin, int reset_pin) {
    LSR_CS = cs_pin;
    LSR_LATCH = latch_pin;
    LSR_CLK_EN = en_pin;
    LSR_RESET = reset_pin;
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
    this->setSPI();
    (*spi).begin();
    pinMode(LSR_RESET, OUTPUT);
    digitalWrite(LSR_RESET, HIGH);
    pinMode(LSR_CS, OUTPUT);
    digitalWrite(LSR_CS, HIGH);
    pinMode(LSR_CLK_EN, OUTPUT);
    digitalWrite(LSR_CLK_EN, HIGH);
    pinMode(LSR_LATCH, OUTPUT);
    digitalWrite(LSR_LATCH, HIGH);
    this->clear();
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
        latency_start = t;
        this->map_io_pointers();
        latency_start = t - latency_start;
        t = micros();
        latency_transfer = t;
        delayMicroseconds(1);
        (*spi).beginTransaction(LSR32IO_SPI_CONF);
        for (i = 0; i < segmentByteCount; i++)
            input[i] = (*spi).transfer(output[segmentByteCount - i - 1]);
        (*spi).endTransaction();
        t = micros();
        latency_transfer = t - latency_transfer;
        latency_end = t;
        delayMicroseconds(1);
        this->latch(); // Trigger transfered data
        digitalWrite(LSR_CS, HIGH);
        t = micros();
        latency_end = t - latency_end;
        latency_total = latency_start + latency_transfer + latency_end;
    }
}


void LSR32IO::setPWMOverflow(int overflow) {
    pwm_count_overflow = overflow >= 0 ? overflow : pwm_count_overflow;
}

int LSR32IO::getPWMOverflow() {
    return pwm_count_overflow;
}

void LSR32IO::resetPWMCounter() {
    pwm_count = 0;
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
    if (bit < 0 || bit >= maxAddress) return false; // Avoid overflow
    return input_bit[bit];
}

// Reads the boolean bit value at the index of the output
// Example:   bool motor_is_on = expansion.readOutput(9);  // Saves the last known state at output 9 into bool variable [motor_is_on]
bool LSR32IO::readOutput(int bit) {
    if (bit < 0 || bit >= maxAddress) return false; // Avoid overflow
    return output_bit[bit];
}

// Writes the state value to the target output
// Example:   
//      expansion.write(17, true);  // Prepares output 17 to be ON next time the loop function makes a cycle
//      expansion.write(17, HIGH);  // Same as above, excapet using the value of HIGH
void LSR32IO::write(int bit, bool state) {
    if (bit < 0 || bit >= maxAddress) return; // Avoid overflow
    if (usePWM[bit]) output_bit[bit] = state ? pwm_count_overflow : 0;
    else output_bit[bit] = state;
}

void LSR32IO::writePWM(int bit, int value) {
    if (bit < 0 || bit >= maxAddress) return; // Avoid overflow
    usePWM[bit] = true;
    pwm[bit] = value;
}

// Toggles the state value of the target output
// Example:   
//      expansion.toggle(21);  // Prepares output 21 to be inverted (from 1 to 0 or vice versa) next time the loop function makes a cycle
void LSR32IO::toggle(int bit) {
    if (bit < 0 || bit >= maxAddress) return; // Avoid overflow
    output_bit[bit] = !output_bit[bit];
}

// Reads the uint8_t value at requested input segment
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you can access between address 0 and 3
// Example:   
//      uint8_t data = expansion.readByte(0);  // Reads out the last known input states into a uint8_t of 8 bits 
uint8_t LSR32IO::readByte(int segment) {
    if (segment < 0 || segment >= segmentByteCount) return 0; // Avoid overflow
    return input[segment];
}

// Reads the uint8_t value at requested output segment
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you can access between address 0 and 3
// Example:   
//      uint8_t data = expansion.readOutputByte(0);  // Reads out the last known output states into a uint8_t of 8 bits 
uint8_t LSR32IO::readOutputByte(int segment) {
    if (segment < 0 || segment >= segmentByteCount) return 0; // Avoid overflow
    return output[segment];
}

// Writes the given uint8_t value to the requested output segment
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you can access between address 0 and 3
// Example:   
//      expansion.writeByte(0, 0xFF);  // Sets all first 8 [0-7] outputs to active (ON)
void LSR32IO::writeByte(int segment, uint8_t value) {
    if (segment < 0 || segment >= segmentByteCount) return; // Avoid overflow
    //output[segment] = value;
    for (j = 0; j < 8; j++)
        output_bit[segment + j] = (value >> j) & 0x01;
}

// Function which returns the uint8_t array of all inputs of the expansion module
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you will get an array of 4 input bytes
uint8_t* LSR32IO::readBytes() {
    return input;
}

// Function which returns the uint8_t array of all outputs of the expansion module
// Each LSR 32IO Expansion has 4 bytes for inputs and 4 bytes for outputs
// So with only one module connected, you will get an array of 4 output bytes
uint8_t* LSR32IO::readOutputBytes() {
    return output;
}

// Function which writes a sequence of bytes into the outputs with a given length
void LSR32IO::writeBytes(uint8_t* values, int length) {
    if (length < 0 || length >= segmentByteCount) return; // Avoid overflow
    for (i = 0; i < length; i++)
        output[i] = values[i];
}


/*
bool& attachInputBit(int bit, int debounce_cycles = 1, bool inverted = false);
bool& attachInputBit(int bit, bool inverted = false, int debounce_cycles = 1);
*/


bool& LSR32IO::attachInputBit(int bit) {
    return input_bit[bit];
}
bool& LSR32IO::attachInputBit(int bit, int debounce_cycles, bool inverted) {
    if (debounce_cycles > 1) {
        useDebounce[bit] = true;
        debounce[bit] = new Debounce(false, debounce_cycles);
    }
    if (inverted) invertedInput[bit] = true;
    return input_bit[bit];
}
bool& LSR32IO::attachInputBit(int bit, bool inverted, int debounce_cycles) {
    if (debounce_cycles > 1) {
        useDebounce[bit] = true;
        debounce[bit] = new Debounce(false, debounce_cycles);
    }
    if (inverted) invertedInput[bit] = true;
    return input_bit[bit];
}

/*
bool& attachOutputBit(int bit, bool inverted = false);
*/

bool& LSR32IO::attachOutputBit(int bit, bool inverted) {
    if (inverted) invertedOutput[bit] = true;
    usePWM[bit] = false;
    return output_bit[bit];
}

int& LSR32IO::attachOutputPWM(int bit, bool inverted) {
    if (inverted) invertedOutput[bit] = true;
    usePWM[bit] = true;
    return pwm[bit];
}


// Function which clears all output bits
// Note: it executes the data transfer instantly,
// so it ignores the loop function
void LSR32IO::clear() {
    for (i = 0; i < maxAddress; i++) {
        output_bit[i] = 0;
        pwm[i] = 0;
    }
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

    for (i = 0; i < maxAddress; i++)
        output_bit[i] = input_bit[i];
}