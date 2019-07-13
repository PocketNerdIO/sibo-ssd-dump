// Psion SSD Physical Library
// Original version by Richard Warrender

#include "Physical.h"

SIBOSPConnection::SIBOSPConnection() {
    _direct_pin_mode = false;
    _force_asic5 = false;
    setDataPin(3);
    setClockPin(2);
    Reset();
}


// Headers and footers (idle bit) for frames

// _SendDataHeader()
// Send start bit, high ctl bit, then the idle bit
void SIBOSPConnection::_SendDataHeader(bool is_input_frame = false) {
    _DataPinMode(OUTPUT);
    _DataPinWrite(HIGH);
    _ClockCycle(2);
    _SendIdleBit(TRISTATE_DATA_PIN);
}

// Send start bit, low ctl bit, then the idle bit
void SIBOSPConnection::_SendControlHeader() {
    _DataPinMode(OUTPUT);
    _DataPinWrite(HIGH);
    _ClockCycle(1);

    _DataPinWrite(LOW);
    _ClockCycle(1);

    _SendIdleBit();
}

void SIBOSPConnection::_SendIdleBit(bool is_input_frame = false) {
    if (is_input_frame) {
        // As per Psion spec, if switching mode for input frame then
        // controller sets input at end of cycle 2 and cycle 11
        _DataPinMode(INPUT);
    } else {
        _DataPinWrite(LOW);
    }
    
    _ClockCycle(1);
}


// Pin flipping utility methods

void SIBOSPConnection::_DataPinWrite(uint8_t val) {
    if (_direct_pin_mode) { // && BOARD == "Uno"
        if (val == LOW) {
            PORTD &= ~_data_bit;
        } else {
            PORTD |= _data_bit;
        }
    } else {
        digitalWrite(_data_pin, val);
    }
}

void SIBOSPConnection::_DataPinMode(uint8_t mode) {
    if (_direct_pin_mode) { // && BOARD == "Uno"
        if (mode == INPUT) {
            PORTD &= ~_data_bit;
            DDRD &= ~_data_bit;
        } else if (mode == OUTPUT) {
            DDRD |= _data_bit;
        }
    } else {
        pinMode(_data_pin, mode);
    }
}

void SIBOSPConnection::_ClockPinReset() {
    if (_direct_pin_mode) { // && BOARD == "Uno"
        DDRD |= _clock_bit;
    } else {
        pinMode(_clock_pin, OUTPUT);
    }
}


// Move Clock Pin HIGH and then LOW to indicate a cycle
void SIBOSPConnection::_ClockCycle(byte cycles) {
    if (_direct_pin_mode) { // && BOARD == "Uno"
        for (byte _cx = 0; _cx < cycles; _cx++) {
            PORTD |= _clock_bit;
            PORTD &= ~_clock_bit;
        }
    } else {
        for (byte _cx = 0; _cx < cycles; _cx++) {
            digitalWrite(_clock_pin, HIGH);
            digitalWrite(_clock_pin, LOW);
        }
    }
}


// Frame functions

/* Frame structure
   Bit  0   1   2   3   4   5   6   7   8   9   10  11
        ST  CTL I1  D0  D1  D2  D3  D4  D5  D6  D7  I2
   - 
   ST   = Start Bit
   CTL  = Control Bit
   I1   = Idle bit 
   D0-7 = Data bits
   I2   = Idle bit
*/

// Transmitted by controller to ensure all slaves are synchronised
// Tristate the DATA pin, then send 12 CLOCK pulses.
void SIBOSPConnection::sendNullFrame() {
    _DataPinMode(INPUT);
    _ClockCycle(12);
}

// Transmitted by controller. Output the whole frame
void SIBOSPConnection::sendControlFrame(byte data) {
    _SendControlHeader();
    _SendPayload(data);
    _SendIdleBit();
}

// Received by controller from slave, input during header
byte SIBOSPConnection::fetchDataFrame() {
    _SendDataHeader(TRISTATE_DATA_PIN);

    // _DataPinMode(INPUT);
    int input = 0;
    if (_direct_pin_mode) { // && BOARD == "Uno"
        for (byte _dx = 0; _dx < 8; _dx++) {
            PORTD |= _clock_bit;
            input |= (((PIND & _data_bit) == _data_bit) << _dx);
            PORTD ^= _clock_bit;
        }
    } else {
        for (byte _dx = 0; _dx < 8; _dx++) {
            digitalWrite(_clock_pin, HIGH);
            input = input | (digitalRead(_data_pin) << _dx);
            digitalWrite(_clock_pin, LOW);
        }
    }
    _SendIdleBit(true);
    return input;
}


// Transmitted from the controller to slave
void SIBOSPConnection::sendDataFrame(byte data) { 
    _SendDataHeader(false);
    _SendPayload(data);
    _SendIdleBit();
}

// Deselects slave without resetting it
void SIBOSPConnection::deselectASIC() {
    sendControlFrame(SP_SSEL);
}

void SIBOSPConnection::_SendPayload(byte data) {
    int output = 0;
    _DataPinMode(OUTPUT);
    for (byte _dx = 0; _dx < 8; _dx++) {
        output = ((data & (0b00000001 << _dx)) == 0) ? LOW : HIGH;
        _DataPinWrite(output);
        _ClockCycle(1);
    }    
}


// Address Control

void SIBOSPConnection::_SetAddress5(unsigned long address) {
    byte a0 = address & 0xFF;
    byte a1 = (address >> 8) & 0xFF; 
    byte a2 = ((address >> 16) & 0b00011111) | ((_cur_device << 6) & 0b11000000);

    // ports D + C
    sendControlFrame(SP_SCTL_WRITE_MULTI_BYTE | 3); 
    sendDataFrame(a1);
    sendDataFrame(a2);
    
    // send port B address
    sendControlFrame(SP_SCTL_WRITE_SINGLE_BYTE | 1);
    sendDataFrame(a0);
}

void SIBOSPConnection::_SetAddress4(unsigned long address) {
    byte a0 = address & 0xFF;
    byte a1 = (address >> 8) & 0xFF;
    byte a2 = (address >> 16) & 0xFF;
    byte a3 = ((address >> 24) & 0b00001111) | ((_cur_device << 4) & 0b00110000); // | 0b01000000);

    sendControlFrame(SP_SCTL_WRITE_MULTI_BYTE | 3);
    // Observed: The first two address bytes are always sent, even if 0x00
    sendDataFrame(a0);
    sendDataFrame(a1);
    // Observed: The last two address bytes are only sent if populated
    if (address >> 16 != 0) sendDataFrame(a2);
    if (address >> 24 != 0) sendDataFrame(a3);
}

void SIBOSPConnection::setAddress(unsigned long address) {
    if (_id == ID_ASIC4 && !_force_asic5) {
        _SetAddress4(address);
    } else {
        _SetAddress5(address);
    }
    _cur_address = address;
}


// Getters
byte SIBOSPConnection::getInfoByte() {
    return _infobyte;
}

byte SIBOSPConnection::getSize() {
    return _size;
}

byte SIBOSPConnection::getTotalDevices() {
    return _devices;
}

int SIBOSPConnection::getTotalBlocks() {
    return _blocks;
}

byte SIBOSPConnection::getType() {
    return _type;
}

byte SIBOSPConnection::getASIC4InputRegister() {
    return _asic4_input_register;
}

byte SIBOSPConnection::getClockPin() {
    return _clock_pin;
}

byte SIBOSPConnection::getDataPin() {
    return _data_pin;
}

byte SIBOSPConnection::getClockBit() {
    return _clock_bit;
}

byte SIBOSPConnection::getDataBit() {
    return _data_bit;
}

bool SIBOSPConnection::getDirectPinMode() {
    return _direct_pin_mode;
}

// get/setID: Device ID used to communicate with ASIC
// TODO: If this is zero, IDs will be picked in order ID:6, then ID:2 
byte SIBOSPConnection::getID() {
    return _id;
}

// void SIBOSPConnection::setID(byte id) {
//     _id = id & 0b00111111;
// }

void SIBOSPConnection::setForceASIC5(bool flag) {
    _force_asic5 = flag;
}

bool SIBOSPConnection::getForceASIC5() {
    return _force_asic5;
}

bool SIBOSPConnection::setDataPin(byte pin) {
    // if (_clock_pin != pin) {
        _data_pin = pin;
        _data_bit = (1 << (_data_pin));
        return true;
    // }
    // return false;
}

bool SIBOSPConnection::setClockPin(byte pin) {
    // if (_data_pin != pin % 8) {
        _clock_pin = pin;
        _clock_bit = (1 << _clock_pin);
        return true;
    // }
    // return false;
    _ClockPinReset();
}

void SIBOSPConnection::setDirectPinMode(bool flag) {
    _direct_pin_mode = flag;
}


// Reset

void SIBOSPConnection::Reset() {
    _id = 0;
    _ClockPinReset();
    _DataPinMode(INPUT);

    // Send null frame, then reset frame, then null frame
    // (undocumented: this is what all SIBO machines do)
    sendNullFrame();
    sendControlFrame(0); // Reset frame
    sendNullFrame();

    // Select ASIC4 (ID:6)
    sendControlFrame(SP_SSEL | ID_ASIC4);
    _FetchSSDInfo();
    if (_infobyte != 0) { // if it's ASIC4
        _id = ID_ASIC4;
        _FetchASIC4ExtraInfo();
        sendASIC4DeviceSizeRegister();  // Send size back to ASIC4, because reasons
    }

    if (_infobyte == 0 || _force_asic5) {
        sendControlFrame(SP_SSEL | ID_ASIC5);
        _FetchSSDInfo();
        if (_infobyte != 0) { // if it's ASIC5
            _id = ID_ASIC5;
        }
    }
}

void SIBOSPConnection::sendASIC4DeviceSizeRegister() {
    sendControlFrame(SP_SCTL_WRITE_SINGLE_BYTE | 1);
    sendDataFrame(_size);
    // sendDataFrame(0x0f);
}

void SIBOSPConnection::_FetchSSDInfo() {
  _infobyte = fetchDataFrame();

  _type    = (_infobyte & 0b11100000) >> 5;
  _devices = ((_infobyte & 0b00011000) >> 3) + 1;
  _size    = (_infobyte & 0b00000111);
  _blocks  = (_size == 0) ? 0 : ((0b10000 << _size) * 4);
}

bool SIBOSPConnection::_FetchASIC4ExtraInfo() {
    if (_id == ID_ASIC4) {
        sendControlFrame(SP_SCTL_READ_SINGLE_BYTE | 1);
        _asic4_input_register = fetchDataFrame();
        return true;
    } else {
        _ResetASIC4ExtraInfo();
        return false;
    }
}

void SIBOSPConnection::_ResetASIC4ExtraInfo() {
    _asic4_input_register = 0;
    // _asic4_device_size_register = 0;
}

