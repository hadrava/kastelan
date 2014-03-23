avr-gcc -Wall -std=gnu99 -Os -finline-functions -I. -DDEBUG_LEVEL=0 -mmcu=atmega48p -c decoder.c -o decoder.o
avr-gcc -Wall -std=gnu99 -Os -finline-functions -I. -DDEBUG_LEVEL=0 -mmcu=atmega48p -c TWI_slave.c -o TWI_slave.o
avr-gcc -Wall -std=gnu99 -Os -finline-functions -I. -DDEBUG_LEVEL=0 -mmcu=atmega48p -o decoder.elf decoder.o TWI_slave.o
rm -f decoder.hex decoder.eep.hex
avr-objcopy -j .text -j .data -O ihex decoder.elf decoder.hex
avr-size decoder.hex
