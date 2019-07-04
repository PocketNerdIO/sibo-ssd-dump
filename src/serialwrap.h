#ifndef LIBSERIALWRAP
#define LIBSERIALWRAP

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <tchar.h>
    #define B9600    CBR_9600
    #define B57600   CBR_57600
    #define B115200  CBR_115200
#else
    #include <unistd.h>
    #include <termios.h>
#endif


#ifdef _WIN32
    typedef struct {
        HANDLE portHandle;
        const char *device;
    } SerialDevice;
#else
    typedef struct {
        int fd;
        const char *device;
        struct termios tty;
    } SerialDevice;
#endif

int portopen(SerialDevice *sd);
int portcfg(SerialDevice *sd, int speed);
int portsend(SerialDevice *sd, char buffer);
int portread(SerialDevice *sd, unsigned char *buffer);
int portflush(SerialDevice *sd);
int portclose(SerialDevice *sd);

#endif