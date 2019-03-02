CC=gcc

all: sibodump

sibodump: argparse/argparse.c sibodump.c
	$(CC) argparse/argparse.c sibodump.c -o sibodump

mingw32: argparse/argparse.c sibodump.c
	i686-w64-mingw32-clang argparse/argparse.c sibodump.c -o bin/win32/sibodump.exe

mingw64: argparse/argparse.c sibodump.c
	x86_64-w64-mingw32-clang argparse/argparse.c sibodump.c -o bin/win64/sibodump.exe
