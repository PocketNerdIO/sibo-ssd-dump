# sibo-ssd-dump

This is the SIBO SSD Dumper.

The project has two parts. First is the Arduino sketch that talks to the SSD. The second is a C program that talks to your Arduino over serial and will dump what it receives to a file.

With pin 1 on the SSD being closest to its edge, connect the SSD to your Arduino as follows:

SSD Pin 1 -> Arduino Pin 2
SSD Pin 2 -> Arduino GND
SSD Pin 5 -> Arduino 5V
SSD Pin 6 -> Arduino Pin 3

SSD pins 3 and 4 aren't needed for this.

You can use the Arduino sketch by itself, for example with RealTerm on Windows. Connect RealTerm to your Arduino's serial device, set RealTerm to 57600 baud, start the capture and send "d" to the Arduino. The dump will start immediately.

As with siboimg, this is ALPHA-QUALITY SOFTWARE. The Arduino sketch is reliable, although it doesn't currently handle SSDs with more than one "device" (chip). However, the companion app is in a worse state than siboimg. I've only been able to test it on Linux so far. It will probably run on macOS and BSD, but I haven't tested it personally. It definitely won't run on Windows yet, but that's my next priority. It definitely runs on Linux.

The Arduino sketch was originally written by Karl with new features such as the command interpreter and block-by-block dumping added by me. The companion app is written by me with the addition of argparse and various snippets from StackExchange (makes me feel dirty, but sometimes needs must).

Try it, break it, give me feedback!