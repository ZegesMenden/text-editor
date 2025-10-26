CC= gcc

SOURCES := $(wildcard src/*.c)

LIBS := -lncurses

FLAGS := -Wall -Wpedantic

text-editor: $(SOURCES)
	$(CC) -o textedit $^ $(LIBS) $(FLAGS)

clean:
	rm -f text-editor *.o

all: text-editor