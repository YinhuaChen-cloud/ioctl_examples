#ifndef IOCTL_SHARED_H
#define IOCTL_SHARED_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define IOCTL_DEMO_DEVICE_NAME "ioctl_demo"
#define IOCTL_DEMO_DEVICE_PATH "/dev/" IOCTL_DEMO_DEVICE_NAME
#define IOCTL_DEMO_MAX_DATA 128

/*
 * This header is shared by the kernel module and the user-space client.
 * Fixed-width Linux types keep the ioctl ABI stable across architectures.
 */
struct ioctl_demo_message {
    __u32 length;
    char data[IOCTL_DEMO_MAX_DATA];
};

struct ioctl_demo_stats {
    __u64 set_count;
    __u64 get_count;
    __u64 clear_count;
    __u32 current_length;
    __u32 open_count;
};

#define IOCTL_DEMO_MAGIC 'I'

#define IOCTL_DEMO_SET_MESSAGE \
    _IOW(IOCTL_DEMO_MAGIC, 1, struct ioctl_demo_message)
#define IOCTL_DEMO_GET_MESSAGE \
    _IOR(IOCTL_DEMO_MAGIC, 2, struct ioctl_demo_message)
#define IOCTL_DEMO_CLEAR _IO(IOCTL_DEMO_MAGIC, 3)
#define IOCTL_DEMO_GET_STATS \
    _IOR(IOCTL_DEMO_MAGIC, 4, struct ioctl_demo_stats)

#endif
