#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DEVICE_NAME "simple_kmod"
#define BUFFER_SIZE 1024

static dev_t dev_number;
static struct cdev simple_cdev;
static struct class *simple_class;
static char device_buffer[BUFFER_SIZE];
static size_t buffer_len;

static int kmod_open(struct inode *inode, struct file *file) {
    pr_info("simple_kmod: device opened\n");
    return 0;
}

static ssize_t kmod_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset) {
    size_t bytes_to_read;

    if (*offset >= buffer_len)
        return 0;

    bytes_to_read = min(count, buffer_len - (size_t)*offset);
    if (copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read))
        return -EFAULT;

    *offset += bytes_to_read;
    pr_info("simple_kmod: read %zu bytes\n", bytes_to_read);
    return bytes_to_read;
}

static ssize_t kmod_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset) {
    size_t bytes_to_write = min(count, (size_t)(BUFFER_SIZE - 1));

    memset(device_buffer, 0, sizeof(device_buffer));
    if (copy_from_user(device_buffer, user_buffer, bytes_to_write))
        return -EFAULT;

    device_buffer[bytes_to_write] = '\0';
    buffer_len = bytes_to_write;
    *offset = buffer_len;
    pr_info("simple_kmod: wrote %zu bytes: %s\n", bytes_to_write, device_buffer);
    return bytes_to_write;
}

static int kmod_release(struct inode *inode, struct file *file) {
    pr_info("simple_kmod: device released\n");
    return 0;
}

static const struct file_operations simple_fops = {
    .owner = THIS_MODULE,
    .open = kmod_open,
    .read = kmod_read,
    .write = kmod_write,
    .release = kmod_release,
};

static int __init simple_init(void) {
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("simple_kmod: alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&simple_cdev, &simple_fops);
    ret = cdev_add(&simple_cdev, dev_number, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
    simple_class = class_create(THIS_MODULE, DEVICE_NAME);
#else
    simple_class = class_create(DEVICE_NAME);
#endif
    if (IS_ERR(simple_class)) {
        cdev_del(&simple_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(simple_class);
    }

    if (IS_ERR(device_create(simple_class, NULL, dev_number, NULL, DEVICE_NAME))) {
        class_destroy(simple_class);
        cdev_del(&simple_cdev);
        unregister_chrdev_region(dev_number, 1);
        return -ENOMEM;
    }

    snprintf(device_buffer, sizeof(device_buffer), "Hello from simple_kmod\n");
    buffer_len = strlen(device_buffer);
    pr_info("simple_kmod: loaded major=%d minor=%d\n", MAJOR(dev_number), MINOR(dev_number));
    return 0;
}

static void __exit simple_exit(void) {
    device_destroy(simple_class, dev_number);
    class_destroy(simple_class);
    cdev_del(&simple_cdev);
    unregister_chrdev_region(dev_number, 1);
    pr_info("simple_kmod: unloaded\n");
}

module_init(simple_init);
module_exit(simple_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Programming Student");
MODULE_DESCRIPTION("Simple character device kernel module integrated with GTK app");
MODULE_VERSION("1.0");
