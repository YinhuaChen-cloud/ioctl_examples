#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(void)
{
    struct winsize size;

    /*
     * /dev/tty 表示当前进程的控制终端。
     * 使用它而不是直接使用标准输入，可以避免标准输入被重定向时查询失败。
     */
    int tty_fd = open("/dev/tty", O_RDONLY);
    if (tty_fd == -1) {
        fprintf(stderr, "无法打开 /dev/tty: %s\n", strerror(errno));
        return 1;
    }

    /*
     * ioctl 用于对文件描述符执行设备相关的控制操作：
     *   tty_fd     要操作的终端文件描述符
     *   TIOCGWINSZ 获取终端窗口大小的请求码
     *   &size      内核写入查询结果的地址
     */
    // #define TIOCGWINSZ 0x5413
    if (ioctl(tty_fd, TIOCGWINSZ, &size) == -1) {
        fprintf(stderr, "ioctl(TIOCGWINSZ) 失败: %s\n", strerror(errno));
        close(tty_fd);
        return 1;
    }

    printf("终端窗口大小：\n");
    printf("  行数: %u\n", (unsigned int)size.ws_row);
    printf("  列数: %u\n", (unsigned int)size.ws_col);
    printf("  像素宽度: %u\n", (unsigned int)size.ws_xpixel);
    printf("  像素高度: %u\n", (unsigned int)size.ws_ypixel);

    if (close(tty_fd) == -1) {
        fprintf(stderr, "关闭 /dev/tty 失败: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}
