#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "ioctl_shared.h"

struct ioctl_demo_state {
    struct mutex lock;
    char data[IOCTL_DEMO_MAX_DATA];
    __u32 length;
    __u64 set_count;
    __u64 get_count;
    __u64 clear_count;
    atomic_t open_count;
};

static struct ioctl_demo_state demo_state = {
    .lock = __MUTEX_INITIALIZER(demo_state.lock),
    .open_count = ATOMIC_INIT(0),
};

static int ioctl_demo_open(struct inode *inode, struct file *file)
{
    atomic_inc(&demo_state.open_count);
    return 0;
}

static int ioctl_demo_release(struct inode *inode, struct file *file)
{
    atomic_dec(&demo_state.open_count);
    return 0;
}

static long ioctl_demo_set_message(unsigned long arg)
{
    struct ioctl_demo_message message;

    if (copy_from_user(&message, (void __user *)arg, sizeof(message)))
        return -EFAULT;

    if (message.length > IOCTL_DEMO_MAX_DATA)
        return -EINVAL;

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

    mutex_lock(&demo_state.lock);
    message.length = demo_state.length;
    memcpy(message.data, demo_state.data, demo_state.length);
    demo_state.get_count++;
    mutex_unlock(&demo_state.lock);

    if (copy_to_user((void __user *)arg, &message, sizeof(message)))
        return -EFAULT;

    return 0;
}

static long ioctl_demo_clear(void)
{
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

    mutex_lock(&demo_state.lock);
    stats.set_count = demo_state.set_count;
    stats.get_count = demo_state.get_count;
    stats.clear_count = demo_state.clear_count;
    stats.current_length = demo_state.length;
    stats.open_count = atomic_read(&demo_state.open_count);
    mutex_unlock(&demo_state.lock);

    if (copy_to_user((void __user *)arg, &stats, sizeof(stats)))
        return -EFAULT;

    return 0;
}

static long ioctl_demo_ioctl(struct file *file, unsigned int cmd,
                             unsigned long arg)
{
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

static const struct file_operations ioctl_demo_fops = {
    .owner = THIS_MODULE,
    .open = ioctl_demo_open,
    .release = ioctl_demo_release,
    .unlocked_ioctl = ioctl_demo_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = ioctl_demo_ioctl,
#endif
};

static struct miscdevice ioctl_demo_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = IOCTL_DEMO_DEVICE_NAME,
    .fops = &ioctl_demo_fops,
    .mode = 0666,
};

static int __init ioctl_demo_init(void)
{
    int ret;

    ret = misc_register(&ioctl_demo_device);
    if (ret)
        return ret;

    pr_info("ioctl_demo: registered %s\n", IOCTL_DEMO_DEVICE_PATH);
    return 0;
}

static void __exit ioctl_demo_exit(void)
{
    misc_deregister(&ioctl_demo_device);
    pr_info("ioctl_demo: unregistered\n");
}

module_init(ioctl_demo_init);
module_exit(ioctl_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ioctl_examples");
MODULE_DESCRIPTION("A misc device demonstrating structured ioctl commands");
