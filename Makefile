CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET := ioctl_demo
KERNEL_EXAMPLE_DIR := kernel_module

.PHONY: all run kernel-example kernel-client kernel-demo kernel-load kernel-unload clean

all: $(TARGET)

$(TARGET): ioctl_demo.c
	$(CC) $(CFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

kernel-example:
	$(MAKE) -C $(KERNEL_EXAMPLE_DIR) all

kernel-client:
	$(MAKE) -C $(KERNEL_EXAMPLE_DIR) client

kernel-demo:
	$(MAKE) -C $(KERNEL_EXAMPLE_DIR) demo

kernel-load:
	$(MAKE) -C $(KERNEL_EXAMPLE_DIR) load

kernel-unload:
	$(MAKE) -C $(KERNEL_EXAMPLE_DIR) unload

clean:
	rm -f $(TARGET)
	$(MAKE) -C $(KERNEL_EXAMPLE_DIR) clean
