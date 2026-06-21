#include <linux/fs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

#define MODULE_TAG "mouse_monitor"
#define PROC_NAME "mouse_monitor"
#define STATUS_BUF_LEN 256

struct mouse_status {
    bool connected;
    int devices;
    int left;
    int right;
    int middle;
    int dx;
    int dy;
    int wheel;
};

struct monitor_handle {
    struct input_handle handle;
    bool abs_x_initialized;
    bool abs_y_initialized;
    bool mt_x_initialized;
    bool mt_y_initialized;
    int last_abs_x;
    int last_abs_y;
    int last_mt_x;
    int last_mt_y;
};

static struct proc_dir_entry *proc_entry;
static struct mouse_status current_status;
static spinlock_t status_lock;
static int connected_devices;
static int frame_dx;
static int frame_dy;
static int frame_wheel;

static bool has_bit(const unsigned long *bitmap, unsigned int bit)
{
    return test_bit(bit, bitmap);
}

static void set_connected_locked(void)
{
    current_status.connected = connected_devices > 0;
    current_status.devices = connected_devices;
    if (!current_status.connected) {
        current_status.left = 0;
        current_status.right = 0;
        current_status.middle = 0;
        current_status.dx = 0;
        current_status.dy = 0;
        current_status.wheel = 0;
        frame_dx = 0;
        frame_dy = 0;
        frame_wheel = 0;
    }
}

static bool is_pointer_device(struct input_dev *dev)
{
    bool relative_mouse;
    bool absolute_pointer;
    bool multitouch_pointer;

    if (!dev)
        return false;

    relative_mouse = has_bit(dev->evbit, EV_REL) &&
                     has_bit(dev->relbit, REL_X) &&
                     has_bit(dev->relbit, REL_Y);

    absolute_pointer = has_bit(dev->evbit, EV_ABS) &&
                       has_bit(dev->absbit, ABS_X) &&
                       has_bit(dev->absbit, ABS_Y);

    multitouch_pointer = has_bit(dev->evbit, EV_ABS) &&
                         has_bit(dev->absbit, ABS_MT_POSITION_X) &&
                         has_bit(dev->absbit, ABS_MT_POSITION_Y);

    return relative_mouse || absolute_pointer || multitouch_pointer;
}

static bool is_mouse_event(unsigned int type, unsigned int code)
{
    if (type == EV_KEY)
        return code == BTN_LEFT || code == BTN_RIGHT || code == BTN_MIDDLE ||
               code == BTN_TOUCH || code == BTN_TOOL_FINGER;

    if (type == EV_REL)
        return code == REL_X || code == REL_Y || code == REL_WHEEL;

    if (type == EV_ABS)
        return code == ABS_X || code == ABS_Y ||
               code == ABS_MT_POSITION_X || code == ABS_MT_POSITION_Y;

    return type == EV_SYN && code == SYN_REPORT;
}

static void log_snapshot(const struct mouse_status *snapshot)
{
    if (!snapshot)
        return;

    pr_info("[%s] left=%d right=%d middle=%d dx=%d dy=%d wheel=%d\n",
            MODULE_TAG,
            snapshot->left,
            snapshot->right,
            snapshot->middle,
            snapshot->dx,
            snapshot->dy,
            snapshot->wheel);
}

static void mouse_event(struct input_handle *handle,
                        unsigned int type,
                        unsigned int code,
                        int value)
{
    struct monitor_handle *monitor;
    struct mouse_status snapshot;
    unsigned long flags;
    bool button_changed = false;
    bool should_log = false;

    if (!handle || !is_mouse_event(type, code))
        return;

    monitor = container_of(handle, struct monitor_handle, handle);

    spin_lock_irqsave(&status_lock, flags);
    current_status.connected = true;

    if (type == EV_KEY) {
        int pressed = value ? 1 : 0;

        if ((code == BTN_LEFT || code == BTN_TOUCH || code == BTN_TOOL_FINGER) &&
            current_status.left != pressed) {
            current_status.left = pressed;
            button_changed = true;
        } else if (code == BTN_RIGHT && current_status.right != pressed) {
            current_status.right = pressed;
            button_changed = true;
        } else if (code == BTN_MIDDLE && current_status.middle != pressed) {
            current_status.middle = pressed;
            button_changed = true;
        }
    } else if (type == EV_REL) {
        if (code == REL_X)
            frame_dx += value;
        else if (code == REL_Y)
            frame_dy += value;
        else if (code == REL_WHEEL)
            frame_wheel += value;
    } else if (type == EV_ABS) {
        if (code == ABS_X) {
            if (monitor->abs_x_initialized)
                frame_dx += value - monitor->last_abs_x;
            monitor->last_abs_x = value;
            monitor->abs_x_initialized = true;
        } else if (code == ABS_Y) {
            if (monitor->abs_y_initialized)
                frame_dy += value - monitor->last_abs_y;
            monitor->last_abs_y = value;
            monitor->abs_y_initialized = true;
        } else if (code == ABS_MT_POSITION_X) {
            if (monitor->mt_x_initialized)
                frame_dx += value - monitor->last_mt_x;
            monitor->last_mt_x = value;
            monitor->mt_x_initialized = true;
        } else if (code == ABS_MT_POSITION_Y) {
            if (monitor->mt_y_initialized)
                frame_dy += value - monitor->last_mt_y;
            monitor->last_mt_y = value;
            monitor->mt_y_initialized = true;
        }
    } else if (type == EV_SYN && code == SYN_REPORT) {
        current_status.dx = frame_dx;
        current_status.dy = frame_dy;
        current_status.wheel = frame_wheel;
        should_log = frame_dx || frame_dy || frame_wheel;
        frame_dx = 0;
        frame_dy = 0;
        frame_wheel = 0;
    }

    if (button_changed) {
        current_status.dx = 0;
        current_status.dy = 0;
        current_status.wheel = 0;
        should_log = true;
    }

    if (should_log)
        snapshot = current_status;

    spin_unlock_irqrestore(&status_lock, flags);

    if (should_log)
        log_snapshot(&snapshot);
}

static int mouse_connect(struct input_handler *handler,
                         struct input_dev *dev,
                         const struct input_device_id *id)
{
    struct monitor_handle *monitor;
    struct input_handle *handle;
    unsigned long flags;
    int ret;

    (void)id;

    if (!is_pointer_device(dev))
        return -ENODEV;

    monitor = kzalloc(sizeof(*monitor), GFP_KERNEL);
    if (!monitor)
        return -ENOMEM;

    handle = &monitor->handle;
    handle->dev = dev;
    handle->handler = handler;
    handle->name = MODULE_TAG;

    ret = input_register_handle(handle);
    if (ret)
        goto fail_free;

    ret = input_open_device(handle);
    if (ret)
        goto fail_unregister;

    spin_lock_irqsave(&status_lock, flags);
    connected_devices++;
    set_connected_locked();
    spin_unlock_irqrestore(&status_lock, flags);

    pr_info("%s: pointer connected name=\"%s\" phys=\"%s\"\n",
            MODULE_TAG,
            dev->name ? dev->name : "unknown",
            dev->phys ? dev->phys : "unknown");
    return 0;

fail_unregister:
    input_unregister_handle(handle);
fail_free:
    kfree(monitor);
    return ret;
}

static void mouse_disconnect(struct input_handle *handle)
{
    struct monitor_handle *monitor;
    struct input_dev *dev = handle ? handle->dev : NULL;
    unsigned long flags;

    if (!handle)
        return;

    monitor = container_of(handle, struct monitor_handle, handle);

    input_close_device(handle);
    input_unregister_handle(handle);

    spin_lock_irqsave(&status_lock, flags);
    if (connected_devices > 0)
        connected_devices--;
    set_connected_locked();
    spin_unlock_irqrestore(&status_lock, flags);

    pr_info("%s: pointer disconnected name=\"%s\"\n",
            MODULE_TAG,
            dev && dev->name ? dev->name : "unknown");

    kfree(monitor);
}

static ssize_t proc_status_read(struct file *file, char __user *user_buffer,
                                size_t count, loff_t *offset)
{
    struct mouse_status snapshot;
    unsigned long flags;
    char buffer[STATUS_BUF_LEN];
    int len;

    (void)file;

    spin_lock_irqsave(&status_lock, flags);
    snapshot = current_status;
    spin_unlock_irqrestore(&status_lock, flags);

    len = scnprintf(buffer, sizeof(buffer),
                    "connected=%d\n"
                    "devices=%d\n"
                    "left=%d\n"
                    "right=%d\n"
                    "middle=%d\n"
                    "dx=%d\n"
                    "dy=%d\n"
                    "wheel=%d\n",
                    snapshot.connected ? 1 : 0,
                    snapshot.devices,
                    snapshot.left,
                    snapshot.right,
                    snapshot.middle,
                    snapshot.dx,
                    snapshot.dy,
                    snapshot.wheel);

    return simple_read_from_buffer(user_buffer, count, offset, buffer, len);
}

static const struct proc_ops proc_status_ops = {
    .proc_read = proc_status_read,
};

static const struct input_device_id mouse_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_RELBIT,
        .evbit = { BIT_MASK(EV_REL) },
        .relbit = {
            [BIT_WORD(REL_X)] = BIT_MASK(REL_X) | BIT_MASK(REL_Y),
        },
    },
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_ABSBIT,
        .evbit = { BIT_MASK(EV_ABS) },
        .absbit = {
            [BIT_WORD(ABS_X)] = BIT_MASK(ABS_X) | BIT_MASK(ABS_Y),
        },
    },
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_ABSBIT,
        .evbit = { BIT_MASK(EV_ABS) },
        .absbit = {
            [BIT_WORD(ABS_MT_POSITION_X)] =
                BIT_MASK(ABS_MT_POSITION_X) | BIT_MASK(ABS_MT_POSITION_Y),
        },
    },
    {}
};
MODULE_DEVICE_TABLE(input, mouse_ids);

static struct input_handler mouse_handler = {
    .event = mouse_event,
    .connect = mouse_connect,
    .disconnect = mouse_disconnect,
    .name = MODULE_TAG,
    .id_table = mouse_ids,
};

static int __init mouse_monitor_init(void)
{
    int ret;

    spin_lock_init(&status_lock);
    memset(&current_status, 0, sizeof(current_status));
    connected_devices = 0;
    frame_dx = 0;
    frame_dy = 0;
    frame_wheel = 0;

    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_status_ops);
    if (!proc_entry) {
        pr_err("%s: failed to create /proc/%s\n", MODULE_TAG, PROC_NAME);
        return -ENOMEM;
    }

    ret = input_register_handler(&mouse_handler);
    if (ret) {
        proc_remove(proc_entry);
        proc_entry = NULL;
        pr_err("%s: input_register_handler failed: %d\n", MODULE_TAG, ret);
        return ret;
    }

    pr_info("%s loaded\n", MODULE_TAG);
    return 0;
}

static void __exit mouse_monitor_exit(void)
{
    input_unregister_handler(&mouse_handler);
    if (proc_entry)
        proc_remove(proc_entry);
    pr_info("%s unloaded\n", MODULE_TAG);
}

module_init(mouse_monitor_init);
module_exit(mouse_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Programming Student");
MODULE_DESCRIPTION("Current Ubuntu mouse and touchpad input event monitor");
MODULE_VERSION("2.0");
