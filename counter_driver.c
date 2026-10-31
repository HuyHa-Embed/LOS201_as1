#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>

#define DEVICE_NAME "counter"

static atomic_t counter = ATOMIC_INIT(0);
static atomic_t read_count = ATOMIC_INIT(0);
static atomic_t write_count = ATOMIC_INIT(0);

static struct proc_dir_entry *counter_proc_entry;

/*
 * Purpose:
 *   Read the current counter value and then increment it by one.
 *
 * Parameters:
 *   file  - pointer to the opened file.
 *   user_buffer - userspace buffer receiving the counter string.
 *   count - maximum number of bytes requested by userspace.
 *   ppos  - current file offset.
 *
 * Return:
 *   Number of bytes copied on success, 0 when the file has been fully read,
 *   or a negative error code on failure.
 */
static ssize_t counter_read(
    struct file *file,
    char __user *user_buffer,
    size_t count,
    loff_t *ppos)
{
    char buffer[32];
    int len;
    int value;

    if (*ppos != 0)
        return 0;

    value = atomic_read(&counter);
    len = scnprintf(buffer, sizeof(buffer), "%d\n", value);

    if (count < len)
        return -EINVAL;

    if (copy_to_user(user_buffer, buffer, len))
        return -EFAULT;

    *ppos += len;

    atomic_inc(&counter);
    atomic_inc(&read_count);

    return len;
}

/*
 * Purpose:
 *   Set the counter from an ASCII integer written by userspace.
 *
 * Parameters:
 *   file  - pointer to the opened file.
 *   user_buffer - userspace buffer containing the integer.
 *   count - number of bytes supplied by userspace.
 *   ppos  - current file offset.
 *
 * Return:
 *   Number of bytes consumed on success, or a negative error code on failure.
 */
static ssize_t counter_write(
    struct file *file,
    const char __user *user_buffer,
    size_t count,
    loff_t *ppos)
{
    char buffer[32];
    int value;
    int ret;

    if (count == 0 || count >= sizeof(buffer))
        return -EINVAL;

    if (copy_from_user(buffer, user_buffer, count))
        return -EFAULT;

    buffer[count] = '\0';

    ret = kstrtoint(buffer, 10, &value);
    if (ret)
        return -EINVAL;

    if (value < 0)
        return -EINVAL;

    atomic_set(&counter, value);
    atomic_inc(&write_count);

    return count;
}

/*
 * Purpose:
 *   Open the counter device.
 *
 * Parameters:
 *   inode - inode associated with the device.
 *   file  - opened file structure.
 *
 * Return:
 *   0 on success.
 */
static int counter_open(
    struct inode *inode,
    struct file *file)
{
    return 0;
}

/*
 * Purpose:
 *   Display counter statistics through /proc/counter_info.
 *
 * Parameters:
 *   seq - sequential file used to output information.
 *   v   - unused private data.
 *
 * Return:
 *   0 on success.
 */
static int counter_proc_show(
    struct seq_file *seq,
    void *v)
{
    seq_printf(seq, "counter = %d\n", atomic_read(&counter));
    seq_printf(seq, "read_count = %d\n", atomic_read(&read_count));
    seq_printf(seq, "write_count = %d\n", atomic_read(&write_count));

    return 0;
}

/*
 * Purpose:
 *   Open /proc/counter_info for sequential reading.
 *
 * Parameters:
 *   inode - proc filesystem inode.
 *   file  - proc file structure.
 *
 * Return:
 *   0 on success or a negative error code on failure.
 */
static int counter_proc_open(
    struct inode *inode,
    struct file *file)
{
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open = counter_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
    .open = counter_open,
    .read = counter_read,
    .write = counter_write,
};

static struct miscdevice counter_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &counter_fops,
};

/*
 * Purpose:
 *   Initialize and register the counter misc device and proc entry.
 *
 * Parameters:
 *   None.
 *
 * Return:
 *   0 on success or a negative error code on failure.
 */
static int __init counter_init(void)
{
    int ret;

    atomic_set(&counter, 0);
    atomic_set(&read_count, 0);
    atomic_set(&write_count, 0);

    ret = misc_register(&counter_device);
    if (ret) {
        pr_err("counter: misc_register failed\n");
        return ret;
    }

    counter_proc_entry = proc_create(
        "counter_info",
        0444,
        NULL,
        &counter_proc_ops);

    if (!counter_proc_entry) {
        misc_deregister(&counter_device);
        pr_err("counter: proc_create failed\n");
        return -ENOMEM;
    }

    pr_info("counter: driver loaded\n");
    return 0;
}

/*
 * Purpose:
 *   Remove the proc entry and unregister the counter misc device.
 *
 * Parameters:
 *   None.
 *
 * Return:
 *   None.
 */
static void __exit counter_exit(void)
{
    proc_remove(counter_proc_entry);
    misc_deregister(&counter_device);

    pr_info("counter: driver unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HaAnhHuy - SE203732");
MODULE_DESCRIPTION("Assignment 01 misc counter device driver");
