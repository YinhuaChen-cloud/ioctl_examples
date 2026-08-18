ifneq ($(KERNELRELEASE),)
obj-m := ioctl_kmod.o
else

KDIR ?= /lib/modules/$(shell uname -r)/build
CC ?= cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2
CLIENT := ioctl_client

.PHONY: all module client load unload demo clean

all: module client

module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

client: $(CLIENT)

$(CLIENT): ioctl_client.c ioctl_shared.h
	$(CC) $(CFLAGS) ioctl_client.c -o $@

load: module
	sudo insmod ioctl_kmod.ko

unload:
	sudo rmmod ioctl_kmod

demo: $(CLIENT)
	./$(CLIENT) demo

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -f $(CLIENT)

endif
