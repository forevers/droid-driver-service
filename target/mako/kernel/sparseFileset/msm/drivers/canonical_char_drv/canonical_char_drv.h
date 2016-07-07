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

#ifndef CANONICAL_CHAR_DRV_H
#define CANONICAL_CHAR_DRV_H

/* NOTE: debugging techniques

- printk() usage
    KERN_EMERG "<0>"   - system is unusable
    KERN_ALERT "<1>"   - action must be taken immediately
    KERN_CRIT "<2>"    - critical conditions
    KERN_ERR "<3>"     - error conditions
    KERN_WARNING "<4>" - warning conditions
    KERN_NOTICE "<5>"  - normal but significant condition
    KERN_INFO "<6>"    - informational
    KERN_DEBUG "<7>"   - debug-level messages

- dmesg user space utility to interrogate printk's
    $ adb shell dmesg | grep canonical
    $ adb shell dmesg > dmesgEraseme

- logcat
    $ adb shell logcat -d *:V > logcatEraseme

- kernel message buffer user space access
    $ adb shell cat /proc/kmsg

- procfs
    $ adb shell echo '5' > /proc/canonical_char_drv
    $ adb shell cat /proc/canonical_char_drv

- sysfs
    $ adb shell echo '10' > /sys/class/canonical_char_drv/canonical_char_drv/mode
    $ adb shell cat /sys/class/canonical_char_drv/canonical_char_drv/mode
*/

/* NOTES: IOCTL Numbers
https://www.kernel.org/doc/Documentation/ioctl/ioctl-number.txt

The first argument to _IO, _IOW, _IOR, or _IOWR is an identifying letter
or number.  Because of the large number of drivers, many drivers share a 
partial letter with other drivers.

The second argument to _IO, _IOW, _IOR, or _IOWR is a sequence number
to distinguish ioctls from each other.  The third argument to _IOW,
_IOR, or _IOWR is the type of the data going into the kernel or coming
out of the kernel (e.g.  'int' or 'struct foo').  
*/

/* NOTES: ioctl command decoding
    include/asm-generic/ioctl.h, include/asm-generic/ioctls.h, 
    Documentation/ioctl/ioctl-number.txt, Documentation/ioctl/ioctl-decoding.txt

    bits 31:30 are the command direction
    00 - no parameters: uses _IO macro
    10 - read: _IOR macro
    01 - write: _IOW mcaro
    11 - read/write: _IOWR macro

    bits 29:16 are the size of arguments
    
    bits 15:0 are the command
    15-8   ascii character supposedly unique to each driver
    7-0    function id
*/

// http://tuxthink.blogspot.com/2011/01/creating-ioctl-command.html
#define __CANONICAL_CHAR_DRV_IOC 0xBF

#define GET_BUFFER_CAPACITY     _IOR(__CANONICAL_CHAR_DRV_IOC, 1, int)  /* read capacity of buffer */
#define GET_BUFFER_SIZE	        _IOR(__CANONICAL_CHAR_DRV_IOC, 2, int)  /* read used buffer size */
#define FLUSH_BUFFER            _IO(__CANONICAL_CHAR_DRV_IOC, 3)        /* flush buffer */
#define ASNYC_INPUT_BLOCK       _IOWR(__CANONICAL_CHAR_DRV_IOC, 4, int) /* block for async input (with timeout) */

#define __CANONICAL_CHAR_DRV_IOC_MAX_NMBR 4

#endif // CANONICAL_CHAR_DRV_H