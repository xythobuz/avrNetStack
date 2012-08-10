# avrNetStack 

This aims to be a very modular Networking Stack running on AVR Microcontrollers and supporting different Network Hardware (ENC28J60, MRF24WB).
Select your MCU and hardware driver in the makefile.
Compile with "make lib" to create a static library.
Compile with "make test" to create a test hex file to use with the hardware found in Hardware/avrNetStack.sch. You need Eagle 6, available for free from cadsoft.
In the future, a PCB will be designed that can act as WLAN / LAN Module for your AVR Project, in addition to this software.
