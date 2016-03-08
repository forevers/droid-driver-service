#ifndef _CANONICAL_CHAR_DRV_H
#define _CANONICAL_CHAR_DRV_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <hardware/hardware.h>

__BEGIN_DECLS

#define CANONICAL_CHAR_DRV_HEADER_VERSION          1
#define CANONICAL_CHAR_DRV_MODULE_API_VERSION_0_1  HARDWARE_MODULE_API_VERSION(0, 1)
#define CANONICAL_CHAR_DRV_DEVICE_API_VERSION_0_1  HARDWARE_DEVICE_API_VERSION_2(0, 1, CANONICAL_CHAR_DRV_HEADER_VERSION)

#define CANONICAL_CHR_DRV_MODULE_ID "canonical_char_drv"

#define __CANONICAL_CHAR_DRV_IOC 0xBF

// copy of driver level ioctl command enumerations
// TODO share header with driver
#define GET_BUFFER_CAPACITY     _IOR(__CANONICAL_CHAR_DRV_IOC, 1, int)  /* read capacity of buffer */
#define GET_BUFFER_SIZE         _IOR(__CANONICAL_CHAR_DRV_IOC, 2, int)  /* read used buffer size */
#define FLUSH_BUFFER            _IO(__CANONICAL_CHAR_DRV_IOC, 3)        /* flush buffer */
#define ASNYC_INPUT_BLOCK       _IOWR(__CANONICAL_CHAR_DRV_IOC, 4, int) /* block for async input (with timeout) */

struct canonical_char_drv_device_t {
  struct hw_device_t common;

  int fd;

  /*
   * Flush the buffer
   *
   * Returns: 0 on success, error code on failure
   */
  int (*flush_buffer)(struct canonical_char_drv_device_t* dev);

  /*
   * Get the total buffer capacity
   *
   * Returns: total buffer capacity, < 0 on failure
   */
  int (*get_buffer_capacity)(struct canonical_char_drv_device_t* dev);

  /*
   * Get the used buffer size
   *
   * Returns: used buffer size, < 0 on failure
   */
  int (*get_buffer_size)(struct canonical_char_drv_device_t* dev);

  /*
   * Write to the buffer
   *
   * Returns: number of bytes written, < 0 on failure
   */
  int (*write_buffer)(struct canonical_char_drv_device_t* dev, uint8_t* pBuffer, int size);

   /*
   * Read from the buffer
   *
   * Returns: number of bytes read, < 0 on failure
   */
  int (*read_buffer)(struct canonical_char_drv_device_t* dev, uint8_t* pBuffer, int size);

  /*
   * Wait until more buffer data becomes available or until timeout expires
   * timeout: the max wait time (in ms). The value of -1 means wait forever
   * Returns: < 0 or error, 0 on timeout, the amount of data read (in bytes)
   */
  int (*wait_for_buffer_data)(struct canonical_char_drv_device_t* dev, uint8_t* pBuffer, int size, int timeout);

    /*
   * Block for asnyc input or until timeout expires
   * timeout: the max wait time (in ms). The value of -1 means wait forever
   * Returns: < 0 or error, 0 on timeout, the amount of new data written to buffer (in bytes)
   */
  int (*block_for_async_input)(struct canonical_char_drv_device_t* dev, int timeout);
};

__END_DECLS

#endif // #define _CANONICAL_CHAR_DRV_H
