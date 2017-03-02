CC=gcc
CFLAGS=-c -Wall
CFLAGS+=`pkg-config --cflags pocketsphinx sphinxbase`
CFLAGS+=-DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\"
LDFLAGS=`pkg-config --libs pocketsphinx sphinxbase`
LDFLAGS+=-ljson-c -luuid -lxaal
SOURCES=stt_module.c smtc_module.c dummy_commander.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=voice_commandor

.PHONY: default
default: all

all: $(EXECUTABLE) dummyLamp

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

dummyLamp: dummyLamp.o
	$(CC) dummyLamp.o -o $@ $(LDFLAGS)

.c.o:
#	echo $(CFLAGS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE) dummyLamp
