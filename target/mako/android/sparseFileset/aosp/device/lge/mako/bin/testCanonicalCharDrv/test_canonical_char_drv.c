#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <hardware/canonical_char_drv.h>
#include <hardware/hardware.h>

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

int main (int argc, char* argv[]) {

    hw_module_t* module;

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
                fprintf(stderr, "get_buffer_size() error: %d", ret);
                goto closeFile;
            }
            if ((capacity = dev->get_buffer_capacity(dev)) < 0) {
                fprintf(stderr, "get_buffer_capacity() error: %d", ret);
                goto closeFile;
            }
            printf("size: %d, capacity: %d\n", size, capacity);

            if (size >= 0 && capacity >= 0) {
                printf("wait_for_buffer_data() read into readBuffer_\n");
                if ((ret = dev->wait_for_buffer_data(dev, readBuffer_, WRITE_CHUNK_SIZE, 5000)) < 0) {
                    fprintf(stderr, "Failed to read buffer: %s\n", strerror(errno));
                } else if (ret == 0) {
                    fprintf(stderr, "timeout waiting for data\n");
                } else {
                    printf("read %d bytes\n", ret);
                }

                // write to buffer
                if ((ret = dev->write_buffer(dev, writeBuffer_, WRITE_CHUNK_SIZE)) < 0) {
                    fprintf(stderr, "Failed to write buffer: %s", strerror(errno));
                    goto closeFile;
                } else {
                    printf("write buffer success\n");
                }
                // get current size
                if ((size = dev->get_buffer_size(dev)) < 0) {
                    fprintf(stderr, "get_buffer_size() error: %d", ret);
                    goto closeFile;
                }
                printf("post write buffer size: %d\n", size);

                if (dev->flush_buffer(dev) == 0) {
                    printf("Flushed buffer. Previously it was consuming %d of %d bytes\n", size, capacity);
                    ret = 0;
                } else {
                    fprintf(stderr, "Failed to flush buffer: %s", strerror(errno));
                    ret = -1;
                }
                // get current size
                if ((size = dev->get_buffer_size(dev)) < 0) {
                    fprintf(stderr, "get_buffer_size() error: %d", ret);
                    goto closeFile;
                }
                printf("post flush buffer size: %d\n", size);


                advance_write_buffer();

                if ((ret = dev->write_buffer(dev, writeBuffer_, WRITE_CHUNK_SIZE)) < 0) {
                    fprintf(stderr, "Failed to write buffer: %s", strerror(errno));
                    goto closeFile;
                } else {
                    printf("write buffer success\n");
                }

                if ((ret = dev->read_buffer(dev, readBuffer_, WRITE_CHUNK_SIZE)) < 0) {
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
            fprintf(stderr, "Failed to open device: %d", ret);
            ret = -3;
        }
    } else {
        fprintf(stderr, "Failed to get module: %s", CANONICAL_CHR_DRV_MODULE_ID);
        ret = -4;
    }

    return ret;
}
