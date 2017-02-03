CC=gcc
CFLAGS=-c -Wall
CFLAGS+=`pkg-config --cflags pocketsphinx sphinxbase`
CFLAGS+=-DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\"
LDFLAGS=`pkg-config --libs pocketsphinx sphinxbase`
SOURCES=hello_world.c json_parser.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=hello_world

.PHONY: default
default: all

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	echo $(CFLAGS)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXECUTABLE)