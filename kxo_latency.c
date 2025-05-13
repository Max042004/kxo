// SPDX-License-Identifier: GPL-2.0
#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#define LAT_BUF_SHIFT 12 /* 2^12 = 4096 筆 */
#define LAT_BUF_SIZE (1 << LAT_BUF_SHIFT)
#define LAT_MASK (LAT_BUF_SIZE - 1)

static u64 lat_buf[LAT_BUF_SIZE];
static atomic_t lat_head = ATOMIC_INIT(0);

static struct dentry *kxo_dbg_dir;

/* 供其他檔案呼叫，用來記錄延遲 */
void kxo_lat_record(u64 nsec)
{
    int idx = atomic_fetch_add(1, &lat_head);
    lat_buf[idx & LAT_MASK] = nsec;
}
EXPORT_SYMBOL_GPL(kxo_lat_record);

/* ---------- seq_file 介面 ---------- */
static void *lat_seq_start(struct seq_file *m, loff_t *pos)
{
    u32 head = atomic_read(&lat_head);
    if (*pos >= head) /* 只走到 head-1 就停 */
        return NULL;
    return &lat_buf[*pos & LAT_MASK];
}


static void *lat_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
    ++*pos;
    if (*pos >= LAT_BUF_SIZE)
        return NULL;
    return &lat_buf[*pos];
}

static void lat_seq_stop(struct seq_file *m, void *v) {}

static int lat_seq_show(struct seq_file *m, void *v)
{
    u64 *ptr = v;
    seq_printf(m, "%llu\n", *ptr);
    return 0;
}

static const struct seq_operations lat_seq_ops = {
    .start = lat_seq_start,
    .next = lat_seq_next,
    .stop = lat_seq_stop,
    .show = lat_seq_show,
};

static int lat_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &lat_seq_ops);
}

static const struct file_operations lat_fops = {
    .owner = THIS_MODULE,
    .open = lat_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

/* ---------- 模組生命週期 ---------- */

int __init kxo_lat_debugfs_init(void)
{
    kxo_dbg_dir = debugfs_create_dir("kxo", NULL);
    if (!kxo_dbg_dir) {
        pr_err("kxo_lat: cannot create debugfs dir\n");
        return -ENOMEM;
    }

    if (!debugfs_create_file("latency_ns", 0444, kxo_dbg_dir, NULL,
                             &lat_fops)) {
        pr_err("kxo_lat: cannot create latency_ns file\n");
        debugfs_remove_recursive(kxo_dbg_dir);
        return -ENOMEM;
    }

    pr_info("kxo_lat: debugfs interface ready\n");
    return 0;
}

void __exit kxo_lat_debugfs_exit(void)
{
    debugfs_remove_recursive(kxo_dbg_dir);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("your_name");
MODULE_DESCRIPTION("KXO latency recorder via debugfs");
