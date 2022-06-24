# sibo-ssd-dump

This is the SIBO SSD Dumper.

It is designed to work with the [libsibo](https://github.com/PockeNerdIO/libsibo/) test routines. Libsibo needs to be installed on your Arduino device of choice.

This app runs on Win32, Win64, Linux and macOS. It should run on *BSD, but I've only tried OpenBSD. You'll also find some binaries for Win32, Win64 and macOS in the repository.

**This is ALPHA-QUALITY SOFTWARE.** Things are likely to be broken. Use it at your own risk.

## Using with an Arduino Uno

With pin 1 on the SSD being closest to its edge, connect the SSD to your Arduino as follows:

* SSD Pin 1 -> Arduino Pin 2
* SSD Pin 2 -> Arduino GND
* SSD Pin 5 -> Arduino 5V
* SSD Pin 6 -> Arduino Pin 3

SSD pins 3 and 4 aren't needed for this.

## Using with 3.3v microcontrollers
You will need to have some sort of logic level shifting to safely communicate with Psion hardware. More details on this can be found in the sibolib readme.
