#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <hardware/canonical_char_drv.h>
#include <hardware/hardware.h>
#include <pthread.h>

#define WRITE_CHUNK_SIZE 10
#define READ_CHUNK_SIZE WRITE_CHUNK_SIZE

static uint8_t writeBuffer_[WRITE_CHUNK_SIZE];
static uint8_t readBuffer_[READ_CHUNK_SIZE];


static void advance_write_buffer(void) {

    for (int i = 0, j = writeBuffer_[WRITE_CHUNK_SIZE-1]; i < WRITE_CHUNK_SIZE; ++i, ++j) {
        writeBuffer_[i] = j;
    }
}


static int read_from_fifo(struct canonical_char_drv_device_t* pDevice, uint8_t* pReadBuffer, uint32_t size) {

    int ret;

    if ((ret = pDevice->read_buffer(pDevice, pReadBuffer, size)) < 0) {
        fprintf(stderr, "Failed to read buffer: %s", strerror(errno));
    } else if (ret > 0) {
        printf("read buffer success\n");
        printf("readBuffer_: ");
        for (int i = 0; i < READ_CHUNK_SIZE; ++i) {
            printf("%d ,", pReadBuffer[i]);
        }
        printf("\n");
    } else {
        printf("read buffer timeout\n");
    }

    return ret;
}


static int g_thread_retval;
void* async_input_blocking_thread (void* arg) {

    int timeout = 60000;
    struct canonical_char_drv_device_t *dev = (struct canonical_char_drv_device_t *) arg;

    fprintf(stderr, "block_for_async_input() with: %d timeout\n", timeout);
    if ((g_thread_retval = dev->block_for_async_input(dev, timeout)) < 0) {
        fprintf(stderr, "block_for_async_input() error: %d\n", g_thread_retval);
    } else {
        fprintf(stderr, "block_for_async_input() success with size: %d\n", g_thread_retval);
    }

    return &g_thread_retval;
}


int main (int argc, char* argv[]) {

    hw_module_t* module;
    pthread_t thread;
    void* thread_retval;
    int timeout;
    int status;

    for (int i = 0; i < WRITE_CHUNK_SIZE; ++i) {
        writeBuffer_[i] = i;
    }
    
    int ret = hw_get_module(CANONICAL_CHR_DRV_MODULE_ID, (hw_module_t const**)&module);
    if (ret == 0) {

        struct canonical_char_drv_device_t *dev;
        printf("hw_get_module() success\n");
        ret = module->methods->open(module, 0, (struct hw_device_t **) &dev);
        if (ret == 0) {

            printf("open() success\n");

            int err;

            int size, capacity;
            if ((size = dev->get_buffer_size(dev)) < 0) {
                fprintf(stderr, "get_buffer_size() error: %d\n", ret);
                goto closeFile;
            }
            if ((capacity = dev->get_buffer_capacity(dev)) < 0) {
                fprintf(stderr, "get_buffer_capacity() error: %d\n", ret);
                goto closeFile;
            }
            printf("size: %d, capacity: %d\n", size, capacity);

            if (size >= 0 && capacity >= 0) {

                /* issue a few blocking async input calls */
                int timeout_msec = 1000;
                printf("block_for_async_input() with %d msec timeout\n", timeout_msec );
                if ((size = dev->block_for_async_input(dev, timeout_msec)) < 0) {
                    fprintf(stderr, "block_for_async_input() error: %d\n", ret);
                    goto closeFile;
                }
                timeout_msec = 5000;
                printf("block_for_async_input() with %d msec timeout\n", timeout_msec );
                if ((size = dev->block_for_async_input(dev, timeout_msec)) < 0) {
                    fprintf(stderr, "block_for_async_input() error: %d\n", ret);
                    goto closeFile;
                }

                /* make a blocking async input call from another thread */
                status = pthread_create(&thread, NULL, async_input_blocking_thread, dev);
                if (status != 0) {
                    printf("pthread_create() failure");
                    goto closeFile;
                }

                printf("wait_for_buffer_data() read into readBuffer_\n");
                if ((ret = dev->wait_for_buffer_data(dev, readBuffer_, WRITE_CHUNK_SIZE, 5000)) < 0) {
                    fprintf(stderr, "Failed to read buffer: %s\n", strerror(errno));
                    goto closeFile;
                } else if (ret == 0) {
                    fprintf(stderr, "timeout waiting for data\n");
                } else {
                    printf("read %d bytes\n", ret);
                }

                /* write to buffer */
                if ((ret = dev->write_buffer(dev, writeBuffer_, WRITE_CHUNK_SIZE)) < 0) {
                    fprintf(stderr, "Failed to write buffer: %s\n", strerror(errno));
                    goto closeFile;
                } else {
                    printf("write buffer success\n");
                }
                /* get current size */
                if ((size = dev->get_buffer_size(dev)) < 0) {
                    fprintf(stderr, "get_buffer_size() error: %d\n", ret);
                    goto closeFile;
                }
                printf("post write buffer size: %d\n", size);
                /* join async blocking thread */
                status = pthread_join(thread, &thread_retval);
                if (status != 0) {
                    printf("pthread_join() failure\n");
                    goto closeFile;
                } else {
                    printf("pthread_join() success, thread_retval: %d\n", *((int*)thread_retval));
                }
                /* flush buffer */
                if (dev->flush_buffer(dev) == 0) {
                    printf("Flushed buffer. Previously it was consuming %d of %d bytes\n", size, capacity);
                    ret = 0;
                } else {
                    fprintf(stderr, "Failed to flush buffer: %s", strerror(errno));
                    ret = -1;
                    goto closeFile;
                }
                /* get current size */
                if ((size = dev->get_buffer_size(dev)) < 0) {
                    fprintf(stderr, "get_buffer_size() error: %d\n", ret);
                    goto closeFile;
                }
                printf("post flush buffer size: %d\n", size);

                advance_write_buffer();

                if ((ret = dev->write_buffer(dev, writeBuffer_, WRITE_CHUNK_SIZE)) < 0) {
                    fprintf(stderr, "Failed to write buffer: %s\n", strerror(errno));
                    goto closeFile;
                } else {
                    printf("write buffer success\n");
                }

                if ((ret = dev->read_buffer(dev, readBuffer_, READ_CHUNK_SIZE)) < 0) {
                    fprintf(stderr, "Failed to read buffer: %s\n", strerror(errno));
                    goto closeFile;
                } else if (ret > 0) {
                    printf("read buffer success\n");
                    printf("readBuffer_: ");
                    for (int i = 0; i < READ_CHUNK_SIZE; ++i) {
                        printf("%d ,", readBuffer_[i]);
                    }
                    printf("\n");
                } else {
                    printf("read buffer timeout\n");
                }

            } else {
                printf("size < 0 OR capacity < 0\n");
            }

            closeFile:
            printf("close file\n");
            dev->common.close((struct hw_device_t *)dev);

        } else {
            fprintf(stderr, "Failed to open device: %d\n", ret);
            ret = -3;
        }
    } else {
        fprintf(stderr, "Failed to get module: %s\n", CANONICAL_CHR_DRV_MODULE_ID);
        ret = -4;
    }

    return ret;
}
