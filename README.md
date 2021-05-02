# LSR32IO
LSR 32IO Expansion library designed for Arduino compatible boards.

# Intro 
SPI I/O expansion module for microcontrollers and microprocessors. 
Consists of 32 inputs and 32 outputs per module (64 pins total).
Supports input range between +5V to +50V DC and output capability of +9V to 36V DC with each output capable of 100mA curent.
Each input and output pin has its indicator LED like modern PLCs. This really helps to visualize and simulate different scenarios without having anything wired.

![image](https://user-images.githubusercontent.com/26510083/116824964-24bc7100-ab8d-11eb-8951-ca0015a675e2.png)
![image](https://user-images.githubusercontent.com/26510083/116825433-a31a1280-ab8f-11eb-8481-561565455cd3.png)
Note: running tests using the SelfCheck example with 2 of the first prototype modules in series, each having mixed ultra bright and normal LED lights.
![image](https://user-images.githubusercontent.com/26510083/116825482-d2c91a80-ab8f-11eb-8c36-113d5e8c2c8e.png)
![image](https://user-images.githubusercontent.com/26510083/116825527-0310b900-ab90-11eb-888f-b41a30e04680.png)
Better pictures comming soon


# Logic
The module uses the SPI interface to communicate with a host controller.
The main logic is based on 4 SIPO (Serial IN Parallel OUT) and 4 PISO (Parallel IN Serial OUT) Latching Shift Registers (LSR).
Transmission is controlled by an octal tri-state buffer to better control the SPI signal lines, since the PISO LSR doesn't fully comply with the SPI standard.

# Design
The module is made of using almost only off-the-shelf low-cost parts.
It is designed to work in industrial environments where reliable I/O control is desired.
The board is designed to be connected to multiple modules of the same type serially. 
This way we can extend the number of IO even more without using any additional pin of the host controller.
You can choose between PNP and NPN transistor output amplifiers BUT if you decide to use PNP, you must solder a jumper between the selector pins AND use the correct transistor arrays. Everything is written on the PCB. Just make sure you have each module have its 4 matching transistor arrays.

# Software
The library is designed to control up to 8 modules connected in series (soft limit). Because why would you need 8x32 I/O (256 inputs, 256 outputs).
Using the library is very simple. You just connect to the 3 default SPI pins and define the other 3 control pins (4th optional reset pin) when initializing the library class object. Then in setup call the setup with the number of modules you have connected to the line. Then just start using it. Look up the examples. The examples were tested on STM32 and AVR MCUs, but it can work on any MCU that is Arduino compatible.

# Hardware
Link to the PCB design files: https://circuitmaker.com/Projects/Details/Joe-Vovk/LSR-32IO-Expansion

