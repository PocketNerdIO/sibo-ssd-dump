#include <Arduino.h>

#ifndef _PHYSICAL_H
#define _PHYSICAL_H


// #include "BoardDetect.h"

#define SP_SCTL_READ_MULTI_BYTE   0b11010000
#define SP_SCTL_READ_SINGLE_BYTE  0b11000000
#define SP_SCTL_WRITE_MULTI_BYTE  0b10010000
#define SP_SCTL_WRITE_SINGLE_BYTE 0b10000000
#define SP_SSEL                   0b01000000

#define ID_ASIC4 6
#define ID_ASIC5 2

#define TRISTATE_DATA_PIN true



class SIBOSPConnection {
    public:
        SIBOSPConnection();

        void setForceASIC5(bool flag);
        bool getForceASIC5();
        void setForcePinMode(bool flag);

        byte getID();
        // void setID(byte id);
        
        byte getInfoByte();
        byte getType();
        byte getTotalDevices();
        int  getCurrentDevice();
        byte getSize();
        int  getTotalBlocks();
        int  getCurrentBlock();

        bool setClockPin(byte pin);
        bool setDataPin(byte pin);
        void setDirectPinMode(bool flag);
        byte getClockPin();
        byte getDataPin();
        byte getClockBit();
        byte getDataBit();
        bool getDirectPinMode();

        // Physical frames
        void sendControlFrame(byte data);
        void sendDataFrame(byte data);
        byte fetchDataFrame();

        void sendNullFrame();
        void deselectASIC();

        // Control Commands
        void setAddress(unsigned long);
        void useLastSetAddress();

        byte getASIC4InputRegister();
        void sendASIC4DeviceSizeRegister();

        void Reset();

    private:
        // Flags
        bool _force_asic5;
        bool _direct_pin_mode;

        // Device Info
        byte _id;
        byte _infobyte;
        byte _type;
        byte _devices;
        byte _size;
        int  _blocks;
        byte _asic4inputreg;
        byte _asic4devsizereg;

        byte _clock_pin;
        byte _data_pin;
        byte _clock_bit;
        byte _data_bit;

        // Address
        unsigned long _cur_address;
        unsigned long _last_setAddress;
        byte _cur_device;

        // Frame headers and footers (idle bit)
        void _SendDataHeader(bool is_input_frame = false);
        void _SendControlHeader();
        void _SendIdleBit(bool is_input_frame = false);
        void _SendPayload(byte data);

        // Pin flipping
        void _DataPinWrite(byte val);
        void _DataPinMode(byte mode);
        void _ClockPinReset();
        void _ClockCycle(byte cycles);

        void _FetchSSDInfo();

        // Address management
        void _SetAddress4(unsigned long);
        void _SetAddress5(unsigned long);
        
        // ASIC4 specific registers
        byte _asic4_input_register;
        // byte _asic4_device_size_register;
        bool _FetchASIC4ExtraInfo();
        void _ResetASIC4ExtraInfo();
};

#endif