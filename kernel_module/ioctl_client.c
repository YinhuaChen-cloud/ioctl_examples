#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "ioctl_shared.h"

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

    if (length > IOCTL_DEMO_MAX_DATA) {
        fprintf(stderr, "消息过长：最多 %d 字节，实际 %zu 字节\n",
                IOCTL_DEMO_MAX_DATA, length);
        return -1;
    }

    message.length = (uint32_t)length;
    memcpy(message.data, text, length);

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

    if (ioctl(fd, IOCTL_DEMO_GET_MESSAGE, &message) == -1) {
        fprintf(stderr, "GET_MESSAGE 失败: %s\n", strerror(errno));
        return -1;
    }

    if (message.length > IOCTL_DEMO_MAX_DATA) {
        fprintf(stderr, "内核返回了无效长度: %u\n", message.length);
        return -1;
    }

    printf("当前消息（%u 字节）: ", message.length);
    if (message.length != 0)
        fwrite(message.data, 1, message.length, stdout);
    putchar('\n');
    return 0;
}

static int clear_message(int fd)
{
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

    fd = open(IOCTL_DEMO_DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "无法打开 %s: %s\n",
                IOCTL_DEMO_DEVICE_PATH, strerror(errno));
        fprintf(stderr, "请先编译并加载 ioctl_kmod.ko。\n");
        return 1;
    }

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
