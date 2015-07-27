#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <hardware/canonical_char_drv.h>
#include <hardware/hardware.h>

#define WRITE_CHUNK_SIZE 10
#define READ_CHUNK_SIZE 3

int main (int argc, char* argv[]) {

    hw_module_t* module;

    uint8_t writeBuffer[WRITE_CHUNK_SIZE];
    uint8_t readBuffer[READ_CHUNK_SIZE];

    for (int i = 0; i < WRITE_CHUNK_SIZE; ++i) {
        writeBuffer[i] = i;
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
            if ((ret = dev->get_buffer_size(dev, &size))) {
                fprintf(stderr, "get_buffer_size() error: %d", ret);
                goto closeFile;
            }
            if ((ret = dev->get_buffer_capacity(dev, &capacity))) {
                fprintf(stderr, "get_buffer_capacity() error: %d", ret);
                goto closeFile;
            }
            printf("size: %d, capacity: %d\n", size, capacity);

            if (size >= 0 && capacity >= 0) {

                if ((ret = dev->write_buffer(dev, writeBuffer, WRITE_CHUNK_SIZE)) < 0) {
                    fprintf(stderr, "Failed to write buffer: %s", strerror(errno));
                    goto closeFile;
                } else {
                    printf("write buffer success\n");
                }

                if (dev->flush_buffer(dev) == 0) {
                    printf("Flushed buffer. Previously it was consuming %d of %d bytes\n", size, capacity);
                    ret = 0;
                } else {
                    fprintf(stderr, "Failed to flush buffer: %s", strerror(errno));
                    ret = -1;
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
