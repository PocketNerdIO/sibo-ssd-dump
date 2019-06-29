// TODO: Device selection
// TODO: MD5 error checking
// ? If Port D is set, what happens to Port C?
// ?  - Is it reset to 0?
// ?  - Are just the first 5 bits reset, so the device (CS) is the same?
// ?  - Is it untouched?

#define DATA 3
#define CLOCK 2
#define CYCLE 2

#define SP_SCTL_READ_MULTI_BYTE   0b11010000
#define SP_SCTL_READ_SINGLE_BYTE  0b11000000
#define SP_SCTL_WRITE_MULTI_BYTE  0b10010000
#define SP_SCTL_WRITE_SINGLE_BYTE 0b10000000
#define SP_SSEL                   0b01000000

#define ID_ASIC4 6
#define ID_ASIC5 2

struct {
  byte infobyte;
  byte type;
  byte devs;
  byte size;
  int  blocks;
  byte asic4inputreg;
  byte asic4devsizereg;
} ssdinfo;

unsigned int curblock = 0;
byte curdev = 0;
bool is_asic4 = false;
bool force_asic5 = true;

void dump(int _blocks) {
  SetMode(0);
  for (int _block = 0; _block < _blocks; _block++) {
    SetAddress(((unsigned long)_block << 8));
    _Control(SP_SCTL_READ_MULTI_BYTE | 0b0000);
    for (int b = 0; b < 256; b++) {
      Serial.write(_DataI());
    }
  }
}

void dumpblock(int _block) {
  if (!is_asic4 || force_asic5) SetMode(0);
  SetAddress(((unsigned long)_block << 8));
  _Control(SP_SCTL_READ_MULTI_BYTE | 0b0000);
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
}

void loop() {
  while (Serial.available() > 0) {
   char incomingCharacter = Serial.read();
   switch (incomingCharacter) {
     case '4':
      force_asic5 = false;
      // Serial.print("4OK");
      break;

     case '5':
      force_asic5 = true;
      // Serial.print("5OK");
      break;

     case 'a':
     case 'A':
      Serial.write(is_asic4 ? 4 : 5);
      break;

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

    //  case 'j': //jump to an address
    //   getstartaddress();
    //   break;
      
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
      Reset();
    }
 }
}

void printinfo() {
  long unsigned int blocks;

  Serial.println();
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
      Serial.println("Hardware write-protected SSD");
      break;
  }
  
  Serial.print("DEVICES: ");
  Serial.println(ssdinfo.devs);
  
  Serial.print("SIZE: ");
  switch (ssdinfo.size) {
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
  Serial.println(ssdinfo.blocks);

  if (is_asic4) {
    Serial.print("ASIC4 detected.");
    if (force_asic5) Serial.print(" (ASIC5 mode forced)");
    Serial.println();
    Serial.print("Input Register: ");
    Serial.println(ssdinfo.asic4inputreg);
    Serial.print("Device Size Register: ");
    Serial.println(ssdinfo.asic4devsizereg);
  }

}

void DeselectASIC() {
  _Control(SP_SSEL);
}

byte ReadByte(unsigned long address) {
  SetAddress(address);
  _Control(SP_SCTL_READ_SINGLE_BYTE | 0b0000);
  return _DataI();
}

void ReadBytes(byte* buffer, unsigned long address, int count) {
  _Control(SP_SCTL_READ_MULTI_BYTE | 0b0000);
  for (int _cx = 0; _cx < count; _cx++)
    buffer[_cx] = _DataI();
}

void Reset() {
  curblock = 0;
  curdev = 0;
  _Control(0b00000000);
  is_asic4 = SelectASIC4();
  if (!is_asic4 || force_asic5) SelectASIC5();
}

byte GetASIC4InputRegister() {
  _Control(SP_SCTL_READ_SINGLE_BYTE | 0b0001); // Register 1
  return _DataI();
}

byte GetASIC4DevSizeRegister() {
  _Control(SP_SCTL_WRITE_SINGLE_BYTE | 0b0001); // Register 1
  return _DataI();
}

bool SelectASIC4() {
  _Control(SP_SSEL | ID_ASIC4);
  _GetSSDInfo();

  if (ssdinfo.infobyte > 0) {
    ssdinfo.asic4inputreg = GetASIC4InputRegister();
    ssdinfo.asic4devsizereg = GetASIC4DevSizeRegister();
    return true;
  } else {
    ssdinfo.asic4inputreg = 0;
    ssdinfo.asic4devsizereg = 0;
    return false;
  }
}

bool SelectASIC5() {
  ssdinfo.asic4inputreg = 0;
  ssdinfo.asic4devsizereg = 0;

  _Control(SP_SSEL | ID_ASIC5);
  _GetSSDInfo();

  return (ssdinfo.infobyte > 0);
}

void _SetAddress5(unsigned long address) {
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

void _SetAddress4(unsigned long address) {
  byte a0 = address & 0xFF;
  byte a1 = (address >> 8) & 0xFF;
  byte a2 = (address >> 16) & 0xFF;
  byte a3 = ((address >> 24) & 0b00001111) | ((curdev << 4) & 0b00110000); // | 0b01000000);

  _Control(SP_SCTL_WRITE_MULTI_BYTE | 0b0011); 
  _DataO(a0);
  _DataO(a1);
  // if (address >> 8 != 0) _DataO(a1);
  if (address >> 16 != 0) _DataO(a2);
  if (address >> 24 != 0) _DataO(a3);
}

void SetAddress(unsigned long address) {
  if (is_asic4 && !force_asic5) {
    _SetAddress4(address);
  } else {
    _SetAddress5(address);
  }
}


// void getstartaddress() {
//   int i = 0;
//   unsigned long address;

//   while (i < 4) {
//     Serial.print("?");
//     while (Serial.available() > 0) {
//       char incomingCharacter = Serial.read();
//       address = (long) incomingCharacter << (i * 8) | address;       
//       i++;
//       if (i == 4) {
//         break;
//       }
//     }
//   }
//   if (address > ssdinfo.size * 1024) {
//     Serial.print('z');
//     address = 0;
//   } else {
//     Serial.print('!');
//     SetAddress(address);
//   }
// }


void SetMode(byte mode) {
  // changed from 0b10100010
  _Control(SP_SCTL_WRITE_SINGLE_BYTE | 0b0010);
  _DataO(mode & 0x0F);
}

void _Control(byte data) {
  _SendCtrlHeader();
  
  pinMode(DATA, OUTPUT);
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
    digitalWrite(CLOCK, HIGH);
    digitalWrite(CLOCK, LOW);
  }
}

void _SendBitAndLatch(uint8_t state) {
  digitalWrite(DATA, state);
  _Cycle(1);
}


// _SendIdleBit()
// Tristates the pin by setting it to INPUT to comply with Psion standards,
// then sends a clock pulse
void _SendIdleBit() {
  digitalWrite(DATA, LOW); 
  pinMode(DATA, INPUT);
  _Cycle(1);
}

// _SendDataHeader()
// Send start bit, high Select bit, then the idle bit
void _SendDataHeader() {
  pinMode(DATA, OUTPUT);
  digitalWrite(DATA, HIGH);
  _Cycle(2);
  _SendIdleBit();
}

// _SendCtrlHeader()
// Send start bit, low Select bit, then the idle bit
void _SendCtrlHeader() {
  pinMode(DATA, OUTPUT);
  digitalWrite(DATA, HIGH);
  _Cycle(1);
  digitalWrite(DATA, LOW);
  _Cycle(1);
  _SendIdleBit();
}

byte _DataI() {
  byte input = 0;

  _SendDataHeader();
  pinMode(DATA, INPUT);
  
  for (byte _dx = 0; _dx < 8; _dx++) {
    digitalWrite(CLOCK, HIGH);
    input = input | (digitalRead(DATA) << _dx);
    digitalWrite(CLOCK, LOW);
  }

  _SendIdleBit();
  return input;
}

void _DataO(byte data) { 
  _SendDataHeader();
  pinMode(DATA, OUTPUT);
  
  for (byte _dx = 0; _dx < 8; _dx++) {
    digitalWrite(DATA, ((data & (0b00000001 << _dx)) == 0) ? LOW : HIGH);
    digitalWrite(CLOCK, HIGH);
    digitalWrite(CLOCK, LOW);
  }
  
  _SendIdleBit();
}


void _GetSSDInfo() {
  ssdinfo.infobyte = _DataI();

  ssdinfo.type   = (ssdinfo.infobyte & 0b11100000) >> 5;
  ssdinfo.devs   = ((ssdinfo.infobyte & 0b00011000) >> 3) + 1;
  ssdinfo.size   = (ssdinfo.infobyte & 0b00000111);
  ssdinfo.blocks = (ssdinfo.size == 0) ? 0 : ((0b10000 << ssdinfo.size) * 4);
}


void _Null() {
  pinMode(DATA, INPUT);
  _Cycle(12);
}