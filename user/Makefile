CC = gcc
LD = ld
CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
         -nostdlib -static -O2 -Wall -Wextra -std=gnu11
LDFLAGS = -T nettest.ld -nostdlib -static -z max-page-size=0x1000

BUILD = ../build/user
TARGET = $(BUILD)/nettest
TARGET_SH = $(BUILD)/sh

.PHONY: all clean
all: $(TARGET) $(TARGET_SH)

$(BUILD):
	mkdir -p $(BUILD)

$(TARGET): nettest.c nettest.ld | $(BUILD)
	$(CC) $(CFLAGS) -c nettest.c -o $(BUILD)/nettest.o
	$(LD) $(LDFLAGS) $(BUILD)/nettest.o -o $(TARGET)

$(TARGET_SH): sh.c nettest.ld | $(BUILD)
	$(CC) $(CFLAGS) -c sh.c -o $(BUILD)/sh.o
	$(LD) $(LDFLAGS) $(BUILD)/sh.o -o $(TARGET_SH)

clean:
	rm -f $(BUILD)/nettest.o $(BUILD)/sh.o $(TARGET) $(TARGET_SH)
