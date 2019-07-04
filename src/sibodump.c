// TODO: Device selection
// TODO: MD5 error checking
// TODO: Timeout for reads and counting of bytes
// TODO: Check serial port/device exists

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#include "statwrap.h"
#include "serialwrap.h"

#ifdef _WIN32
    #include <windows.h>
    // #include <tchar.h>
    const char *slash = "\\";

    void usleep(int us) {
        Sleep(us/1000);
    }
#else
    // Assume it's something POSIX-compliant
    #include <unistd.h>
    const char *slash = "/";
#endif

#define BAUDRATE B115200

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
    unsigned char asic;
} ssdinfo;


void GetSSDInfo(char input) {
    ssdinfo.infobyte = input;

    ssdinfo.type   = (ssdinfo.infobyte & 0b11100000) >> 5;
    ssdinfo.devs   = ((ssdinfo.infobyte & 0b00011000) >> 3) + 1;
    ssdinfo.size   = (ssdinfo.infobyte & 0b00000111);
    ssdinfo.blocks = (ssdinfo.size == 0) ? 0 : ((0b10000 << ssdinfo.size) * 4);
}

void printinfo() {
    printf("ASIC: %d\n", ssdinfo.asic);
    if (ssdinfo.infobyte == 0) {
        printf("No SSD detected.\n");
        return;
    }

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
            printf("Hardware write-protected SSD");
            break;
    }
    printf("\n");

    printf("DEVICES: %d\n", ssdinfo.devs);

    printf("SIZE: ");
    switch (ssdinfo.size) {
        case 0:
            printf("No SSD Detected");
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

void dump(SerialDevice *sd, const char *path) {
    FILE *fp;
    unsigned char buffer;
    unsigned int i;

    printf("\nDumping to %s\n", path);

    fp = fopen(path, "wb");

    portsend(sd, 'd');
    for (i = 1; i <= ssdinfo.blocks * ssdinfo.devs * 256; i++) {
        portread(sd, &buffer);
        fwrite(&buffer, sizeof(buffer), 1, fp);
    }

    fclose(fp);
    return;
}

void getblock(SerialDevice *sd, unsigned int blocknum, unsigned char curdev, unsigned char *block) {
    unsigned int i;

    printf("Fetch block %d (0 to %d, total %d) on device %d\r", blocknum, ssdinfo.blocks - 1, ssdinfo.blocks, curdev);
    fflush(stdout);
    portsend(sd, 'f');
    for (i = 0; i <= 255; i++) {
        portread(sd, &block[i]);
        // printf(".");
    }
}


int main (int argc, const char **argv) {
    SerialDevice sd;
    int i;
    const char *dumppath = NULL;
    int result;
    unsigned char buffer;
    unsigned int curblock = 0;
    unsigned char block[256];
    unsigned char curdev;
    int firstblockonly = 0;
    int allow_asic4 = 0;
    FILE *fp;
    int wlen;
    unsigned int address;

    // Introducing...
    printf("SIBODUMP: Psion SSD extraction tool\n");
    printf("(c)2019 The Last Psion Project\n\n");

    // Set up ARGPARSE
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('s', "serial", &sd.device, "set serial device of arduino"),
        OPT_STRING('d', "dump", &dumppath, "dump to file"),
        OPT_BOOLEAN('f', "firstblockonly", &firstblockonly, "only pull the first block (256 characters)"),
        OPT_BOOLEAN('4', "asic4", &allow_asic4, "Allow native ASIC4 mode for compatible SSDs (EXPERIMENTAL)"),
        // OPT_INTEGER('a', "address", &address, "Start address."),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nDumps SIBO SSD images to file.", "");
    argc = argparse_parse(&argparse, argc, argv);

    // Configure serial
    portopen(&sd);
    portcfg(&sd, BAUDRATE);
    usleep(2000000); // wait for 2 seconds
    portflush(&sd);


    // Get infobyte
    wlen = portsend(&sd, 'b'); 
    if (wlen != 1) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }

    wlen = portread(&sd, &buffer);
    if (wlen != 1) {
        printf("Error from read: %d, %d\n", wlen, errno);
    }

    GetSSDInfo(buffer);

    // Get ASIC detected
    wlen = portsend(&sd, 'a'); 
    if (wlen != 1) {
        printf("Error from write: %d, %d\n", wlen, errno);
    }

    wlen = portread(&sd, &buffer);
    if (wlen != 1) {
        printf("Error from read: %d, %d\n", wlen, errno);
    }
    ssdinfo.asic = buffer;

    printinfo();


    if (dumppath != NULL) {
        // Force ASIC4 (ID:6) (if it matters)
        if (allow_asic4 == 0) {
            printf("Forcing ASIC5 Pack Mode (ID:2)\n");
        } else {
            printf("Allowing ASIC4 Native Mode (ID:6)\n");
        }
        wlen = portsend(&sd, allow_asic4 == 0 ? '5' : '4');
        if (wlen != 1) {
            printf("Error from write: %d, %d\n", wlen, errno);
        }

        printf("\n");

        portsend(&sd, 'R');
        fp = fopen(dumppath, "wb");

        if (firstblockonly != 0) { // fake some SSD info
            ssdinfo.devs = 1;
            ssdinfo.blocks = 1;
        }
        printf("SSDINFO DEVS/BLOCKS = %d/%d\n", ssdinfo.devs, ssdinfo.blocks);

        // If provided, set the address
        // if (address != 0) {
        //     printf("Setting address to 0x%06x (%d)\n", address, address);

        //     wlen = portsend(&sd, 'j');
        //     if (wlen != 1) {
        //         printf("Error from write: %d, %d\n", wlen, errno);
        //     }
        //     wlen = portread(&sd, &buffer);
        //     if (wlen != 1) {
        //         printf("Error from read: %d, %d\n", wlen, errno);
        //     }
        //     if (buffer == '?') {
        //         // Send 4 bytes, LSB first
        //         for (i =0; i < 4; i++) {
        //             wlen = portsend(&sd, (address >> (i * 8)) | 0xFF);
        //         }
        //         if (wlen != 1) {
        //             printf("Error from write: %d, %d\n", wlen, errno);
        //         }
        //     }

        //     // Wait for OK
        //     wlen = portread(&sd, &buffer);
        //     if (wlen != 1) {
        //         printf("Error from read: %d, %d\n", wlen, errno);
        //     }
        //     if (buffer != '!') {
        //         printf("Arduino didn't like the address.\n");
        //         exit(-1);
        //     }

        // }

        for (curdev = 0; curdev < ssdinfo.devs; curdev++) {
            for (i = 0; i < ssdinfo.blocks; i++) {
                getblock(&sd, curblock, curdev, block);
                portsend(&sd, 'n');
                curblock++;
                fwrite(&block, sizeof(block), 1, fp);
            }
            portsend(&sd, 'N');
            curblock = 0;
        }

        fclose(fp);
        printf("\n");
    }

    portclose(&sd);
}