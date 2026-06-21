#include <linux/init.h>
#include <linux/fs.h>
#include <linux/hid.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/usb.h>

#define MODULE_TAG "usb_mouse_monitor"
#define PROC_NAME "usb_mouse_monitor"
#define STATUS_BUF_LEN 256

struct mouse_status {
    bool connected;
    int left;
    int right;
    int middle;
    int dx;
    int dy;
    int wheel;
};

struct mouse_device {
    struct usb_device *udev;
    struct usb_interface *interface;
    struct urb *irq_urb;
    unsigned char *data;
    dma_addr_t data_dma;
    int data_size;
    unsigned char endpoint_addr;
};

static struct proc_dir_entry *proc_entry;
static struct mouse_device *monitored_mouse;
static struct mouse_status current_status;
static spinlock_t status_lock;

static void update_connected(bool connected)
{
    unsigned long flags;

    spin_lock_irqsave(&status_lock, flags);
    current_status.connected = connected;
    if (!connected) {
        current_status.left = 0;
        current_status.right = 0;
        current_status.middle = 0;
        current_status.dx = 0;
        current_status.dy = 0;
        current_status.wheel = 0;
    }
    spin_unlock_irqrestore(&status_lock, flags);
}

static void parse_mouse_report(const unsigned char *data, int len)
{
    unsigned long flags;
    int left;
    int right;
    int middle;
    int dx;
    int dy;
    int wheel = 0;

    if (!data || len < 3)
        return;

    left = !!(data[0] & 0x01);
    right = !!(data[0] & 0x02);
    middle = !!(data[0] & 0x04);
    dx = (signed char)data[1];
    dy = (signed char)data[2];
    if (len >= 4)
        wheel = (signed char)data[3];

    spin_lock_irqsave(&status_lock, flags);
    current_status.connected = true;
    current_status.left = left;
    current_status.right = right;
    current_status.middle = middle;
    current_status.dx = dx;
    current_status.dy = dy;
    current_status.wheel = wheel;
    spin_unlock_irqrestore(&status_lock, flags);

    if (left || right || middle || dx || dy || wheel) {
        pr_info("[%s] left=%d right=%d middle=%d dx=%d dy=%d wheel=%d\n",
                MODULE_TAG, left, right, middle, dx, dy, wheel);
    }
}

static void mouse_irq_callback(struct urb *urb)
{
    struct mouse_device *mouse = urb->context;
    int ret;

    if (!mouse)
        return;

    switch (urb->status) {
    case 0:
        parse_mouse_report(mouse->data, urb->actual_length);
        break;
    case -ECONNRESET:
    case -ENOENT:
    case -ESHUTDOWN:
        return;
    default:
        break;
    }

    ret = usb_submit_urb(urb, GFP_ATOMIC);
    if (ret)
        pr_warn("%s: usb_submit_urb failed: %d\n", MODULE_TAG, ret);
}

static int mouse_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(interface);
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint = NULL;
    struct mouse_device *mouse;
    int pipe;
    int maxp;
    int ret;
    int i;

    (void)id;

    if (monitored_mouse) {
        pr_info("%s: another mouse is already monitored\n", MODULE_TAG);
        return -EBUSY;
    }

    iface_desc = interface->cur_altsetting;
    if (!iface_desc)
        return -ENODEV;

    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;
        if (usb_endpoint_is_int_in(endpoint))
            break;
        endpoint = NULL;
    }
    if (!endpoint) {
        pr_warn("%s: interrupt IN endpoint not found\n", MODULE_TAG);
        return -ENODEV;
    }

    mouse = kzalloc(sizeof(*mouse), GFP_KERNEL);
    if (!mouse)
        return -ENOMEM;

    mouse->udev = usb_get_dev(udev);
    mouse->interface = interface;
    mouse->endpoint_addr = endpoint->bEndpointAddress;
    mouse->data_size = usb_endpoint_maxp(endpoint);
    if (mouse->data_size < 4)
        mouse->data_size = 4;

    mouse->data = usb_alloc_coherent(mouse->udev, mouse->data_size,
                                     GFP_KERNEL, &mouse->data_dma);
    if (!mouse->data) {
        ret = -ENOMEM;
        goto fail_free_mouse;
    }

    mouse->irq_urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!mouse->irq_urb) {
        ret = -ENOMEM;
        goto fail_free_buffer;
    }

    pipe = usb_rcvintpipe(mouse->udev, mouse->endpoint_addr);
    maxp = usb_maxpacket(mouse->udev, pipe);
    if (maxp > mouse->data_size)
        maxp = mouse->data_size;

    usb_fill_int_urb(mouse->irq_urb, mouse->udev, pipe, mouse->data, maxp,
                     mouse_irq_callback, mouse, endpoint->bInterval);
    mouse->irq_urb->transfer_dma = mouse->data_dma;
    mouse->irq_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    usb_set_intfdata(interface, mouse);
    monitored_mouse = mouse;
    update_connected(true);

    ret = usb_submit_urb(mouse->irq_urb, GFP_KERNEL);
    if (ret) {
        pr_err("%s: usb_submit_urb failed: %d\n", MODULE_TAG, ret);
        monitored_mouse = NULL;
        usb_set_intfdata(interface, NULL);
        update_connected(false);
        goto fail_free_urb;
    }

    pr_info("%s: USB mouse connected vendor=%04x product=%04x\n",
            MODULE_TAG, le16_to_cpu(udev->descriptor.idVendor),
            le16_to_cpu(udev->descriptor.idProduct));
    return 0;

fail_free_urb:
    usb_free_urb(mouse->irq_urb);
fail_free_buffer:
    usb_free_coherent(mouse->udev, mouse->data_size, mouse->data, mouse->data_dma);
fail_free_mouse:
    usb_put_dev(mouse->udev);
    kfree(mouse);
    return ret;
}

static void mouse_disconnect(struct usb_interface *interface)
{
    struct mouse_device *mouse = usb_get_intfdata(interface);

    usb_set_intfdata(interface, NULL);
    if (!mouse)
        return;

    if (mouse->irq_urb)
        usb_kill_urb(mouse->irq_urb);

    if (monitored_mouse == mouse)
        monitored_mouse = NULL;

    update_connected(false);
    pr_info("%s: USB mouse disconnected\n", MODULE_TAG);

    usb_free_urb(mouse->irq_urb);
    usb_free_coherent(mouse->udev, mouse->data_size, mouse->data, mouse->data_dma);
    usb_put_dev(mouse->udev);
    kfree(mouse);
}

static ssize_t proc_status_read(struct file *file, char __user *user_buffer,
                                size_t count, loff_t *offset)
{
    struct mouse_status snapshot;
    unsigned long flags;
    char buffer[STATUS_BUF_LEN];
    int len;

    spin_lock_irqsave(&status_lock, flags);
    snapshot = current_status;
    spin_unlock_irqrestore(&status_lock, flags);

    len = scnprintf(buffer, sizeof(buffer),
                    "connected=%d\n"
                    "left=%d\n"
                    "right=%d\n"
                    "middle=%d\n"
                    "dx=%d\n"
                    "dy=%d\n"
                    "wheel=%d\n",
                    snapshot.connected ? 1 : 0,
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

static const struct usb_device_id mouse_table[] = {
    { USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                         USB_INTERFACE_SUBCLASS_BOOT,
                         USB_INTERFACE_PROTOCOL_MOUSE) },
    {}
};
MODULE_DEVICE_TABLE(usb, mouse_table);

static struct usb_driver mouse_driver = {
    .name = MODULE_TAG,
    .probe = mouse_probe,
    .disconnect = mouse_disconnect,
    .id_table = mouse_table,
};

static int __init usb_mouse_monitor_init(void)
{
    int ret;

    spin_lock_init(&status_lock);
    memset(&current_status, 0, sizeof(current_status));

    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_status_ops);
    if (!proc_entry) {
        pr_err("%s: failed to create /proc/%s\n", MODULE_TAG, PROC_NAME);
        return -ENOMEM;
    }

    ret = usb_register(&mouse_driver);
    if (ret) {
        proc_remove(proc_entry);
        proc_entry = NULL;
        pr_err("%s: usb_register failed: %d\n", MODULE_TAG, ret);
        return ret;
    }

    pr_info("%s loaded\n", MODULE_TAG);
    return 0;
}

static void __exit usb_mouse_monitor_exit(void)
{
    usb_deregister(&mouse_driver);
    if (proc_entry)
        proc_remove(proc_entry);
    pr_info("%s unloaded\n", MODULE_TAG);
}

module_init(usb_mouse_monitor_init);
module_exit(usb_mouse_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Programming Student");
MODULE_DESCRIPTION("USB HID mouse interrupt report monitor");
MODULE_VERSION("1.0");
