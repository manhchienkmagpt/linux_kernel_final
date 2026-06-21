#include <linux/cdev.h>
#include <linux/dcache.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/path.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DEVICE_NAME "access_monitor"
#define PATH_MAX_LEN 256
#define EVENT_MAX_LEN 512

static dev_t dev_number;
static struct cdev monitor_cdev;
static struct class *monitor_class;

static char protected_path[PATH_MAX_LEN] = "/tmp/protected";
module_param_string(protected_path, protected_path, sizeof(protected_path), 0644);
MODULE_PARM_DESC(protected_path, "Directory path monitored for WRITE and DELETE events");

static DEFINE_SPINLOCK(path_lock);
static DEFINE_SPINLOCK(event_lock);
static char last_event[EVENT_MAX_LEN] = "No events yet";
static atomic64_t total_events = ATOMIC64_INIT(0);

static struct kprobe write_kp;
static struct kprobe unlink_kp;

static bool path_is_protected(const char *path)
{
    char local_path[PATH_MAX_LEN];
    unsigned long flags;
    size_t len;

    if (!path)
        return false;

    spin_lock_irqsave(&path_lock, flags);
    strscpy(local_path, protected_path, sizeof(local_path));
    spin_unlock_irqrestore(&path_lock, flags);

    len = strlen(local_path);
    if (len == 0)
        return false;

    return strncmp(path, local_path, len) == 0 &&
           (path[len] == '\0' || path[len] == '/');
}

static void remember_event(const char *action, const char *path)
{
    unsigned long flags;
    char event[EVENT_MAX_LEN];

    snprintf(event, sizeof(event),
             "[access_monitor] PID=%d COMM=%s ACTION=%s PATH=%s",
             current->pid, current->comm, action, path ? path : "N/A");

    spin_lock_irqsave(&event_lock, flags);
    strscpy(last_event, event, sizeof(last_event));
    spin_unlock_irqrestore(&event_lock, flags);

    atomic64_inc(&total_events);
    pr_info("%s\n", event);
}

static char *path_from_file(struct file *file)
{
    char *page;
    char *resolved;
    char *result = NULL;

    if (!file)
        return NULL;

    page = (char *)__get_free_page(GFP_ATOMIC);
    if (!page)
        return NULL;

    resolved = d_path(&file->f_path, page, PAGE_SIZE);
    if (!IS_ERR(resolved))
        result = kstrdup(resolved, GFP_ATOMIC);

    free_page((unsigned long)page);
    return result;
}

static char *path_from_dentry(struct dentry *dentry)
{
    char *page;
    char *resolved;
    char *result = NULL;

    if (!dentry)
        return NULL;

    page = (char *)__get_free_page(GFP_ATOMIC);
    if (!page)
        return NULL;

    resolved = dentry_path_raw(dentry, page, PAGE_SIZE);
    if (!IS_ERR(resolved))
        result = kstrdup(resolved, GFP_ATOMIC);

    free_page((unsigned long)page);
    return result;
}

static int write_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct file *file = NULL;
    char *path;

    (void)p;
#if defined(CONFIG_X86_64)
    file = (struct file *)regs->di;
#else
    return 0;
#endif

    path = path_from_file(file);
    if (path && path_is_protected(path))
        remember_event("WRITE", path);
    kfree(path);
    return 0;
}

static int unlink_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    struct dentry *dentry = NULL;
    char *path;

    (void)p;
#if defined(CONFIG_X86_64)
    dentry = (struct dentry *)regs->dx;
#else
    return 0;
#endif

    path = path_from_dentry(dentry);
    if (path && path_is_protected(path))
        remember_event("DELETE", path);
    kfree(path);
    return 0;
}

static int monitor_open(struct inode *inode, struct file *file)
{
    pr_info("access_monitor: device opened\n");
    return 0;
}

static ssize_t monitor_read(struct file *file, char __user *user_buffer,
                            size_t count, loff_t *offset)
{
    char buffer[PATH_MAX_LEN + 32];
    unsigned long flags;
    int len;

    spin_lock_irqsave(&path_lock, flags);
    len = scnprintf(buffer, sizeof(buffer), "%s\n", protected_path);
    spin_unlock_irqrestore(&path_lock, flags);

    return simple_read_from_buffer(user_buffer, count, offset, buffer, len);
}

static ssize_t monitor_write(struct file *file, const char __user *user_buffer,
                             size_t count, loff_t *offset)
{
    char buffer[PATH_MAX_LEN];
    char *trimmed;
    unsigned long flags;
    size_t bytes = min(count, (size_t)(PATH_MAX_LEN - 1));

    if (copy_from_user(buffer, user_buffer, bytes))
        return -EFAULT;

    buffer[bytes] = '\0';
    trimmed = strim(buffer);

    if (trimmed[0] != '/') {
        pr_warn("access_monitor: rejected non-absolute path: %s\n", trimmed);
        return -EINVAL;
    }

    spin_lock_irqsave(&path_lock, flags);
    strscpy(protected_path, trimmed, sizeof(protected_path));
    spin_unlock_irqrestore(&path_lock, flags);

    pr_info("access_monitor: protected_path changed to %s\n", protected_path);
    return count;
}

static int monitor_release(struct inode *inode, struct file *file)
{
    pr_info("access_monitor: device released\n");
    return 0;
}

static const struct file_operations monitor_fops = {
    .owner = THIS_MODULE,
    .open = monitor_open,
    .read = monitor_read,
    .write = monitor_write,
    .release = monitor_release,
};

static int setup_device(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&monitor_cdev, &monitor_fops);
    ret = cdev_add(&monitor_cdev, dev_number, 1);
    if (ret < 0)
        goto unregister_region;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
    monitor_class = class_create(THIS_MODULE, DEVICE_NAME);
#else
    monitor_class = class_create(DEVICE_NAME);
#endif
    if (IS_ERR(monitor_class)) {
        ret = PTR_ERR(monitor_class);
        goto del_cdev;
    }

    if (IS_ERR(device_create(monitor_class, NULL, dev_number, NULL, DEVICE_NAME))) {
        ret = -ENOMEM;
        goto destroy_class;
    }

    return 0;

destroy_class:
    class_destroy(monitor_class);
del_cdev:
    cdev_del(&monitor_cdev);
unregister_region:
    unregister_chrdev_region(dev_number, 1);
    return ret;
}

static void cleanup_device(void)
{
    device_destroy(monitor_class, dev_number);
    class_destroy(monitor_class);
    cdev_del(&monitor_cdev);
    unregister_chrdev_region(dev_number, 1);
}

static int register_monitor_kprobes(void)
{
    int ret;

    write_kp.symbol_name = "vfs_write";
    write_kp.pre_handler = write_pre_handler;
    ret = register_kprobe(&write_kp);
    if (ret < 0) {
        pr_err("access_monitor: register_kprobe vfs_write failed: %d\n", ret);
        return ret;
    }

    unlink_kp.symbol_name = "vfs_unlink";
    unlink_kp.pre_handler = unlink_pre_handler;
    ret = register_kprobe(&unlink_kp);
    if (ret < 0) {
        pr_err("access_monitor: register_kprobe vfs_unlink failed: %d\n", ret);
        unregister_kprobe(&write_kp);
        return ret;
    }

    return 0;
}

static void unregister_monitor_kprobes(void)
{
    unregister_kprobe(&unlink_kp);
    unregister_kprobe(&write_kp);
}

static int __init access_monitor_init(void)
{
    int ret;

    ret = setup_device();
    if (ret < 0) {
        pr_err("access_monitor: device setup failed: %d\n", ret);
        return ret;
    }

    ret = register_monitor_kprobes();
    if (ret < 0) {
        cleanup_device();
        return ret;
    }

    pr_info("access_monitor loaded protected_path=%s\n", protected_path);
    return 0;
}

static void __exit access_monitor_exit(void)
{
    unregister_monitor_kprobes();
    cleanup_device();
    pr_info("access_monitor unloaded\n");
}

module_init(access_monitor_init);
module_exit(access_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Programming Student");
MODULE_DESCRIPTION("Mini file access security monitor using kprobes");
MODULE_VERSION("1.0");
