#ifndef IOCTL_SHARED_H
#define IOCTL_SHARED_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define IOCTL_DEMO_DEVICE_NAME "ioctl_demo"
#define IOCTL_DEMO_DEVICE_PATH "/dev/" IOCTL_DEMO_DEVICE_NAME
#define IOCTL_DEMO_MAX_DATA 128

/*
 * 此头文件由内核模块和用户空间客户端共同使用，用于约定 ioctl ABI。
 * 使用 Linux 固定宽度类型，避免不同架构上的类型宽度差异破坏数据布局。
 */
struct ioctl_demo_message {
    /* data 中有效数据的字节数，不包含额外的字符串结束符。 */
    __u32 length;
    /* 消息按原始字节保存，因此内容不要求以 '\0' 结尾。 */
    char data[IOCTL_DEMO_MAX_DATA];
};

/* 设备运行期间维护的操作统计信息。 */
struct ioctl_demo_stats {
    /* 驱动处理三类修改/查询命令的累计次数。 */
    __u64 set_count;
    __u64 get_count;
    __u64 clear_count;
    /* 当前保存的消息长度，以及此刻打开设备的文件描述符数量。 */
    __u32 current_length;
    __u32 open_count;
};

/* ioctl 命令类型标识，用于排除发给其他设备族的命令。 */
#define IOCTL_DEMO_MAGIC 'I'

/*
 * _IOW：用户空间向内核写入参数；_IOR：用户空间从内核读取结果；
 * _IO：命令本身不携带参数。编号 1~4 在本设备内必须保持唯一。
 */
#define IOCTL_DEMO_SET_MESSAGE \
    _IOW(IOCTL_DEMO_MAGIC, 1, struct ioctl_demo_message)
#define IOCTL_DEMO_GET_MESSAGE \
    _IOR(IOCTL_DEMO_MAGIC, 2, struct ioctl_demo_message)
#define IOCTL_DEMO_CLEAR _IO(IOCTL_DEMO_MAGIC, 3)
#define IOCTL_DEMO_GET_STATS \
    _IOR(IOCTL_DEMO_MAGIC, 4, struct ioctl_demo_stats)

#endif
