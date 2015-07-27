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

// https://www.kernel.org/doc/Documentation/ioctl/ioctl-number.txt
// http://www.makelinux.net/ldd3/chp-6-sect-1
    // include/asm-generic/ioctl.h, include/asm-generic/ioctls.h, 
    // Documentation/ioctl/ioctl-number.txt, Documentation/ioctl/ioctl-decoding.txt
// http://tuxthink.blogspot.com/2011/01/creating-ioctl-command.html
#define __CANONICAL_CHAR_DRV_IOC	0xBF

#define GET_BUFFER_CAPACITY     _IOR(__CANONICAL_CHAR_DRV_IOC, 1, int) /* read capacity of buffer */
#define GET_BUFFER_SIZE	        _IOR(__CANONICAL_CHAR_DRV_IOC, 2, int) /* read used buffer size */
#define FLUSH_BUFFER            _IO(__CANONICAL_CHAR_DRV_IOC, 3) /* flush buffer */

#define __CANONICAL_CHAR_DRV_IOC_MAX_NMBR 4


 #endif // CANONICAL_CHAR_DRV_H