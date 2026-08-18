#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ioctl_shared.h"

/* 输出客户端支持的命令格式。 */
static void print_usage(const char *program)
{
    fprintf(stderr,
            "用法:\n"
            "  %s set <消息>\n"
            "  %s get\n"
            "  %s clear\n"
            "  %s stats\n"
            "  %s demo\n",
            program, program, program, program, program);
}

static int set_message(int fd, const char *text)
{
    struct ioctl_demo_message message = { 0 };
    size_t length = strlen(text);

    /* 驱动端缓冲区固定为 128 字节，调用 ioctl 前先在用户态拦截超长输入。 */
    if (length > IOCTL_DEMO_MAX_DATA) {
        fprintf(stderr, "消息过长：最多 %d 字节，实际 %zu 字节\n",
                IOCTL_DEMO_MAX_DATA, length);
        return -1;
    }

    message.length = (uint32_t)length;
    memcpy(message.data, text, length);

    /* _IOW 命令把 message 结构复制到内核。 */
    if (ioctl(fd, IOCTL_DEMO_SET_MESSAGE, &message) == -1) {
        fprintf(stderr, "SET_MESSAGE 失败: %s\n", strerror(errno));
        return -1;
    }

    printf("已写入 %zu 字节\n", length);
    return 0;
}

static int get_message(int fd)
{
    struct ioctl_demo_message message = { 0 };

    /* _IOR 命令由驱动填充 message 结构。 */
    if (ioctl(fd, IOCTL_DEMO_GET_MESSAGE, &message) == -1) {
        fprintf(stderr, "GET_MESSAGE 失败: %s\n", strerror(errno));
        return -1;
    }

    /* 不盲目信任内核返回的长度，防止后续读取越过本地数组。 */
    if (message.length > IOCTL_DEMO_MAX_DATA) {
        fprintf(stderr, "内核返回了无效长度: %u\n", message.length);
        return -1;
    }

    /* 消息不保证以 '\0' 结尾，必须按明确长度输出。 */
    printf("当前消息（%u 字节）: ", message.length);
    if (message.length != 0)
        fwrite(message.data, 1, message.length, stdout);
    putchar('\n');
    return 0;
}

static int clear_message(int fd)
{
    /* CLEAR 不携带第三个参数，只触发驱动清空当前消息。 */
    if (ioctl(fd, IOCTL_DEMO_CLEAR) == -1) {
        fprintf(stderr, "CLEAR 失败: %s\n", strerror(errno));
        return -1;
    }

    puts("消息已清空");
    return 0;
}

static int get_stats(int fd)
{
    struct ioctl_demo_stats stats = { 0 };

    /* 获取驱动当前统计信息的快照。 */
    if (ioctl(fd, IOCTL_DEMO_GET_STATS, &stats) == -1) {
        fprintf(stderr, "GET_STATS 失败: %s\n", strerror(errno));
        return -1;
    }

    printf("统计信息:\n"
           "  set 次数:   %llu\n"
           "  get 次数:   %llu\n"
           "  clear 次数: %llu\n"
           "  当前长度:   %u\n"
           "  打开数量:   %u\n",
           (unsigned long long)stats.set_count,
           (unsigned long long)stats.get_count,
           (unsigned long long)stats.clear_count,
           stats.current_length, stats.open_count);
    return 0;
}

static int run_demo(int fd)
{
    /* 按“写入、读取、查看统计、清空、再次读取”的顺序演示接口。 */
    if (set_message(fd, "hello from user space") == -1)
        return -1;
    if (get_message(fd) == -1)
        return -1;
    if (get_stats(fd) == -1)
        return -1;
    if (clear_message(fd) == -1)
        return -1;
    return get_message(fd);
}

int main(int argc, char **argv)
{
    const char *command;
    int command_is_valid;
    int fd;
    int result = -1;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    /* set 需要额外的消息参数，其余合法命令都不接受附加参数。 */
    command = argv[1];
    command_is_valid = strcmp(command, "get") == 0 ||
                       strcmp(command, "clear") == 0 ||
                       strcmp(command, "stats") == 0 ||
                       strcmp(command, "demo") == 0;

    if ((strcmp(command, "set") == 0 && argc != 3) ||
        (command_is_valid && argc != 2) ||
        (strcmp(command, "set") != 0 && !command_is_valid)) {
        print_usage(argv[0]);
        return 1;
    }

    /* 所有命令共用同一个设备描述符，并在退出前统一关闭。 */
    fd = open(IOCTL_DEMO_DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "无法打开 %s: %s\n",
                IOCTL_DEMO_DEVICE_PATH, strerror(errno));
        fprintf(stderr, "请先编译并加载 ioctl_kmod.ko。\n");
        return 1;
    }

    /* 根据命令行子命令调用对应的用户态封装函数。 */
    if (strcmp(command, "set") == 0)
        result = set_message(fd, argv[2]);
    else if (strcmp(command, "get") == 0)
        result = get_message(fd);
    else if (strcmp(command, "clear") == 0)
        result = clear_message(fd);
    else if (strcmp(command, "stats") == 0)
        result = get_stats(fd);
    else if (strcmp(command, "demo") == 0)
        result = run_demo(fd);
    else
        print_usage(argv[0]);

    if (close(fd) == -1) {
        fprintf(stderr, "关闭 %s 失败: %s\n",
                IOCTL_DEMO_DEVICE_PATH, strerror(errno));
        return 1;
    }

    return result == 0 ? 0 : 1;
}
