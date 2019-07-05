#include "serialwrap.h"


#ifdef _WIN32

int portopen(SerialDevice *sd) {
    sd->portHandle = CreateFile(sd->device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (sd->portHandle == INVALID_HANDLE_VALUE) {
        fputs("Unable to open serial port: ", stderr);
        // printf("Error opening serial port %s\n", sd->device);
        sd->portHandle = NULL;
        return 2;
    // } else if (sd->portHandle == ERROR_FILE_NOT_FOUND) {
    //     fputs("Could not find serial port", stderr);
    //     sd->portHandle = NULL;
    //     return 1;
    }
    return 0;
}

#else

int portopen(SerialDevice *sd) {
    // Does the device exist?
    struct stat path_stat;
    if (stat(sd->device , &path_stat) != 0) {
        perror(sd->device);
        return 1;
    }

    sd->fd = open(sd->device, O_RDWR | O_NOCTTY);
    if (sd->fd < 0) {
        perror(sd->device);
        exit(-1);
    }
    return 0;
}

#endif



#ifdef _WIN32
int portcfg(SerialDevice *sd, int speed) {
    BOOL Success;
    DCB dcb;
    COMMTIMEOUTS timeouts = { 0 };

    dcb.DCBlength = sizeof(dcb);
    Success = GetCommState(sd->portHandle, &dcb);
    if (!Success) {
        printf("GetCommState failed with error %ld.\n", GetLastError());
        return 2;
    }

    dcb.BaudRate = speed; // TODO: Test alternative methods
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity   = NOPARITY;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    
    timeouts.ReadIntervalTimeout         = 50; // in milliseconds
    timeouts.ReadTotalTimeoutConstant    = 50; // in milliseconds
    timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds
    timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds
    timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds

    // PrintCommState(dcb);

    Success = SetCommState(sd->portHandle, &dcb);
    if (!Success) {
        printf("SetCommState failed with error %ld.\n", GetLastError());
        return 1;
    }
    Success = GetCommState(sd->portHandle, &dcb);
    if (!Success) {
        printf("Second GetCommState failed with error %ld.\n", GetLastError());
        return 3;
    }
    // PrintCommState(dcb);


    Success = SetCommTimeouts(sd->portHandle, &timeouts);
    return 0;
}

#else

int portcfg(SerialDevice *sd, int speed) {
    struct termios tty;

    if (tcgetattr(sd->fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(sd->fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

#endif



#ifdef _WIN32

int portsend(SerialDevice *sd, char buffer) {
    DWORD count;
    // printf("Trying to send '%s' (%x)...\n", &buffer, buffer);
    WriteFile(sd->portHandle, &buffer, 1, &count, NULL);
    return (int) count;
}

#else

int portsend(SerialDevice *sd, char buffer) {
    return write(sd->fd, &buffer, 1);
}

#endif



#ifdef _WIN32

int portread(SerialDevice *sd, unsigned char *buffer) {
    DWORD count;
    ReadFile(sd->portHandle, buffer, 1, &count, NULL);
    return (int) count;
}

#else

int portread(SerialDevice *sd, unsigned char *buffer) {
    return read(sd->fd, buffer, 1);
}

#endif



#ifdef _WIN32
int portflush(SerialDevice *sd) {
    return PurgeComm(sd->portHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

#else

int portflush(SerialDevice *sd) {
    return tcflush(sd->fd, TCIFLUSH);
}

#endif



#if _WIN32

int portclose(SerialDevice *sd) {
    return CloseHandle(sd->portHandle);
}

#else

int portclose(SerialDevice *sd) {
    return close(sd->fd);
}

#endif