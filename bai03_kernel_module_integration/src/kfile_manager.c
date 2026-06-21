#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/mutex.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/user_namespace.h>
#include <linux/version.h>

#define DEVICE_NAME "kfile_manager"
#define CLASS_NAME "kfile_manager"
#define DEFAULT_ROOT "/tmp/kfile_manager_root"

#define MAX_COMMAND_LEN 1024
#define MAX_FILENAME_LEN 128
#define MAX_CONTENT_LEN 4096
#define MAX_PATH_LEN 512
#define MAX_RESULT_LEN 8192

static dev_t kfile_dev;
static struct cdev kfile_cdev;
static struct class *kfile_class;
static struct device *kfile_device;

static DEFINE_MUTEX(state_lock);
static char root_dir[MAX_PATH_LEN] = DEFAULT_ROOT;
static char result_buffer[MAX_RESULT_LEN];
static size_t result_len;
static char last_command[32] = "NONE";
static char last_result[32] = "NONE";
static unsigned long total_commands;

static void set_result(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    result_len = vscnprintf(result_buffer, MAX_RESULT_LEN, fmt, args);
    va_end(args);
}

static void append_result(const char *fmt, ...)
{
    va_list args;
    size_t remain;

    if (result_len >= MAX_RESULT_LEN - 1)
        return;

    remain = MAX_RESULT_LEN - result_len;
    va_start(args, fmt);
    result_len += vscnprintf(result_buffer + result_len, remain, fmt, args);
    va_end(args);
}

static void log_command(const char *cmd, const char *file, const char *status)
{
    strscpy(last_command, cmd ? cmd : "UNKNOWN", sizeof(last_command));
    strscpy(last_result, status ? status : "UNKNOWN", sizeof(last_result));
    total_commands++;

    pr_info("[kfile_manager] PID=%d COMM=%s CMD=%s FILE=%s RESULT=%s\n",
            current->pid,
            current->comm,
            cmd ? cmd : "UNKNOWN",
            file ? file : "-",
            status ? status : "UNKNOWN");
}

static bool safe_root_path(const char *path)
{
    if (!path || !*path)
        return false;

    if (!strcmp(path, "/") || !strcmp(path, "/etc") ||
        !strcmp(path, "/bin") || !strcmp(path, "/usr") ||
        !strcmp(path, "/boot") || !strcmp(path, "/sbin") ||
        !strcmp(path, "/lib") || !strcmp(path, "/lib64"))
        return false;

    return !strncmp(path, "/tmp/", 5) || !strcmp(path, "/tmp") ||
           !strncmp(path, "/home/", 6);
}

static bool validate_filename(const char *name)
{
    int i;

    if (!name || !*name)
        return false;
    if (strlen(name) >= MAX_FILENAME_LEN)
        return false;
    if (name[0] == '/' || strstr(name, ".."))
        return false;

    for (i = 0; name[i]; i++) {
        char c = name[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')
            continue;
        return false;
    }

    return true;
}

static int build_safe_path(const char *filename, char *out, size_t out_len)
{
    if (!validate_filename(filename))
        return -EINVAL;

    if (scnprintf(out, out_len, "%s/%s", root_dir, filename) >= out_len)
        return -ENAMETOOLONG;

    return 0;
}

static int ensure_root_dir(const char *path)
{
    struct path root_path;
    int ret;

    ret = kern_path(path, LOOKUP_DIRECTORY, &root_path);
    if (!ret) {
        path_put(&root_path);
        return 0;
    }

    pr_warn("kfile_manager: root directory does not exist: %s\n", path);
    pr_warn("kfile_manager: create it from user-space with: sudo mkdir -p %s\n", path);
    return ret;
}

static int file_create(const char *filename)
{
    struct file *file;
    char path[MAX_PATH_LEN];
    int ret;

    ret = build_safe_path(filename, path, sizeof(path));
    if (ret)
        return ret;

    file = filp_open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (IS_ERR(file))
        return PTR_ERR(file);
    filp_close(file, NULL);
    return 0;
}

static int file_write_content(const char *filename, const char *content, bool append)
{
    struct file *file;
    char path[MAX_PATH_LEN];
    loff_t pos = 0;
    int ret;
    ssize_t written;

    if (!content)
        content = "";
    if (strlen(content) > MAX_CONTENT_LEN)
        return -E2BIG;

    ret = build_safe_path(filename, path, sizeof(path));
    if (ret)
        return ret;

    file = filp_open(path, O_CREAT | O_WRONLY | (append ? O_APPEND : O_TRUNC), 0644);
    if (IS_ERR(file))
        return PTR_ERR(file);

    if (append)
        pos = i_size_read(file_inode(file));

    written = kernel_write(file, content, strlen(content), &pos);
    filp_close(file, NULL);
    return written < 0 ? (int)written : 0;
}

static int file_read_content(const char *filename)
{
    struct file *file;
    char path[MAX_PATH_LEN];
    char *buf;
    loff_t pos = 0;
    ssize_t n;
    int ret;

    ret = build_safe_path(filename, path, sizeof(path));
    if (ret)
        return ret;

    buf = kzalloc(MAX_CONTENT_LEN + 1, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    file = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(file)) {
        ret = PTR_ERR(file);
        goto out_free;
    }

    n = kernel_read(file, buf, MAX_CONTENT_LEN, &pos);
    filp_close(file, NULL);
    if (n < 0) {
        ret = (int)n;
        goto out_free;
    }

    buf[n] = '\0';
    set_result("READ %s\n%s\n", filename, buf);
    ret = 0;

out_free:
    kfree(buf);
    return ret;
}

static int file_delete(const char *filename)
{
    struct path file_path;
    struct dentry *parent;
    char full_path[MAX_PATH_LEN];
    int ret;

    ret = build_safe_path(filename, full_path, sizeof(full_path));
    if (ret)
        return ret;

    ret = kern_path(full_path, LOOKUP_FOLLOW, &file_path);
    if (ret)
        return ret;

    parent = dget_parent(file_path.dentry);
    inode_lock(d_inode(parent));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
    ret = vfs_unlink(mnt_idmap(file_path.mnt), d_inode(parent), file_path.dentry, NULL);
#else
    ret = vfs_unlink(mnt_user_ns(file_path.mnt), d_inode(parent), file_path.dentry, NULL);
#endif
    inode_unlock(d_inode(parent));
    dput(parent);
    path_put(&file_path);
    return ret;
}

static int file_copy(const char *src, const char *dst)
{
    struct file *in;
    struct file *out;
    char src_path[MAX_PATH_LEN];
    char dst_path[MAX_PATH_LEN];
    char *buf;
    loff_t in_pos = 0;
    loff_t out_pos = 0;
    ssize_t n;
    int ret;

    ret = build_safe_path(src, src_path, sizeof(src_path));
    if (ret)
        return ret;
    ret = build_safe_path(dst, dst_path, sizeof(dst_path));
    if (ret)
        return ret;

    buf = kmalloc(MAX_CONTENT_LEN, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    in = filp_open(src_path, O_RDONLY, 0);
    if (IS_ERR(in)) {
        ret = PTR_ERR(in);
        goto out_free;
    }

    out = filp_open(dst_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (IS_ERR(out)) {
        ret = PTR_ERR(out);
        filp_close(in, NULL);
        goto out_free;
    }

    while ((n = kernel_read(in, buf, MAX_CONTENT_LEN, &in_pos)) > 0) {
        ssize_t w = kernel_write(out, buf, n, &out_pos);
        if (w < 0) {
            ret = (int)w;
            goto out_close;
        }
        if (w != n) {
            ret = -EIO;
            goto out_close;
        }
    }
    ret = n < 0 ? (int)n : 0;

out_close:
    filp_close(out, NULL);
    filp_close(in, NULL);
out_free:
    kfree(buf);
    return ret;
}

static int file_info(const char *filename)
{
    struct path path;
    struct inode *inode;
    char full_path[MAX_PATH_LEN];
    struct timespec64 ts;
    int ret;

    ret = build_safe_path(filename, full_path, sizeof(full_path));
    if (ret)
        return ret;

    ret = kern_path(full_path, LOOKUP_FOLLOW, &path);
    if (ret)
        return ret;

    inode = d_inode(path.dentry);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
    ts = inode_get_mtime(inode);
#else
    ts = inode->i_mtime;
#endif

    set_result("INFO %s\nfull_path=%s\ntype=%s\nsize=%lld\nmode=%o\nuid=%u\ngid=%u\nmtime=%lld\n",
               filename,
               full_path,
               S_ISDIR(inode->i_mode) ? "Folder" : "File",
               (long long)i_size_read(inode),
               inode->i_mode & 0777,
               from_kuid(&init_user_ns, inode->i_uid),
               from_kgid(&init_user_ns, inode->i_gid),
               (long long)ts.tv_sec);
    path_put(&path);
    return 0;
}

struct list_ctx {
    struct dir_context ctx;
};

static bool list_actor(struct dir_context *ctx, const char *name, int namelen,
                       loff_t offset, u64 ino, unsigned int d_type)
{
    char entry[MAX_FILENAME_LEN];

    (void)ctx;
    (void)offset;
    (void)ino;
    (void)d_type;

    if (namelen == 1 && name[0] == '.')
        return true;
    if (namelen == 2 && name[0] == '.' && name[1] == '.')
        return true;

    if (namelen >= sizeof(entry))
        namelen = sizeof(entry) - 1;
    memcpy(entry, name, namelen);
    entry[namelen] = '\0';
    append_result("%s\n", entry);
    return true;
}

static int file_list(void)
{
    struct file *dir;
    struct list_ctx lctx = {
        .ctx.actor = list_actor,
    };

    dir = filp_open(root_dir, O_RDONLY | O_DIRECTORY, 0);
    if (IS_ERR(dir))
        return PTR_ERR(dir);

    set_result("LIST %s\n", root_dir);
    iterate_dir(dir, &lctx.ctx);
    filp_close(dir, NULL);
    return 0;
}

static int command_set_root(char *path)
{
    int ret;

    path = strim(path);
    if (!safe_root_path(path))
        return -EINVAL;

    ret = ensure_root_dir(path);
    if (ret)
        return ret;

    strscpy(root_dir, path, sizeof(root_dir));
    set_result("OK SET_ROOT %s\n", root_dir);
    return 0;
}

static void command_help(void)
{
    set_result("Commands:\n"
               "SET_ROOT path\n"
               "CREATE filename\n"
               "WRITE filename content\n"
               "APPEND filename content\n"
               "READ filename\n"
               "DELETE filename\n"
               "RENAME oldname newname\n"
               "COPY src dst\n"
               "INFO filename\n"
               "LIST\n"
               "STATUS\n"
               "HELP\n");
}

static int execute_command(char *command)
{
    char *cursor = command;
    char *cmd;
    char *arg1;
    char *arg2;
    char *content;
    const char *log_file = "-";
    int ret = 0;

    cursor = strim(cursor);
    cmd = strsep(&cursor, " ");
    if (!cmd || !*cmd)
        return -EINVAL;
    if (cursor)
        cursor = skip_spaces(cursor);

    if (!strcasecmp(cmd, "SET_ROOT")) {
        if (!cursor || !*cursor)
            return -EINVAL;
        ret = command_set_root(cursor);
        log_file = cursor;
    } else if (!strcasecmp(cmd, "CREATE")) {
        arg1 = strsep(&cursor, " ");
        log_file = arg1;
        ret = file_create(arg1);
        if (!ret)
            set_result("OK CREATE %s\n", arg1);
    } else if (!strcasecmp(cmd, "WRITE") || !strcasecmp(cmd, "APPEND")) {
        arg1 = strsep(&cursor, " ");
        log_file = arg1;
        content = cursor ? cursor : "";
        ret = file_write_content(arg1, content, !strcasecmp(cmd, "APPEND"));
        if (!ret)
            set_result("OK %s %s\n", cmd, arg1);
    } else if (!strcasecmp(cmd, "READ")) {
        arg1 = strsep(&cursor, " ");
        log_file = arg1;
        ret = file_read_content(arg1);
    } else if (!strcasecmp(cmd, "DELETE")) {
        arg1 = strsep(&cursor, " ");
        log_file = arg1;
        ret = file_delete(arg1);
        if (!ret)
            set_result("OK DELETE %s\n", arg1);
    } else if (!strcasecmp(cmd, "COPY")) {
        arg1 = strsep(&cursor, " ");
        arg2 = cursor ? strim(cursor) : NULL;
        log_file = arg1;
        ret = file_copy(arg1, arg2);
        if (!ret)
            set_result("OK COPY %s %s\n", arg1, arg2);
    } else if (!strcasecmp(cmd, "RENAME")) {
        arg1 = strsep(&cursor, " ");
        arg2 = cursor ? strim(cursor) : NULL;
        log_file = arg1;
        ret = file_copy(arg1, arg2);
        if (!ret)
            ret = file_delete(arg1);
        if (!ret)
            set_result("OK RENAME %s %s\n", arg1, arg2);
    } else if (!strcasecmp(cmd, "INFO")) {
        arg1 = strsep(&cursor, " ");
        log_file = arg1;
        ret = file_info(arg1);
    } else if (!strcasecmp(cmd, "LIST")) {
        ret = file_list();
    } else if (!strcasecmp(cmd, "STATUS")) {
        set_result("STATUS\nroot=%s\nlast_command=%s\nlast_result=%s\ntotal_commands=%lu\n",
                   root_dir, last_command, last_result, total_commands);
    } else if (!strcasecmp(cmd, "HELP")) {
        command_help();
    } else {
        ret = -EINVAL;
    }

    log_command(cmd, log_file, ret ? "ERROR" : "OK");
    if (ret)
        set_result("ERROR %s: %d\n", cmd, ret);
    return ret;
}

static int kfile_open(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    return 0;
}

static ssize_t kfile_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    ssize_t ret;

    (void)file;
    mutex_lock(&state_lock);
    ret = simple_read_from_buffer(buf, count, ppos, result_buffer, result_len);
    mutex_unlock(&state_lock);
    return ret;
}

static ssize_t kfile_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char *command;
    int ret;

    (void)file;
    (void)ppos;

    if (count == 0 || count > MAX_COMMAND_LEN)
        return -EINVAL;

    command = kzalloc(count + 1, GFP_KERNEL);
    if (!command)
        return -ENOMEM;

    if (copy_from_user(command, buf, count)) {
        kfree(command);
        return -EFAULT;
    }
    command[count] = '\0';
    strim(command);

    mutex_lock(&state_lock);
    ret = execute_command(command);
    mutex_unlock(&state_lock);

    kfree(command);
    return ret ? ret : count;
}

static int kfile_release(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    return 0;
}

static const struct file_operations kfile_fops = {
    .owner = THIS_MODULE,
    .open = kfile_open,
    .read = kfile_read,
    .write = kfile_write,
    .release = kfile_release,
};

static int __init kfile_manager_init(void)
{
    int ret;

    mutex_lock(&state_lock);
    strscpy(root_dir, DEFAULT_ROOT, sizeof(root_dir));
    result_len = 0;
    total_commands = 0;
    set_result("kfile_manager ready\nroot=%s\n", root_dir);
    mutex_unlock(&state_lock);

    ret = ensure_root_dir(DEFAULT_ROOT);
    if (ret)
        pr_warn("kfile_manager: cannot create default root %s: %d\n", DEFAULT_ROOT, ret);

    ret = alloc_chrdev_region(&kfile_dev, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&kfile_cdev, &kfile_fops);
    ret = cdev_add(&kfile_cdev, kfile_dev, 1);
    if (ret)
        goto fail_region;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    kfile_class = class_create(CLASS_NAME);
#else
    kfile_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(kfile_class)) {
        ret = PTR_ERR(kfile_class);
        goto fail_cdev;
    }

    kfile_device = device_create(kfile_class, NULL, kfile_dev, NULL, DEVICE_NAME);
    if (IS_ERR(kfile_device)) {
        ret = PTR_ERR(kfile_device);
        goto fail_class;
    }

    pr_info("kfile_manager loaded\n");
    return 0;

fail_class:
    class_destroy(kfile_class);
fail_cdev:
    cdev_del(&kfile_cdev);
fail_region:
    unregister_chrdev_region(kfile_dev, 1);
    return ret;
}

static void __exit kfile_manager_exit(void)
{
    device_destroy(kfile_class, kfile_dev);
    class_destroy(kfile_class);
    cdev_del(&kfile_cdev);
    unregister_chrdev_region(kfile_dev, 1);
    pr_info("kfile_manager unloaded\n");
}

module_init(kfile_manager_init);
module_exit(kfile_manager_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Programming Student");
MODULE_DESCRIPTION("Kernel-space file manager through a character device");
MODULE_VERSION("1.0");
