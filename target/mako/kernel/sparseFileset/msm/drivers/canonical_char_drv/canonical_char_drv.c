/* 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <asm/uaccess.h>

#include <linux/types.h>
#include <linux/proc_fs.h>

#include <linux/cdev.h>         /* cdev utilities */
#include <linux/circ_buf.h>     /* circular buffer macros */
#include <linux/device.h>
#include <linux/fs.h>           /* file_operations */
#include <linux/init.h>         /* module_init, module_exit */
#include <linux/module.h>       /* for modules */
#include <linux/poll.h>
#include <linux/sched.h>        /* wait queues */
#include <linux/slab.h>         /* kmalloc */
#include <linux/uaccess.h>      /* copy_(to,from)_user */
#include <linux/wait.h>         /* wait queue for blocking ioctl call */

#include "canonical_char_drv.h"

#define DEVICE_NODE_NAME "canonical_char_drv"
#define CANONICAL_CHAR_DRV_DEVICE_CLASS_NAME "canonical_char_drv"
#define CANONICAL_CHAR_DRV_DEVICE_FILE_NAME "canonical_char_drv"
#define CANONICAL_CHAR_DRV_DEVICE_PROC_NAME "canonical_char_drv"

/* mod 2 buffer size */
#define KBUF_SIZE (8*PAGE_SIZE)

/* lifted from serial_core.h */
#define uart_circ_empty(circ)       ((circ)->head == (circ)->tail)
#define uart_circ_clear(circ)       ((circ)->head = (circ)->tail = 0)
#define CHARS_PENDING(circ)         (CIRC_CNT((circ)->head, (circ)->tail, (size_t)KBUF_SIZE))
#define CHARS_PENDING_TO_END(circ)  (CIRC_CNT_TO_END((circ)->head, (circ)->tail, (size_t)KBUF_SIZE))
#define CHARS_FREE(circ)            (CIRC_SPACE((circ)->head, (circ)->tail, (size_t)KBUF_SIZE))
#define CHARS_FREE_TO_END(circ)     (CIRC_SPACE_TO_END((circ)->head, (circ)->tail, (size_t)KBUF_SIZE))

struct CanonicalCharDevice {
    struct circ_buf circBuf;                /* dynamically allocated buffer */
    wait_queue_head_t wq_read;              /* read data available sync */
    wait_queue_head_t wq_write;             /* write buffer space available sync */
    atomic_t data_ready_to_read;
    atomic_t num_async_inputs;              /* async input writes which occured since last ASNYC_INPUT_BLOCK call */
    atomic_t buffer_ready_to_write;
    int dummyMode;                          /* set a "mode" for the device (for procfs, sysfs, and ioctl access) */
    ssize_t bufSize;                        /* buffer depth */
    struct semaphore sem;                   /* buffer access semaphore */
    struct cdev dev;                        /* charactor device */

    // TODO consider using fcntl (... FASYNC ... ) to generate signals to client process instead of blocking client calls
    wait_queue_head_t wq_async_input_block; /* signaling for blocking async input */
};
static struct CanonicalCharDevice* pCanonicalCharDevice = NULL;

/* sysfs */
static struct class* canonical_char_drv_class = NULL;

static dev_t dev = 0;
static int major = 0, minor = 0;
static struct cdev* pCdev;

/* driver fops */
static int drv_open (struct inode *inode, struct file *file);
static int drv_release (struct inode *inode, struct file *file);
static ssize_t drv_read (struct file *file, char __user * buf, size_t lbuf, loff_t * ppos);
static ssize_t drv_write (struct file *file, const char __user * buf, size_t lbuf, loff_t * ppos);
static unsigned int drv_poll (struct file *, struct poll_table_struct *);
static long drv_ioctl (struct file *, unsigned int, unsigned long);

/* the new ioctl
 * https://lwn.net/Articles/119652/
 */
static const struct file_operations canonical_char_drv_fops = {
    .owner = THIS_MODULE,
    .read = drv_read,
    .write = drv_write,
    .poll = drv_poll,
    .unlocked_ioctl = drv_ioctl,    /* gets rid of the Big Kernel Lock used in ioctl */
    .compat_ioctl = drv_ioctl,      /* allows 32 but userspace calls into 64bit kernel */
    .open = drv_open,
    .release = drv_release,
};


/* Note: device attributes
    https://www.kernel.org/doc/Documentation/driver-model/device.txt
*/
/* sysfs show and store mode */
static ssize_t mode_show (struct device* dev, struct device_attribute* attr, char* buf);
static ssize_t mode_store (struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
static DEVICE_ATTR (mode, S_IRUGO | S_IWUSR, mode_show, mode_store);


static int 
drv_open (struct inode *inode, struct file * pFile)
{
    struct CanonicalCharDevice* pCharDevice;
    printk(KERN_INFO DEVICE_NODE_NAME": drv_open()\n");

    pCharDevice = container_of(inode->i_cdev, struct CanonicalCharDevice, dev);
    pFile->private_data = pCharDevice;

    return 0;
}


static int 
drv_release (struct inode *inode, struct file *file) {

    struct CanonicalCharDevice* dev = file->private_data; 

    /* release any wait queued ASNYC_INPUT_BLOCK client */
    wake_up_interruptible(&dev->wq_async_input_block);
    printk(KERN_INFO DEVICE_NODE_NAME": drv_release()\n");
    return 0;
}


static ssize_t
drv_read (struct file *file, char __user *buf, size_t lbuf, loff_t *ppos) {

    struct CanonicalCharDevice* pCharDevice = file->private_data; 
    struct circ_buf* pCircBuf = &(pCharDevice->circBuf);
    ssize_t retVal = 0;
    int chars_pending, chars_pending_to_end;
    printk(KERN_INFO DEVICE_NODE_NAME": drv_read()\n");

    if (down_interruptible(&(pCharDevice->sem))) {
        return -ERESTARTSYS; 
    }

    chars_pending = CHARS_PENDING(pCircBuf);
    chars_pending_to_end = CHARS_PENDING_TO_END(pCircBuf);

    printk(KERN_INFO DEVICE_NODE_NAME": lbuf=0x%x, chars_pending=0x%x, chars_pending_to_end:0x%x\n", lbuf, chars_pending, chars_pending_to_end);

    lbuf = (lbuf < chars_pending) ? lbuf : chars_pending;
    //if (lbuf <= chars_pending) {

        if (lbuf <= chars_pending_to_end) {

            /* single segment read from circular buffer */
            if (!copy_to_user(buf, pCircBuf->buf + pCircBuf->tail, lbuf)) {

                printk(KERN_INFO DEVICE_NODE_NAME": read 0x%x (avail:0x%x) from tail 0x%x\n", lbuf, pCharDevice->bufSize, pCircBuf->tail);

                pCircBuf->tail = (pCircBuf->tail + lbuf) & (KBUF_SIZE - 1);
                pCharDevice->bufSize -= lbuf;
                retVal = lbuf;

                /* buffer space available to write */
                atomic_set(&pCharDevice->buffer_ready_to_write, 1);

                /* no more data available to read */
                if (0 == CHARS_PENDING(pCircBuf)) atomic_set(&pCharDevice->data_ready_to_read, 0);

            } else {
                printk(KERN_ERR DEVICE_NODE_NAME": copy_to_user() failed\n");
                retVal = -EFAULT;
                goto out;
            }

        } else {

            /* dual segment read from circular buffer */
            if (!copy_to_user(buf, pCircBuf->buf + pCircBuf->tail, chars_pending_to_end)) {

                printk(KERN_INFO DEVICE_NODE_NAME": read 0x%x (avail:0x%x) from tail 0x%x\n", chars_pending_to_end, pCharDevice->bufSize, pCircBuf->tail);

                pCircBuf->tail = (pCircBuf->tail + chars_pending_to_end) & (KBUF_SIZE - 1);
                pCharDevice->bufSize -= chars_pending_to_end;
                retVal = chars_pending_to_end;

                if (copy_to_user(pCircBuf->buf, buf, lbuf - chars_pending_to_end)) {

                    printk(KERN_INFO DEVICE_NODE_NAME": read 0x%x (avail:0x%x) from tail 0x%x\n", (lbuf - chars_pending_to_end), pCharDevice->bufSize, pCircBuf->tail);

                    pCircBuf->tail = (pCircBuf->tail + (lbuf - chars_pending_to_end)) & (KBUF_SIZE - 1);
                    pCharDevice->bufSize -= (lbuf - chars_pending_to_end);
                    retVal = lbuf;

                    /* buffer space available to write */
                    atomic_set(&pCharDevice->buffer_ready_to_write, 1);

                    /* no more data available to read */
                    if (0 == CHARS_PENDING(pCircBuf)) atomic_set(&pCharDevice->data_ready_to_read, 0);

                } else {
                    printk(KERN_ERR DEVICE_NODE_NAME": copy_from_user() failed\n");
                    retVal = -EFAULT;
                    goto out;
                }
            } else {
                printk(KERN_ERR DEVICE_NODE_NAME": copy_from_user() failed\n");
                retVal = -EFAULT;
                goto out;
            }
        }

        printk(KERN_INFO DEVICE_NODE_NAME": read success: nbytes=0x%x, head = 0x%x, tail = 0x%x, pos=0x%x\n", 
            retVal, pCircBuf->head, pCircBuf->tail, (int)*ppos);

    // } else {
    //     printk(KERN_ERR DEVICE_NODE_NAME": read buffer not full\n");
    //     retVal = -EFAULT;
    //}

    out:
    up(&(pCharDevice->sem));

    return retVal;
}


static ssize_t
drv_write (struct file *file, const char __user * buf, size_t lbuf, loff_t * ppos) {

    struct CanonicalCharDevice* pCharDevice = file->private_data;
    struct circ_buf* pCircBuf = &(pCharDevice->circBuf);
    ssize_t retVal = 0;
    int chars_free, chars_free_to_end;

    printk(KERN_INFO DEVICE_NODE_NAME": drv_write()\n");

    if (down_interruptible(&(pCharDevice->sem))) {
        return -ERESTARTSYS; 
    }

    chars_free = CHARS_FREE(pCircBuf);
    chars_free_to_end = CHARS_FREE_TO_END(pCircBuf);

    /* single or dual segment write */
    /* TODO allow partial writes returning number byte written */
    if (lbuf <= chars_free) {

        if (lbuf <= chars_free_to_end) {

            printk(KERN_INFO DEVICE_NODE_NAME": write 0x%x (avail: 0x%x) to head 0x%x\n", lbuf, chars_free, pCircBuf->head);

            /* single segment write into circular buffer */
            if (!copy_from_user(pCircBuf->buf + pCircBuf->head, buf, lbuf)) {

                pCircBuf->head = (pCircBuf->head + lbuf) & (KBUF_SIZE - 1);
                pCharDevice->bufSize += lbuf;
                retVal = lbuf;

                /* buffer full */
                if (0 == CHARS_FREE(pCircBuf)) atomic_set(&pCharDevice->buffer_ready_to_write, 0);

            } else {
                printk(KERN_ERR DEVICE_NODE_NAME": copy_from_user() failed\n");
                retVal = -EFAULT;
                goto out;
            }
        }
        else {

            /* dual segment write into circular buffer */
            if (!copy_from_user(pCircBuf->buf + pCircBuf->head, buf, chars_free_to_end)) {

                printk(KERN_INFO DEVICE_NODE_NAME": write 0x%x (avail: 0x%x) to head 0x%x\n", chars_free_to_end, chars_free, pCircBuf->head);

                pCircBuf->head = (pCircBuf->head + chars_free_to_end) & (KBUF_SIZE - 1);
                pCharDevice->bufSize -= chars_free_to_end;
                retVal = chars_free_to_end;

                if (copy_from_user(pCircBuf->buf, buf + chars_free_to_end, lbuf - chars_free_to_end)) {

                    printk(KERN_INFO DEVICE_NODE_NAME": write 0x%x (avail: 0x%x) to head 0x%d=x\n", (lbuf - chars_free_to_end), chars_free - chars_free_to_end, pCircBuf->head);

                    pCircBuf->head = (pCircBuf->head + (lbuf - chars_free_to_end)) & (KBUF_SIZE - 1);
                    pCharDevice->bufSize += (lbuf - chars_free_to_end);
                    retVal = lbuf;

                    /* buffer full */
                    if (0 == CHARS_FREE(pCircBuf)) atomic_set(&pCharDevice->buffer_ready_to_write, 0);

                } else {
                    printk(KERN_ERR DEVICE_NODE_NAME": copy_from_user() failed\n");
                    retVal = -EFAULT;
                    goto out;
                }
            } else {
                printk(KERN_ERR DEVICE_NODE_NAME": copy_from_user() failed\n");
                retVal = -EFAULT;
                goto out;
            }
        }

        atomic_inc(&pCharDevice->num_async_inputs);
        atomic_set(&pCharDevice->data_ready_to_read, 1);

        /* wake up any blocked tasks waiting for async input event */
        wake_up_interruptible(&pCharDevice->wq_async_input_block);

        printk(KERN_INFO DEVICE_NODE_NAME": write success: nbytes=0x%x, head = 0x%x, tail = 0x%x, pos=0x%x\n", 
            retVal, pCircBuf->head, pCircBuf->tail, (int)*ppos);
    }
    else
    {
        printk(KERN_ERR DEVICE_NODE_NAME": no space in buffer\n");
        retVal = -ENOMEM;
    }

    out:
    up(&(pCharDevice->sem));

    return retVal;
}


/* poll design
    http://www.makelinux.net/ldd3/chp-6-sect-3

POLLIN
This bit must be set if the device can be read without blocking.

POLLRDNORM
This bit must be set if "normal" data is available for reading. A readable device returns (POLLIN | POLLRDNORM).

POLLRDBAND
This bit indicates that out-of-band data is available for reading from the device. It is currently used only in one place in the Linux kernel (the DECnet code) and is not generally applicable to device drivers.

POLLPRI
High-priority data (out-of-band) can be read without blocking. This bit causes select to report that an exception condition occurred on the file, because select reports out-of-band data as an exception condition.

POLLHUP
When a process reading this device sees end-of-file, the driver must set POLLHUP (hang-up). A process calling select is told that the device is readable, as dictated by the select functionality.

POLLERR
An error condition has occurred on the device. When poll is invoked, the device is reported as both readable and writable, since both read and write return an error code without blocking.

POLLOUT
This bit is set in the return value if the device can be written to without blocking.

POLLWRNORM
This bit has the same meaning as POLLOUT, and sometimes it actually is the same number. A writable device returns (POLLOUT | POLLWRNORM).

POLLWRBAND
Like POLLRDBAND, this bit means that data with nonzero priority can be written to the device. Only the datagram implementation of poll uses this bit, since a datagram can transmit out-of-band data.
*/

static unsigned int drv_poll (struct file *file, poll_table *wait)
{
    unsigned int retVal = 0;
    struct CanonicalCharDevice* pCharDevice = file->private_data;
    // struct circ_buf* pCircBuf = &(pCharDevice->circBuf);

    printk(KERN_INFO DEVICE_NODE_NAME": drv_poll()\n");

    if (down_interruptible(&(pCharDevice->sem))) {
        return -ERESTARTSYS; 
    }

    printk(KERN_INFO DEVICE_NODE_NAME": starting poll waits calls\n");
    poll_wait(file, &pCharDevice->wq_read, wait);
    poll_wait(file, &pCharDevice->wq_write, wait);
    printk(KERN_INFO DEVICE_NODE_NAME": finished poll waits calls\n");

    /* circular buffer read */
    if (atomic_read(&pCharDevice->data_ready_to_read))
        retVal |= POLLIN | POLLRDNORM;  /* data available for reading without blocking */

    /* circular buffer write */
    if (atomic_read(&pCharDevice->buffer_ready_to_write))
        retVal |= POLLOUT | POLLWRNORM; /* data can be written without blocking */

    up(&(pCharDevice->sem));

    return retVal;
}


/* NOTE: ioctl usage
http://www.makelinux.net/ldd3/chp-6-sect-1

The actual nature of the third argument depends on the specific control command being 
issued (the second argument). Some commands take no arguments, some take an integer 
value, and some take a pointer to other data.

There is strong pressure to implement miscellaneous control operations by just about 
any other means. Possible alternatives include embedding commands into the data stream 
(we will discuss this approach later in this chapter) or using virtual filesystems, 
either sysfs or driver-specific filesystems. However, the fact remains that ioctl is 
often the easiest and most straightforward choice for true device operations.
*/

/* NOTE: 
http://tuxthink.blogspot.com/2012/12/implementing-ioctl-call-for-kernel.html

Investigate ioctl_funcs() method for kernel versions above 2.6.39.
*/
static long drv_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    long retVal = -EINVAL;
    int err = 0;

    struct CanonicalCharDevice* pCharDevice = file->private_data;

    printk(KERN_INFO DEVICE_NODE_NAME": drv_ioctl()\n");
    printk(KERN_INFO DEVICE_NODE_NAME": cmd: %x\n", cmd);

    /* validate magic number and ioctl number */
    if (_IOC_TYPE(cmd) != __CANONICAL_CHAR_DRV_IOC) {
        printk(KERN_INFO DEVICE_NODE_NAME": _IOC_TYPE(cmd) != __CANONICAL_CHAR_DRV_IOC\n");
        return retVal;
    }
    if (_IOC_NR(cmd) > __CANONICAL_CHAR_DRV_IOC_MAX_NMBR) {
        printk(KERN_INFO DEVICE_NODE_NAME": _IOC_NR(cmd) > __CANONICAL_CHAR_DRV_IOC_MAX_NMBR\n");
        return retVal;
    }

    /* validate arg read user memory space or write linux memory space */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
    if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;

    /* TODO consider supporting this feature via a read of size 0 */
    if (cmd == ASNYC_INPUT_BLOCK) {

        int timeout_msec;
        if ((retVal = __get_user(timeout_msec, (int __user*)arg)) == 0) {
            unsigned long timeout_jiffies = msecs_to_jiffies(timeout_msec);
            printk(KERN_INFO DEVICE_NODE_NAME": ASNYC_INPUT_BLOCK for %d msec entry\n", timeout_msec);
            /* wait for async input event or timeout */
            /* returns: 0 if the condition evaluated to false after the timeout elapsed
                        1 if the condition evaluated to true after the timeout elapsed, 
                        the remaining jiffies (at least 1) if the condition evaluated to true before the timeout elapsed
                        or -ERESTARTSYS if it was interrupted by a signal. */

            retVal = wait_event_interruptible_timeout(pCharDevice->wq_async_input_block, atomic_read(&pCharDevice->num_async_inputs), timeout_jiffies);
            /* TODO probably make this a while if ERESTARTSYS occurs */
            if (signal_pending(current)) {
                retVal = -ERESTARTSYS;
            } else {
                /* return current buffer size if asnyc input has occured while in ASNYC_INPUT_BLOCK, else return 0 */
                if (retVal) {
                    retVal = __put_user(pCharDevice->bufSize, (int __user*)arg);
                } else {
                    /* timeout occurred */
                    retVal = __put_user(0, (int __user*)arg);
                }
                atomic_set(&pCharDevice->num_async_inputs, 0);
            }
            printk(KERN_INFO DEVICE_NODE_NAME": ASNYC_INPUT_BLOCK exit\n");
        }

    } else {

        if (down_interruptible(&(pCharDevice->sem))) {
            return -ERESTARTSYS; 
        }

        switch (cmd) {
        case GET_BUFFER_CAPACITY:
            printk(KERN_INFO DEVICE_NODE_NAME": GET_BUFFER_CAPACITY, KBUF_SIZE: %d\n", (size_t)KBUF_SIZE);
            retVal = __put_user(KBUF_SIZE, (int __user*)arg);
            break;
        case GET_BUFFER_SIZE:
            printk(KERN_INFO DEVICE_NODE_NAME": GET_BUFFER_SIZE, bufSize: %d\n", pCharDevice->bufSize);
            retVal = __put_user(pCharDevice->bufSize, (int __user*)arg);
            break;
        case FLUSH_BUFFER:
            printk(KERN_INFO DEVICE_NODE_NAME": FLUSH_BUFFER\n");
            /* reset head and tail pointers */
            pCharDevice->circBuf.head = pCharDevice->circBuf.tail = 0;
            pCharDevice->bufSize = 0;
            retVal = 0;
            break;
        }

        up(&(pCharDevice->sem));
    }

    return retVal;
}


/* sysfs mode attribute access */
static ssize_t 
get_mode (struct CanonicalCharDevice* pDevice, char* buf) {

    int mode = 0; 

    printk(KERN_INFO DEVICE_NODE_NAME": get_mode()\n");

    if(down_interruptible(&(pDevice->sem))) { 
        return -ERESTARTSYS; 
    } 

    mode = pDevice->dummyMode; 
    up(&(pDevice->sem)); 

    return snprintf(buf, PAGE_SIZE, "%d\n", mode);
}


/* sysfs mode attribute control */
static ssize_t 
set_mode (struct CanonicalCharDevice* pDevice, const char* buf, size_t count) {

    int mode = 0; 

    printk(KERN_INFO DEVICE_NODE_NAME": set_mode()\n");

    mode = simple_strtol(buf, NULL, 10);

    if(down_interruptible(&(pDevice->sem))) { 
        return -ERESTARTSYS;
    } 

    pDevice->dummyMode = mode; 
    up(&(pDevice->sem));

    return count;
}


static ssize_t 
mode_show (struct device* dev, struct device_attribute* attr, char* buf) {

    struct CanonicalCharDevice* pCanonicalCharDevice = (struct CanonicalCharDevice*)dev_get_drvdata(dev);

    printk (KERN_INFO DEVICE_NODE_NAME": mode_show()\n");

    return get_mode(pCanonicalCharDevice, buf);
}


static ssize_t 
mode_store (struct device* dev, struct device_attribute* attr, const char* buf, size_t count) {

    struct CanonicalCharDevice* pCanonicalCharDevice = (struct CanonicalCharDevice*)dev_get_drvdata(dev);

    printk(KERN_INFO DEVICE_NODE_NAME": mode_store()\n");

    return set_mode(pCanonicalCharDevice, buf, count);
}


/* procfs read, see create_proc() for creation */
static ssize_t 
proc_read (char* page, char** start, off_t off, int count, int* eof, void* data) {

    int retVal;
    static int numCalls = 0;

    printk(KERN_INFO DEVICE_NODE_NAME": proc_read()\n");

    if(down_interruptible(&(pCanonicalCharDevice->sem))) { 
        return -ERESTARTSYS;
    } 

    retVal = snprintf(page, PAGE_SIZE, DEVICE_NODE_NAME": proc_read() called %d times, mode: %d\n", numCalls++, pCanonicalCharDevice->dummyMode);
    if (retVal > 0 && retVal < PAGE_SIZE) {

        int len = snprintf(page + retVal + 1, PAGE_SIZE - retVal - 1, "... buffer dump here ...\n");
        if (len > 0) {

            if (retVal < PAGE_SIZE) {
                retVal = retVal + len + 2;
            } else {
                retVal = len;
            }
        } else {
            printk(KERN_ALERT DEVICE_NODE_NAME": could not formulate proc read string\n");
        }

    } else {
        printk(KERN_ALERT DEVICE_NODE_NAME": could not formulate proc read string\n");
    }

    up(&(pCanonicalCharDevice->sem));

    return retVal;
}


/* procfs write, see create_proc() for creation */
static ssize_t 
proc_write (struct file* file, const char __user *buff, unsigned long len, void* data) {

    struct CanonicalCharDevice* pCharDevice = file->private_data; 
    int retVal = 0;
    char* page = NULL;

    printk(KERN_INFO DEVICE_NODE_NAME": proc_write()\n");

    if(len > PAGE_SIZE) {
        printk(KERN_ALERT DEVICE_NODE_NAME": The buff is too large: %lu.\n", len);
        return -EFAULT;
    }

    page = (char*)__get_free_page(GFP_KERNEL);
    if(!page) { 
        printk(KERN_ALERT DEVICE_NODE_NAME": Failed to alloc page.\n");
        return -ENOMEM;
    } 

    if(copy_from_user(page, buff, len)) {
        printk(KERN_ALERT DEVICE_NODE_NAME": Failed to copy buff from user.\n"); 
        retVal = -EFAULT;
        goto out;
    }

    //retVal = set_mode(pCharDevice, page, len);
    if (pCharDevice != pCanonicalCharDevice) {
        printk(KERN_ALERT DEVICE_NODE_NAME": pCharDevice != pCanonicalCharDevice\n"); 
    }
    retVal = set_mode(pCanonicalCharDevice, page, len);

    out:
    free_page((unsigned long)page);

    return retVal;
}


/* create procfs entry */
static void 
create_proc (void) {

    struct proc_dir_entry* entry;

    printk(KERN_INFO DEVICE_NODE_NAME": create_proc()\n");

    /* create /proc/CANONICAL_CHAR_DRV_DEVICE_PROC_NAME file */
    entry = create_proc_entry(CANONICAL_CHAR_DRV_DEVICE_PROC_NAME, 0, NULL);
    if (entry) {
        entry->read_proc = proc_read;
        entry->write_proc = proc_write;
    }
}


/* remove procfs entry */
static void 
remove_proc (void) {

    printk(KERN_INFO DEVICE_NODE_NAME": remove_proc()\n");

    remove_proc_entry(CANONICAL_CHAR_DRV_DEVICE_PROC_NAME, NULL);
}


/* configure the canonical character device */
static int 
setup_dev (struct CanonicalCharDevice* pDev) {

    int err;
    /* cdev_add() required device number ... which was assigned dynamically */
    dev_t devno = MKDEV(major, minor);

    printk(KERN_INFO DEVICE_NODE_NAME": setup_dev()\n");

    /* null the device */
    memset(pDev, 0, sizeof(struct CanonicalCharDevice));

    /* allocate driver buffer */
    if ((pDev->circBuf.buf = kmalloc(KBUF_SIZE, GFP_KERNEL))) {
        printk(KERN_INFO DEVICE_NODE_NAME": allocated %d bytes for CanonicalCharDev buffer\n", (size_t)KBUF_SIZE);
    } else {
        err = -ENOMEM;
        printk(KERN_ALERT DEVICE_NODE_NAME": Failed to alloc CanonicalCharDev buffer\n");
        return err;
    } 

    /* circular buffer initialization */
#if 0
    pDev->pHead = pDev->pTail = pDev->kbuf;
#else
    /* early test of buffer wrap */
    pDev->circBuf.head = pDev->circBuf.tail = KBUF_SIZE - 20;
#endif
    pDev->bufSize = 0;

    /* intialize character device with file operations object */
    cdev_init(&(pDev->dev), &canonical_char_drv_fops);
    pDev->dev.owner = THIS_MODULE;
    pDev->dev.ops = &canonical_char_drv_fops; 

    /* add charactor device to the system */
    if ((err = cdev_add(&(pDev->dev), devno, 1))) {
        printk(KERN_ERR DEVICE_NODE_NAME": cdev_add() failed\n");
        kfree(pDev->circBuf.buf);
        return err;
    }

    /* init device access semaphore */
    // TODO: switch to MUTEX
    // mutex_init(struct mutex* lock)
    // mutex_lock_interruptible(struct mutex* lock)
    // mutex_lock_killable(struct mutex* lock)
    sema_init(&(pDev->sem), 1);

    return 0;
}


/* driver init */
static int __init 
canonical_char_drv_init (void)
{
    struct device* pDevice = NULL;

    int err = -1;

    printk(KERN_INFO "PRINTK_TAG: canonical_char_drv_init() %s\n", DEVICE_NODE_NAME);

    /* allocate dev to hold major and minor numbers */
    err = alloc_chrdev_region(&dev, 0, 1, DEVICE_NODE_NAME);
    if(err < 0) {
        printk(KERN_ALERT "  PRINTK_TAG: Failed to alloc char dev region.\n");
        goto fail;
    }

    /* extract major and minor from assigned dev */
    major = MAJOR(dev);
    minor = MINOR(dev); 

    /* NOTE: kermel memory allocation
        kmalloc gfp_mask values in /include/linux/gfp.h, the following 3 are normally used by drivers ...
        GFP_KERNEL - normal usage, block and slepp if memory not available
        GFP_ATOMIC - may be used if kmalloc used in interrupt. ret immediately if no pages avail.
        GFP_DMA 

        in_interrupt() macro used to check if in interrupt context, then apprpriate mask may be used.
        kzalloc() to malloc zeroed memory
        krealloc() to resize existing memory
        mod 2 memory is allocated
    */

    /* allocate device object */
    pCanonicalCharDevice = kmalloc(sizeof(struct CanonicalCharDevice), GFP_KERNEL);
    if (pCanonicalCharDevice) {
        printk(KERN_INFO DEVICE_NODE_NAME": allocated CanonicalCharDevice: %d bytes\n", sizeof(struct CanonicalCharDevice));
    } else {
        err = -ENOMEM;
        printk(KERN_ALERT DEVICE_NODE_NAME": Failed to alloc CanonicalCharDev\n");
        goto unregister;
    } 

    /* configure character  device object */
    if ((err = setup_dev(pCanonicalCharDevice))) {
        printk(KERN_ALERT DEVICE_NODE_NAME": setup_dev() failed: %d\n", err);
        goto cleanup;
    } 

    /* sysfs: create linux driver model class 
       /sys/class/CANONICAL_CHAR_DRV_DEVICE_CLASS_NAME
    */
    canonical_char_drv_class = class_create(THIS_MODULE, CANONICAL_CHAR_DRV_DEVICE_CLASS_NAME);
    if (IS_ERR(canonical_char_drv_class)) {
        err = PTR_ERR(canonical_char_drv_class);
        printk(KERN_ALERT DEVICE_NODE_NAME": Failed to create canonical_char_drv_class.\n");
        goto destroy_cdev;
    } 

    /* sysfs: create linux driver mode device with allocated dev major/minor numbers 
       /sys/class/CANONICAL_CHAR_DRV_DEVICE_CLASS_NAME/CANONICAL_CHAR_DRV_DEVICE_FILE_NAME
    */
    pDevice = device_create(canonical_char_drv_class, NULL, dev, "%s", CANONICAL_CHAR_DRV_DEVICE_FILE_NAME);
    if (IS_ERR(pDevice)) {
        err = PTR_ERR(pDevice);
        printk(KERN_ALERT DEVICE_NODE_NAME": Failed to create device.");
        goto destroy_class;
    }

    /* create sysfs endpoint */
    err = device_create_file(pDevice, &dev_attr_mode);
    if(err < 0) {
        printk(KERN_ALERT DEVICE_NODE_NAME": Failed to create attribute val.");
        goto destroy_device;
    }

    atomic_set(&pCanonicalCharDevice->data_ready_to_read, 0);
    atomic_set(&pCanonicalCharDevice->buffer_ready_to_write, 1);
    atomic_set(&pCanonicalCharDevice->num_async_inputs, 0);

    init_waitqueue_head(&pCanonicalCharDevice->wq_read);
    init_waitqueue_head(&pCanonicalCharDevice->wq_write);
    init_waitqueue_head(&pCanonicalCharDevice->wq_async_input_block);
    printk(KERN_INFO DEVICE_NODE_NAME": waitqueue's created\n");

    /* for sysfs device cache */
    dev_set_drvdata(pDevice, pCanonicalCharDevice); 

    create_proc();

    /* success */
    printk(KERN_INFO DEVICE_NODE_NAME": Succeeded in registering character device %s\n", DEVICE_NODE_NAME);
    printk(KERN_INFO DEVICE_NODE_NAME": Major number = %d, Minor number = %d\n", MAJOR (dev), MINOR (dev));
    return 0;

    destroy_device:
    device_destroy(canonical_char_drv_class, dev);

    destroy_class:
    class_destroy(canonical_char_drv_class);

    destroy_cdev:
    cdev_del(pCdev);

    cleanup:
    kfree(pCanonicalCharDevice);

    unregister:
    unregister_chrdev_region (dev, 1);

    fail:
    return err;
}


/* driver exit */
static void __exit 
canonical_char_drv_exit (void) {

    printk (KERN_INFO DEVICE_NODE_NAME": canonical_char_drv_exit() %s\n", DEVICE_NODE_NAME);

    /* remove proc entry from procfs */
    remove_proc();

    if (canonical_char_drv_class) {
        device_destroy(canonical_char_drv_class, MKDEV(major, minor));
        class_destroy(canonical_char_drv_class);
    } 

    if (pCanonicalCharDevice) {
        cdev_del(&(pCanonicalCharDevice->dev));
        kfree(pCanonicalCharDevice->circBuf.buf);
        kfree(pCanonicalCharDevice);
    } 

    unregister_chrdev_region(dev, 1);
}

/* called by OS the module is loaded and unloaded */
module_init (canonical_char_drv_init);
module_exit (canonical_char_drv_exit);

MODULE_AUTHOR ("severs");
MODULE_DESCRIPTION ("canonical_char_drv.c");
/* if not marked as open source, the module is considered tainted */
MODULE_LICENSE ("GPL v2");
