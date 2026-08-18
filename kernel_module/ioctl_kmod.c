#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "ioctl_shared.h"

/* 设备的全局状态；除 open_count 外，其余可变字段均由 lock 保护。 */
struct ioctl_demo_state {
    struct mutex lock;
    char data[IOCTL_DEMO_MAX_DATA];
    __u32 length;
    __u64 set_count;
    __u64 get_count;
    __u64 clear_count;
    atomic_t open_count;
};

/* 静态对象在模块加载时已清零，只需显式初始化锁和原子计数器。 */
static struct ioctl_demo_state demo_state = {
    .lock = __MUTEX_INITIALIZER(demo_state.lock),
    .open_count = ATOMIC_INIT(0),
};

/* 每次成功打开设备时记录一个活跃文件描述符。 */
static int ioctl_demo_open(struct inode *inode, struct file *file)
{
    atomic_inc(&demo_state.open_count);
    return 0;
}

/* 与 open 配对；原子操作允许它不依赖主状态互斥锁。 */
static int ioctl_demo_release(struct inode *inode, struct file *file)
{
    atomic_dec(&demo_state.open_count);
    return 0;
}

static long ioctl_demo_set_message(unsigned long arg)
{
    struct ioctl_demo_message message;

    /* ioctl 参数是用户地址，必须通过 uaccess 接口安全地读取。 */
    if (copy_from_user(&message, (void __user *)arg, sizeof(message)))
        return -EFAULT;

    /* 在复制 message.data 前先验证用户提供的有效长度。 */
    if (message.length > IOCTL_DEMO_MAX_DATA)
        return -EINVAL;

    /* 清零尾部，确保短消息覆盖长消息后不会残留旧内容。 */
    mutex_lock(&demo_state.lock);
    memset(demo_state.data, 0, sizeof(demo_state.data));
    memcpy(demo_state.data, message.data, message.length);
    demo_state.length = message.length;
    demo_state.set_count++;
    mutex_unlock(&demo_state.lock);

    return 0;
}

static long ioctl_demo_get_message(unsigned long arg)
{
    struct ioctl_demo_message message = { 0 };

    /* 在锁内取得一致的长度、内容，并同步更新读取次数。 */
    mutex_lock(&demo_state.lock);
    message.length = demo_state.length;
    memcpy(message.data, demo_state.data, demo_state.length);
    demo_state.get_count++;
    mutex_unlock(&demo_state.lock);

    /* 将完整结构复制回用户空间，未使用的 data 区域保持为零。 */
    if (copy_to_user((void __user *)arg, &message, sizeof(message)))
        return -EFAULT;

    return 0;
}

static long ioctl_demo_clear(void)
{
    /* 清空消息内容和有效长度，但保留历史统计计数。 */
    mutex_lock(&demo_state.lock);
    memset(demo_state.data, 0, sizeof(demo_state.data));
    demo_state.length = 0;
    demo_state.clear_count++;
    mutex_unlock(&demo_state.lock);

    return 0;
}

static long ioctl_demo_get_stats(unsigned long arg)
{
    struct ioctl_demo_stats stats;

    /* 在同一个临界区内生成统计快照，避免字段来自不同时间点。 */
    mutex_lock(&demo_state.lock);
    stats.set_count = demo_state.set_count;
    stats.get_count = demo_state.get_count;
    stats.clear_count = demo_state.clear_count;
    stats.current_length = demo_state.length;
    /* open_count 是原子变量，可在持有主状态锁时直接读取当前值。 */
    stats.open_count = atomic_read(&demo_state.open_count);
    mutex_unlock(&demo_state.lock);

    /* 将锁内生成的完整统计快照复制回用户空间。 */
    if (copy_to_user((void __user *)arg, &stats, sizeof(stats)))
        return -EFAULT;

    return 0;
}

static long ioctl_demo_ioctl(struct file *file, unsigned int cmd,
                             unsigned long arg)
{
    /* 先校验命令族，再把具体命令分发给对应处理函数。 */
    if (_IOC_TYPE(cmd) != IOCTL_DEMO_MAGIC)
        return -ENOTTY;

    switch (cmd) {
    case IOCTL_DEMO_SET_MESSAGE:
        return ioctl_demo_set_message(arg);
    case IOCTL_DEMO_GET_MESSAGE:
        return ioctl_demo_get_message(arg);
    case IOCTL_DEMO_CLEAR:
        return ioctl_demo_clear();
    case IOCTL_DEMO_GET_STATS:
        return ioctl_demo_get_stats(arg);
    default:
        return -ENOTTY;
    }
}

/* 将字符设备操作挂接到本模块的实现。 */
static const struct file_operations ioctl_demo_fops = {
    .owner = THIS_MODULE,
    .open = ioctl_demo_open,
    .release = ioctl_demo_release,
    .unlocked_ioctl = ioctl_demo_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = ioctl_demo_ioctl,
#endif
};

/* misc 设备可自动分配次设备号，免去手工注册字符设备号。 */
static struct miscdevice ioctl_demo_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = IOCTL_DEMO_DEVICE_NAME,
    .fops = &ioctl_demo_fops,
    .mode = 0666,
};

static int __init ioctl_demo_init(void)
{
    int ret;

    /* 注册 misc 设备；启用 devtmpfs 时会出现对应的 /dev 设备节点。 */
    ret = misc_register(&ioctl_demo_device);
    if (ret)
        return ret;

    pr_info("ioctl_demo: registered %s\n", IOCTL_DEMO_DEVICE_PATH);
    return 0;
}

static void __exit ioctl_demo_exit(void)
{
    /* 模块卸载时撤销注册，释放 misc 设备占用的内核资源。 */
    misc_deregister(&ioctl_demo_device);
    pr_info("ioctl_demo: unregistered\n");
}

module_init(ioctl_demo_init);
module_exit(ioctl_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ioctl_examples");
MODULE_DESCRIPTION("A misc device demonstrating structured ioctl commands");
