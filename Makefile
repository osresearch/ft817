CFLAGS=-c -Wall

all: ft817 read-eeprom

ft817: ft817.o util.o tune.o

read-eeprom: read-eeprom.o ft817.o util.o

ft817.o: ft817.c

read-eeprom.o: read-eeprom.c

util.o: util.c

tune.o: tune.c

clane:
        rm -rf *.o ft817 read-eeprom
