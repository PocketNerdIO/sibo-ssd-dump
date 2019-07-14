# sibo-ssd-dump

This is the SIBO SSD Dumper.

This is a tool of two parts: an Arduino sketch which talks to the SSD using SIBO-SP, and a companion app written in C that controls the Arduino over serial and will dump what it receives to a file.

The app runs on Win32 and Win64, Linux and macOS. It should run on *BSD, but I've only tried OpenBSD. You'll also find some binaries for Win32, Win64 and macOS in the repository.

With pin 1 on the SSD being closest to its edge, connect the SSD to your Arduino as follows:

* SSD Pin 1 -> Arduino Pin 2
* SSD Pin 2 -> Arduino GND
* SSD Pin 5 -> Arduino 5V
* SSD Pin 6 -> Arduino Pin 3

SSD pins 3 and 4 aren't needed for this.

If you don't want to use the companion app and want to see the raw data coming through, you can use the Arduino sketch by itself with an app like RealTerm on Windows. Connect RealTerm to your Arduino's serial device, set RealTerm to 115200 baud, start the capture (input only) and send "d" to the Arduino. The dump will start immediately.

As with siboimg, this is ALPHA-QUALITY SOFTWARE. Both the companion app and the sketch are missing a lot of error checking at the moment, but that will come in the future.

The Arduino sketch was originally written by Karl with new features such as the command interpreter and block-by-block dumping added by me. The companion app is written by me with the addition of argparse.

Try it, break it, give me feedback!
