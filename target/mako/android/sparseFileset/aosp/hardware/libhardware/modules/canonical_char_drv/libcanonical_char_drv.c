// LOG_NDEBUG 0 enables verbose debugging
//#define LOG_NDEBUG 1
#define LOG_NDEBUG 0
#define CANONICAL_CHAR_DRV_FILE "/dev/canonical_char_drv"
#define LOG_TAG "CanonicalCharDrv"

#include <hardware/canonical_char_drv.h>
#include <cutils/log.h>
//// sde - logger.h not here
////#include <cutils/logger.h>
////./system/core/include/log/logger.h
//#include <log/logger.h>   

#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <sys/ioctl.h>

static int ioctl_buffer_read (int mode, int request, int* pArg) {

    int logfd = open(CANONICAL_CHAR_DRV_FILE, mode);
    if (logfd < 0) {
        SLOGE("Failed to open %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
        return -1;
    } else {
        int ret = ioctl(logfd, request, pArg);
        close(logfd);
        return ret;
    }
}

static int ioctl_buffer_write (int mode, int request, int arg) {

    int logfd = open(CANONICAL_CHAR_DRV_FILE, mode);
    if (logfd < 0) {
        SLOGE("Failed to open %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
        return -1;
    } else {
        int ret = ioctl(logfd, request, arg);
        close(logfd);
        return ret;
    }
}

static int ioctl_buffer (int mode, int request) {

    int logfd = open(CANONICAL_CHAR_DRV_FILE, mode);
    if (logfd < 0) {
        SLOGE("Failed to open %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
        return -1;
    } else {
        int ret = ioctl(logfd, request);
        close(logfd);
        return ret;
    }
}

static int flush_buffer (struct canonical_char_drv_device_t* dev) {
    SLOGV("Flushing %s", CANONICAL_CHAR_DRV_FILE);
    return ioctl_buffer(O_WRONLY, FLUSH_BUFFER);
}

static int get_buffer_capacity (struct canonical_char_drv_device_t* dev, int* pCapacity) {
    SLOGV("Getting buffer capacity of %s", CANONICAL_CHAR_DRV_FILE);
    return ioctl_buffer_read(O_RDONLY, GET_BUFFER_CAPACITY, pCapacity);
}

static int get_buffer_size (struct canonical_char_drv_device_t* dev, int* pSize) {
    SLOGV("Getting used buffer size of %s", CANONICAL_CHAR_DRV_FILE);
    return ioctl_buffer_read(O_RDONLY, GET_BUFFER_SIZE, pSize);
}

static int write_buffer (struct canonical_char_drv_device_t* dev, uint8_t* pBuffer, int size) {

    SLOGV("write to %s", CANONICAL_CHAR_DRV_FILE);  

    int logfd = open(CANONICAL_CHAR_DRV_FILE, O_WRONLY);
    if (logfd < 0) {
        SLOGE("Failed to open %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
        return -1;
    } else {
        int ret = write(logfd, pBuffer, size);
        close(logfd);
        return ret;
    }
    return size;
}

static int read_buffer (struct canonical_char_drv_device_t* dev, uint8_t* pBuffer, int size) {

    SLOGV("read %d bytes from %s at address %x", size, CANONICAL_CHAR_DRV_FILE, pBuffer);

    int logfd = open(CANONICAL_CHAR_DRV_FILE, O_RDONLY);
    if (logfd < 0) {
        SLOGE("Failed to open %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
        return -1;
    } else {
        int ret = read(logfd, pBuffer, size);
        close(logfd);
        return ret;
    }
}

static int wait_for_buffer_data (struct canonical_char_drv_device_t* dev, uint8_t* pBuffer, int size, int timeout) {

    SLOGV("Waiting for %d bytes data from %s with timeout: %d", size, CANONICAL_CHAR_DRV_FILE, timeout);

    int ret = -1;
    struct pollfd pfd;
    pfd.fd = dev->fd;
    pfd.events = POLLIN;

    ret = poll(&pfd, 1, timeout);
    if (ret < 0) {
        SLOGE("Failed to poll %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
    } else if (ret == 0) {
        SLOGV("Waiting for buffer data timed out");
    } else {
        //ret = read_buffer(dev, pBuffer, size);
        ret = read(dev->fd, pBuffer, size);
        if (ret < 0) {
            SLOGE("Failed to read %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
        } else if (ret == 0) {
            SLOGE("Unexpected EOF on reading %s", CANONICAL_CHAR_DRV_FILE);
        } else {
            SLOGV("read %d from %s", ret, CANONICAL_CHAR_DRV_FILE);
        }

        //SLOGV("Waiting for buffer data driver returned ... do something here");

        // /* consume all of the available data */
        // unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1] __attribute__((aligned(4)));
        // int new_data_counter = 0;
        // while(1) {
        //   /* we have to read because the file is not seekable */
        //   ret = read(dev->fd, buf, LOGGER_ENTRY_MAX_LEN);              /* 5 */
        //   if (ret < 0) {
        //     SLOGE("Failed to read %s: %s", LOG_FILE, strerror(errno)); /* 1 */
        //     return -1;
        //   } else if (ret == 0) {
        //     SLOGE("Unexpected EOF on reading %s", LOG_FILE);           /* 1 */
        //     return -1;
        //   } else {
        //     new_data_counter += ret;
        //     /* check to see if there is more data to read */
        //     ret = ioctl(dev->fd, LOGGER_GET_NEXT_ENTRY_LEN);           /* 4 */
        //     if (ret < 0) {
        //       SLOGE("Failed to get next entry len on %s: %s",          /* 1 */
        //         LOG_FILE, strerror(errno));
        //       return -1;
        //     } else if (ret == 0) { /* no more data; we are done */
        //       SLOGV("Got %d / %d / %d on %s", new_data_counter,        /* 1 */
        //         get_used_log_size(dev), get_total_log_size(dev), LOG_FILE);
        //       return new_data_counter;
        //     }
        //   }
        // }
    }

    return ret;
}

static int close_canonicalchardrv (struct canonical_char_drv_device_t* dev) {

    SLOGV("Closing %s", CANONICAL_CHAR_DRV_FILE);
    if (dev) {
        close(dev->fd);
        free(dev);
    }

    return 0;
}

static int open_canonicalchardrv (const struct hw_module_t *module, char const *name, struct hw_device_t **device) {

    SLOGV("open_canonicalchardrv()");

    int fd = open(CANONICAL_CHAR_DRV_FILE, O_RDWR);
    if (fd < 0) {
        SLOGE("Failed to open %s: %s", CANONICAL_CHAR_DRV_FILE, strerror(errno));
        return -1;
    } else {
        struct canonical_char_drv_device_t *dev = malloc(sizeof(struct canonical_char_drv_device_t));
        if (!dev) {
            close(dev->fd);
            return -ENOMEM;
        }

        SLOGV("Opened %s", CANONICAL_CHAR_DRV_FILE);

        memset(dev, 0, sizeof(*dev));
        dev->common.tag = HARDWARE_DEVICE_TAG;
        dev->common.version = 0;
        dev->common.module = (struct hw_module_t *)module;
        dev->common.close = (int (*)(struct hw_device_t *)) close_canonicalchardrv;
        dev->fd = fd;
        dev->flush_buffer = flush_buffer;
        dev->get_buffer_capacity = get_buffer_capacity;
        dev->get_buffer_size = get_buffer_size;
        dev->write_buffer = write_buffer;
        dev->read_buffer = read_buffer;
        dev->wait_for_buffer_data = wait_for_buffer_data;
        *device = (struct hw_device_t *)dev;
        return 0;
    }
}

static struct hw_module_methods_t canonicalchardrv_module_methods = {
    .open = open_canonicalchardrv,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = CANONICAL_CHR_DRV_MODULE_ID,
    .name = "canonicalchardrv module",
    .author = "severs",
    .methods = &canonicalchardrv_module_methods,
};
