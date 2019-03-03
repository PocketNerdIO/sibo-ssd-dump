#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
// #include <time.h>
// #include <utime.h>

#define BAUDRATE B57600

#ifdef _WIN32
    #include <windows.h>
    const char *slash = "\\";
#else
    // Assume it's something POSIX-compliant
    #include <unistd.h>
    #include <sys/stat.h>
    #include <termios.h>
    const char *slash = "/";
#endif

#include "argparse/argparse.h"
static const char *const usage[] = {
    "sibodump [options] [[--] args]",
    "sibodump [options]",
    NULL,
};

struct {
    unsigned char infobyte;
    unsigned char type;
    unsigned char devs;
    unsigned char size;
    unsigned int  blocks;
} ssdinfo;

#ifdef _WIN32
void usleep(int us) {
    Sleep(us);
}
#endif

#ifdef _WIN32
BOOL fileexists(LPCTSTR szPath) {
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL direxists(LPCTSTR szPath) {
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL fsitemexists(LPCTSTR szPath) {
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES);
}

#else
bool fileexists(const char *filename) {
    struct stat path_stat;

    return (stat(filename, &path_stat) == 0 && S_ISREG(path_stat.st_mode));
}
bool direxists(const char *filename) {
    struct stat path_stat;

    return (stat(filename, &path_stat) == 0 && S_ISDIR(path_stat.st_mode));
}
bool fsitemexists(const char *filename) {
    struct stat path_stat;

    return (stat(filename, &path_stat) == 0);
}
#endif

//
// Serial Port Configuration
//
#ifdef _WIN32

#define B9600   CBR_9600
#define B57600  CBR_57600

int set_interface_attribs(HANDLE *fd, int speed) {
    BOOL Status;
    DCB dcbSerialParams = { 0 };
    COMMTIMEOUTS timeouts = { 0 };

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    Status = GetCommState(*fd, &dcbSerialParams);
    dcbSerialParams.BaudRate = speed;
    dcbSerialParams.ByteSize = 0;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    timeouts.ReadIntervalTimeout         = 50; // in milliseconds
    timeouts.ReadTotalTimeoutConstant    = 50; // in milliseconds
    timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds
    timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds
    timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds

    Status = SetCommState(*fd, &dcbSerialParams);
    Status = SetCommTimeouts(*fd, &timeouts);
    return 0;
}

HANDLE portopen(const char *serialdev) {
    return CreateFile(serialdev, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
}

int portsend(HANDLE *fd, char cmd) {
    return WriteFile(*fd, &cmd, 1, NULL, NULL);
}

int portread(HANDLE *fd, unsigned char *buffer) {
    return ReadFile(*fd, &buffer, 1, NULL, NULL);
}

int portflush(HANDLE *fd) {
    return PurgeComm(*fd, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

int portclose(HANDLE *fd) {
    return CloseHandle(*fd);
}
#else
int set_interface_attribs(int *fd, int speed) {
    struct termios tty;

    if (tcgetattr(*fd, &tty) < 0) {
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

    if (tcsetattr(*fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int portopen(const char *serialdev) {
    return open(serialdev, O_RDWR | O_NOCTTY);
}

int portsend(int fd, char cmd) {
    return write(fd, &cmd, 1);
}

int portread(int *fd, unsigned char *buffer) {
    return read(*fd, buffer, 1);
}

int portflush(int fd) {
    return tcflush(fd, TCIFLUSH);
}

int portclose(int fd) {
    return close(fd);
}
#endif

void GetSSDInfo(char input) {
    ssdinfo.infobyte = input;

    ssdinfo.type   = (ssdinfo.infobyte & 0b11100000) >> 5;
    ssdinfo.devs   = ((ssdinfo.infobyte & 0b00011000) >> 3) + 1;
    ssdinfo.size   = (ssdinfo.infobyte & 0b00000111);
    ssdinfo.blocks = (ssdinfo.size == 0) ? 0 : ((0b10000 << ssdinfo.size) * 4);
}

void printinfo() {
    long unsigned int blocks;
    printf("TYPE: ");
    switch (ssdinfo.type) {
        case 0:
            printf("RAM");
            break;
        case 1:
            printf("Type 1 Flash");
            break;
        case 2:
            printf("Type 2 Flash");
            break;
        case 3:
            printf("TBS");
            break;
        case 4:
            printf("TBS");
            break;
        case 5:
            printf("???");
            break;
        case 6:
            printf("ROM");
            break;
        case 7:
            printf("???");
            break;
    }
    printf("\n");

    printf("DEVICES: %d\n", ssdinfo.devs);
  
    printf("SIZE: ");
    switch (ssdinfo.size) {
        case 0:
            printf("0");
            break;
        case 1:
            printf("32KB");
            break;
        case 2:
            printf("64KB");
            break;
        case 3:
            printf("128KB");
            break;
        case 4:
            printf("256KB");
            break;
        case 5:
            printf("512KB");
            break;
        case 6:
            printf("1MB");
            break;
        case 7:
            printf("2MB");
            break;
    }
    printf("\n");

    printf("BLOCKS: %d\n", ssdinfo.blocks);
}

void dump(int fd, const char *path) {
    FILE *fp;
    unsigned char buffer;
    unsigned int i;

    printf("\nDumping to %s\n", path);

    fp = fopen(path, "wb");

    portsend(fd, 'd');
    for (i = 1; i <= ssdinfo.blocks * ssdinfo.devs * 256; i++) {
        portread(&fd, &buffer);
        fwrite(&buffer, sizeof(buffer), 1, fp);
    }

    fclose(fp);
    return;
}

void getblock(int fd, unsigned int blocknum, unsigned char *block) {
    unsigned int i;

    printf("Fetch block %d/%d\r", blocknum, ssdinfo.blocks);
    fflush(stdout);
    portsend(fd, 'f');
    for (i = 0; i <= 255; i++) {
        portread(&fd, &block[i]);
        // printf(".");
    }
}


int main (int argc, const char **argv) {
#ifdef _WIN32
    HANDLE fd;
#else
    int fd;
#endif

    int i;
    bool only_list, ignore_attributes, ignore_modtime;
    const char *serialdev = NULL, *dumppath = NULL;
    int result;
    unsigned char input;
    unsigned int curblock = 0;
    unsigned char block[256];
    FILE *fp;
    int wlen;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('s', "serial", &serialdev, "set serial device of arduino"),
        OPT_STRING('d', "dump", &dumppath, "dump to file"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nDumps SIBO SSD images to file.", "");
    argc = argparse_parse(&argparse, argc, argv);

    fd = portopen(serialdev);

#ifdef _WIN32
    fd = CreateFile(serialdev, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (fd == INVALID_HANDLE_VALUE) {
        printf("Error opening serial port %s\n", serialdev);
        exit(-1);
    }  
#else
    if (fd < 0) {
        perror(serialdev);
        exit(-1);
    }
#endif

    set_interface_attribs(&fd, B57600);
    usleep(2000000);
    portflush(fd);

    wlen = portsend(fd, 'b');
    if (wlen != 1) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }

    portread(&fd, &input);
    GetSSDInfo(input);
    printinfo();

    if (dumppath != NULL) {
        printf("\n");
        portsend(fd, 'r');
        fp = fopen(dumppath, "wb");

        for (i = 0; i < ssdinfo.blocks; i++) {
            getblock(fd, curblock, block);
            portsend(fd, 'n');
            curblock++;
            fwrite(&block, sizeof(block), 1, fp);
        }
        fclose(fp);
    }

    portclose(fd);
    printf("\n");
}