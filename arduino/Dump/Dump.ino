// TODO: Device selection
// TODO: Detect ASIC4
// TODO: Force ASIC5 mode
// TODO: MD5 error checking
// ? If Port D is set, what happens to Port C?
// ?  - Is it reset to 0?
// ?  - Are just the first 5 bits reset, so the device (CS) is the same?
// ?  - Is it untouched?

#define DATA 3
#define CLOCK 2
#define CYCLE 20

#define SP_SCTL_READ_MULTI_BYTE   0b11010000
#define SP_SCTL_READ_SINGLE_BYTE  0b11000000
#define SP_SCTL_WRITE_MULTI_BYTE  0b10010000
#define SP_SCTL_WRITE_SINGLE_BYTE 0b10000000
#define SP_SSEL                   0b01000000

struct {
  byte infobyte;
  byte type;
  byte devs;
  byte size;
  int  blocks;
} ssdinfo;

unsigned int curblock = 0;
byte curdev = 0;

void dump(int _blocks) {
  SetMode(0);
  for (int _block = 0; _block < _blocks; _block++) {
    SetAddress5(((long)_block << 8));
    _Control(SP_SCTL_READ_MULTI_BYTE | 0b0000);
    for (int b = 0; b < 256; b++) {
      Serial.write(_DataI());
    }
  }
}

void dumpblock(int _block) {
  SetMode(0);
  SetAddress5(((long)_block << 8));
  _Control(0b11010000);
  for (int b = 0; b < 256; b++) {
    Serial.write(_DataI());
  }
}

void setup() {
  Serial.begin(57600);
  pinMode(CLOCK, OUTPUT);
  pinMode(DATA, INPUT);
  digitalWrite(CLOCK, LOW);
  
  Reset();
  SelectASIC5();

  GetSSDInfo();
}

void loop() {
  while (Serial.available() > 0) {
   char incomingCharacter = Serial.read();
   switch (incomingCharacter) {
     case 'b':
     case 'B':
      Serial.write(ssdinfo.infobyte);
      break;
 
     case 'd':
     case 'D':
      dump(ssdinfo.blocks);
      break;

     case 'i':
     case 'I':
      printinfo();
      break;

     case 'f':
      dumpblock(curblock);
      break;
      
     case 'n':
      curblock++ % (ssdinfo.blocks + 1);
      break;

     case 'N':
      curblock = 0;
      curdev++ % (ssdinfo.devs + 1);
      break;

     case 'r':
      curblock = 0;
      break;

     case 'R':
      curblock = 0;
      curdev = 0;
      GetSSDInfo();
    }
 }
}

void printinfo() {
  long unsigned int blocks;
  Serial.print("TYPE: ");
  switch (ssdinfo.type) {
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
      Serial.println("???");
      break;
  }
  
  Serial.print("DEVICES: ");
  Serial.println(ssdinfo.devs);
  
  Serial.print("SIZE: ");
  switch (ssdinfo.size) {
    case 0:
      Serial.println("0");
      break;
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
  Serial.println(ssdinfo.blocks);
}

void DeselectASIC() {
  _Control(SP_SSEL);
}

byte ReadByte(unsigned long address) {
  SetAddress5(address);
  _Control(SP_SCTL_READ_SINGLE_BYTE | 0b0000);
  return _DataI();
}

void ReadBytes(byte* buffer, unsigned long address, int count) {
  _Control(SP_SCTL_READ_MULTI_BYTE | 0b0000);
  for (int _cx = 0; _cx < count; _cx++)
    buffer[_cx] = _DataI();
}

void Reset() {
  _Control(0b00000000);
}

void SelectASIC4() {
  _Control(SP_SSEL | 6);
}

void SelectASIC5() {
  _Control(SP_SSEL | 2);
}

void SetAddress5(unsigned long address) {
  byte a0 = address & 0xFF;
  byte a1 = (address >> 8) & 0xFF; 
  byte a2 = ((address >> 16) & 0b00011111) | ((curdev << 6) & 0b11000000);

  // ports D + C????
  _Control(SP_SCTL_WRITE_MULTI_BYTE | 0b0011); 
  _DataO(a1);
  _DataO(a2);
  
  // send port b address
  _Control(SP_SCTL_WRITE_SINGLE_BYTE | 0b0001);
  _DataO(a0);
}

void SetMode(byte mode) {
  // changed from 0b10100010
  _Control(SP_SCTL_WRITE_SINGLE_BYTE | 0b0010);
  _DataO(mode & 0x0F);
}

void _Control(byte data) {
  pinMode(DATA, OUTPUT);
  digitalWrite(DATA, HIGH);
  _Cycle(1);
  digitalWrite(DATA, LOW);
  _Cycle(2);
  
  for (byte _dx = 0; _dx < 8; _dx++) {
    digitalWrite(DATA, ((data & (0b00000001 << _dx)) == 0) ? LOW : HIGH);
    _Cycle(1);
  }
  
  digitalWrite(DATA, LOW);
  _Cycle(1);
  
  pinMode(DATA, INPUT);
}

void _Cycle(byte cycles) {
  for (byte _cx = 0; _cx < cycles; _cx++) {
    delayMicroseconds(CYCLE / 5);
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(CYCLE);
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(CYCLE - (CYCLE / 5));
  }
}

byte _DataI() {
  pinMode(DATA, OUTPUT);
  digitalWrite(DATA, HIGH);
  _Cycle(2);
  pinMode(DATA, INPUT);
  _Cycle(1);
  
  byte input = 0;
  for (byte _dx = 0; _dx < 8; _dx++) {
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(CYCLE / 5);
    input = input | (digitalRead(DATA) << _dx);
    delayMicroseconds(CYCLE - (CYCLE /5));
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(CYCLE);
  }
  _Cycle(1);
  return input;
}

void GetSSDInfo() {
  ssdinfo.infobyte = _DataI();

  ssdinfo.type   = (ssdinfo.infobyte & 0b11100000) >> 5;
  ssdinfo.devs   = ((ssdinfo.infobyte & 0b00011000) >> 3) + 1;
  ssdinfo.size   = (ssdinfo.infobyte & 0b00000111);
  ssdinfo.blocks = (ssdinfo.size == 0) ? 0 : ((0b10000 << ssdinfo.size) * 4);
}

void _DataO(byte data) { 
  pinMode(DATA, OUTPUT);
  digitalWrite(DATA, HIGH);
  _Cycle(2);
  digitalWrite(DATA, LOW);
  _Cycle(1);
  
  for (byte _dx = 0; _dx < 8; _dx++) {
    digitalWrite(DATA, ((data & (0b00000001 << _dx)) == 0) ? LOW : HIGH);
    _Cycle(1);
  }
  
  digitalWrite(DATA, LOW);
  _Cycle(1);
  
  pinMode(DATA, INPUT);
}

void _Null() {
  pinMode(DATA, INPUT);
  _Cycle(12);
}
