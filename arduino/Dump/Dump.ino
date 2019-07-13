// TODO: Device selection
// TODO: MD5 error checking
// ? If Port D is set, what happens to Port C?
// ?  - Is it reset to 0?
// ?  - Are just the first 5 bits reset, so the device (CS) is the same?
// ?  - Is it untouched?

#include "Physical.h"

#define BAUDRATE 115200

unsigned int curblock = 0;
byte curdev = 0;
bool is_asic4 = false;
bool direct_pin_mode = false;

SIBOSPConnection sibosp;


void dump(int _blocks) {
  SetMode(0);
  for (int _block = 0; _block < _blocks; _block++) {
    sibosp.setAddress(((unsigned long)_block << 8));
    sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 0);
    for (int b = 0; b < 256; b++) {
      Serial.write(sibosp.fetchDataFrame());
    }
  }
}

void dumpblock(int _block) {
  if (sibosp.getID() == ID_ASIC5 || sibosp.getForceASIC5()) SetMode(0);
  sibosp.setAddress(((unsigned long)_block << 8));
  sibosp.sendControlFrame(SP_SCTL_READ_MULTI_BYTE | 0);
  for (int b = 0; b < 256; b++) {
    Serial.write(sibosp.fetchDataFrame());
  }
}

void setup() {
  Serial.begin(BAUDRATE);
  Reset();
}

void loop() {
  while (Serial.available() > 0) {
   char incomingCharacter = Serial.read();
   switch (incomingCharacter) {
     case '4':
      sibosp.setForceASIC5(false);
      // Serial.print("ID:6");
      break;

     case '5':
      sibosp.setForceASIC5(true);
      // Serial.print("ID:2");
      break;

     case 'a':
     case 'A':
      Serial.write(sibosp.getID() == ID_ASIC4 ? 4 : 5);
      break;

     case 'b':
     case 'B':
      Serial.write(sibosp.getInfoByte());
      break;
 
     case 'd':
     case 'D':
      dump(sibosp.getTotalBlocks());
      break;

     case 'i':
     case 'I':
      printinfo();
      break;

     case 'f':
      dumpblock(curblock);
      break;

    //  case 'j': //jump to an address
    //   getstartaddress();
    //   break;
      
     case 'n':
      curblock++ % (sibosp.getTotalBlocks() + 1);
      break;

     case 'N':
      curblock = 0;
      curdev++ % (sibosp.getTotalDevices() + 1);
      break;

     case 'p':
      sibosp.setDirectPinMode(false);
      break;

     case 'P':
      sibosp.setDirectPinMode(true);
      break;

     case 'r':
      curblock = 0;
      break;

     case 'R':
      Reset();
    }
 }
}

void printinfo() {
  long unsigned int blocks;

  Serial.println();
  Serial.print("Pin Numbers (CLK/DATA): ");
  Serial.print(sibosp.getClockPin());
  Serial.print("/");
  Serial.println(sibosp.getDataPin());
  Serial.print("Pin Bits (CLK/DATA): ");
  Serial.print(sibosp.getClockBit(), BIN);
  Serial.print("/");
  Serial.println(sibosp.getDataBit(), BIN);
  Serial.print("Direct pin mode: ");
  if (sibosp.getDirectPinMode()) {
    Serial.println("On");
  } else {
    Serial.println("Off");
  }

  Serial.println();
  Serial.print("TYPE: ");
  switch (sibosp.getType()) {
    case 0:
      Serial.println("RAM");
      break;
    case 1:
      Serial.println("Type 1 Flash");
      break;
    case 2:
      Serial.println("Type 2 Flash");
      break;
    case 3:
      Serial.println("TBS");
      break;
    case 4:
      Serial.println("TBS");
      break;
    case 5:
      Serial.println("???");
      break;
    case 6:
      Serial.println("ROM");
      break;
    case 7:
      Serial.println("Hardware write-protected SSD");
      break;
  }
  
  Serial.print("DEVICES: ");
  Serial.println(sibosp.getTotalDevices());
  
  Serial.print("SIZE: ");
  switch (sibosp.getSize()) {
    case 0:
      Serial.println("No SSD Detected");
      return;
    case 1:
      Serial.println("32KB");
      break;
    case 2:
      Serial.println("64KB");
      break;
    case 3:
      Serial.println("128KB");
      break;
    case 4:
      Serial.println("256KB");
      break;
    case 5:
      Serial.println("512KB");
      break;
    case 6:
      Serial.println("1MB");
      break;
    case 7:
      Serial.println("2MB");
      break;
  }

  Serial.print("BLOCKS: ");
  Serial.println(sibosp.getTotalBlocks());
  Serial.print("Current block: ");
  Serial.println(curblock);

  // if (is_asic4) {
  if (sibosp.getID() == ID_ASIC4) {
    Serial.print("ASIC4 detected.");
    // if (force_asic5) Serial.print(" (ASIC5 mode forced)");
    if (sibosp.getForceASIC5()) Serial.print(" (ASIC5 mode forced)");
    Serial.println();
    Serial.print("Input Register: ");
    Serial.println(sibosp.getASIC4InputRegister());
  }

}

void Reset() {
  curblock = 0;
  curdev = 0;
  sibosp.Reset();
}

void SetMode(byte mode) {
  sibosp.sendControlFrame(SP_SCTL_WRITE_SINGLE_BYTE | 2);
  sibosp.sendDataFrame(mode & 0x0F);
}
