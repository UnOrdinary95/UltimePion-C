CC = gcc
CFLAGS = -std=c17 -Wall

TARGET = src/ultime_pion_mlv
SOURCE = src/ultime_pion_mlv.c

.PHONY: all clean run

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(shell pkg-config --cflags MLV) $(LDFLAGS) $(shell pkg-config --libs-only-other --libs-only-L MLV) $< $(shell pkg-config --libs-only-l MLV) $(LDLIBS) -o $@

clean:
	$(RM) $(TARGET)
