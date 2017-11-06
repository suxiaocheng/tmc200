CC:=arm-linux-gnueabi-gcc
CFLAGS:=-g -c
LDFLAGS:=-static

SRC := 7816.c tmc200.c uart.c
OBJ := $(patsubst %.c,%.o,${SRC})

all:tmc200 reg

#include depend

tmc200: 7816.o tmc200.o uart.o
	$(CC) $(LDFLAGS) -o $@ $^

reg: reg.o
	$(CC) $(LDFLAGS) -o $@ $^

depend:
	${CC} -MM ${SRC} > depend

.PHONY: clean
clean:
	rm *.o ${OBJ} depend -rf
