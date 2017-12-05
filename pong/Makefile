# makfile configuration
CPU             	= msp430g2553
CFLAGS          	= -mmcu=${CPU} -Os -I../h
LDFLAGS		= -L../lib -L/opt/ti/msp430_gcc/include/ 

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-gcc -mmcu=${CPU} -c


all:pong.elf

#additional rules for files                    speakerState.s if assem
pong.elf: ${COMMON_OBJECTS} pong.o speaker.o speakerState.c wdt_handler.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^ -lTimer -lLcd -lShape -lCircle -lp2sw

load: pong.elf
	mspdebug rf2500 "prog $^"

clean:
	rm -f *.o *.elf
