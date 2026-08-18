CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET := ioctl_demo

.PHONY: all run clean

all: $(TARGET)

$(TARGET): ioctl_demo.c
	$(CC) $(CFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
