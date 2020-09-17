###############################################################
#
# Purpose: Makefile for "M-JPEG Streamer"
# Author.: Tom Stoeveken (TST)
# Version: 0.3
# License: GPL
#
###############################################################

CC =  /home/andrei/tmp/xiaomi_camera_360/buildroot/buildroot-2020.02.1/output/host/bin/arm-buildroot-linux-uclibcgnueabihf-gcc

OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

CFLAGS += -O2 -DLINUX -shared -fPIC -march=armv7-a
#CFLAGS += -DDEBUG
LFLAGS += -L./libs -l:libev.so -l:libshbf.so.0.2 -l:libshbfev.so.0.2 -l:ld-uClibc.so.1 -luClibc-1.0.26 -Wl,--dynamic-linker=/lib/ld-uClibc.so.0

all: input_snapshot.so

clean:
	rm -f *.a *.o core *~ *.so *.lo

input_snapshot.so: $(OTHER_HEADERS) input_snapshot.c
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ input_snapshot.c
