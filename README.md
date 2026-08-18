# ioctl examples

这个仓库包含两个由浅入深的 Linux `ioctl` 例子。

## 1. 查询终端窗口大小

`ioctl_demo.c` 是纯用户态程序，调用终端驱动已经实现的
`TIOCGWINSZ` 请求：

```sh
make
make run
```

## 2. 自定义 ioctl 内核模块

`kernel_module/` 展示了用户态与内核态共同定义一套 ioctl ABI：

- `ioctl_shared.h`：双方共享的数据结构和请求码。
- `ioctl_kmod.c`：注册 `/dev/ioctl_demo` 的内核模块。
- `ioctl_client.c`：发送 ioctl 请求的用户态控制程序。
- `Makefile`：通过内核 Kbuild 系统编译模块。

模块维护一段最多 128 字节的内核缓冲区，并支持：

| 命令 | 方向 | 作用 |
| --- | --- | --- |
| `IOCTL_DEMO_SET_MESSAGE` | 用户态 -> 内核态 | 写入消息 |
| `IOCTL_DEMO_GET_MESSAGE` | 内核态 -> 用户态 | 读取消息 |
| `IOCTL_DEMO_CLEAR` | 无数据 | 清空消息 |
| `IOCTL_DEMO_GET_STATS` | 内核态 -> 用户态 | 获取调用统计 |

先安装与当前内核匹配的头文件，然后编译：

```sh
make kernel-example
```

加载模块需要 root 权限：

```sh
make kernel-load
ls -l /dev/ioctl_demo
```

运行完整流程或逐条操作：

```sh
make kernel-demo

./kernel_module/ioctl_client set "一条来自用户态的消息"
./kernel_module/ioctl_client get
./kernel_module/ioctl_client stats
./kernel_module/ioctl_client clear
```

最后卸载模块：

```sh
make kernel-unload
```

可以用下面的命令查看模块日志：

```sh
sudo dmesg | tail
```

如果启用了 Secure Boot，系统可能拒绝加载未签名的 `.ko` 文件；这时需要为
模块签名，或在专门用于学习的虚拟机中关闭 Secure Boot 后再试。

### 一次 ioctl 调用经过哪里

1. 用户态调用 `ioctl(fd, request, &data)`。
2. 系统调用根据 `fd` 找到本模块的 `file_operations`。
3. 内核进入 `ioctl_demo_ioctl()`，按请求码分发操作。
4. `copy_from_user()` 或 `copy_to_user()` 跨越用户态/内核态边界复制数据。
5. 内核返回 `0` 或负 errno；C 库将负 errno 转换为 `-1` 并设置 `errno`。

共享头文件中的 `_IO`、`_IOR`、`_IOW` 会把设备类型、命令编号、数据方向和
数据大小编码进请求码。固定宽度的 `__u32`/`__u64` 则避免用户态和内核态因
整数大小不同而产生 ABI 歧义。
