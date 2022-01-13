/*
 * Copyright 2021 CEVA, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License and
 * any applicable agreements you may have with CEVA, Inc.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bno_dfu_hal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "sh2_err.h"

#define RESET_DELAY_US (10000)

// Wait this long before assuming bootloader is ready
#define DFU_BOOT_DELAY_US (50000)

// Wait this long before assuming SH2 is ready
#define SH2_BOOT_DELAY_US (150000)

// Augmented HAL structure with BNO DFU on Linux specific fields.
struct bno_dfu_hal_s {
    sh2_Hal_t hal_fns; // must be first so (sh2_Hal_t *) can be cast as (bno_dfu_hal_t *)

    bool is_open;
    int fd;
    const char* device_filename;
};
typedef struct bno_dfu_hal_s bno_dfu_hal_t;

// Set RESETN to state.
static void setResetN(int fd, bool state) {
    // RESETN connected to DTR
    int resetSignal = TIOCM_DTR;
    int status = 0;
    ioctl(fd, TIOCMGET, &status);

    if (state) {
        // Clear the signal (logic level inverted, this outputs high.)
        status &= ~resetSignal;
    } else {
        // Set the signal (logic level inverted, this outputs low.)
        status |= resetSignal;
    }
    ioctl(fd, TIOCMSET, &status);
}

// Set BOOTN to state.
static void setBootN(int fd, bool state) {
    // BOOTN connected to RTS
    int bootnSignal = TIOCM_RTS;

    int status;
    ioctl(fd, TIOCMGET, &status);

    if (state) {
        // Clear the signal (logic level inverted, this outputs high.)
        status &= ~bootnSignal;
    } else {
        // Set the signal (logic level inverted, this outputs low.)
        status |= bootnSignal;
    }
    ioctl(fd, TIOCMSET, &status);
}

static void uart_errno_printf(const char* s, ...) {
    va_list vl;
    va_start(vl, s); // initializes vl with arguments after s. accessed by int=va_arg(vl,int)
    vfprintf(stderr, s, vl);
    fprintf(stderr, " errno = %d (%s)\n", errno, strerror(errno));
    fflush(stderr);
    va_end(vl);
}

static uint32_t time32_now_us() {
    static uint64_t initialTimeUs = 0;
    uint64_t t_us = 0;

    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    t_us = ((uint32_t)tp.tv_sec * 1000000) + ((uint32_t)tp.tv_nsec / 1000.0);

    if (initialTimeUs == 0) {
        initialTimeUs = t_us;
    }
    t_us -= initialTimeUs;

    return (uint32_t)t_us;
}

static void delay_us(uint32_t t) {
    uint32_t now = time32_now_us();
    uint32_t start = now;
    while ((now - start) <= t) {
        now = time32_now_us();
    }
}

static int dfu_hal_open(sh2_Hal_t* self) {
    bno_dfu_hal_t* pHal = (bno_dfu_hal_t*)self;
    struct termios tty;
    speed_t baud = B115200;

    // Return error if already open
    if (pHal->is_open) {
        return SH2_ERR;
    }

    // Mark as open
    pHal->is_open = true;

    // Open device file
    if ((pHal->fd = open(pHal->device_filename, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
        uart_errno_printf("uart_connect: OPEN %s:", pHal->device_filename);
        pHal->is_open = false;
        return SH2_ERR_IO;
    }

    // Get attributes
    if (tcgetattr(pHal->fd, &tty) < 0) {
        fprintf(stderr, "unable to read port attributes");
        pHal->is_open = false;
        close(pHal->fd);
        return SH2_ERR_IO;
    }
    tty.c_iflag &=
            ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF); //| IGNPAR
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;
    tty.c_cflag &= ~(CSIZE | PARENB);
    tty.c_cflag |= CS8 | CREAD | CLOCAL;

    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    // Set speed
    cfsetispeed(&tty, baud);
    cfsetospeed(&tty, baud);

    // VMIN and VTIME so data is returned ASAP
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    // Set attributes
    if (tcsetattr(pHal->fd, TCSANOW, &tty) < 0) {
        fprintf(stderr, "unable to set port attributes");
        pHal->is_open = false;
        close(pHal->fd);
        return SH2_ERR_IO;
    }

    fsync(pHal->fd);
    tcflush(pHal->fd, TCIOFLUSH);

    // Reset into bootloader
    setResetN(pHal->fd, false); // Assert reset
    setBootN(pHal->fd, false);  // Assert BOOTN

    // Delay for RESET_DELAY_US to ensure reset takes effect
    delay_us(RESET_DELAY_US);

    setResetN(pHal->fd, true); // Deassert reset with BOOTN asserted

    // Wait until we know bootloader is up
    delay_us(DFU_BOOT_DELAY_US);

    return SH2_OK;
}

static void dfu_hal_close(sh2_Hal_t* self) {
    bno_dfu_hal_t* pHal = (bno_dfu_hal_t*)self;

    // Reset into normal SHTP mode
    setResetN(pHal->fd, false); // Assert reset
    setBootN(pHal->fd, true);   // De-assert BOOTN

    // Delay for RESET_DELAY_US to ensure reset takes effect
    delay_us(RESET_DELAY_US);

    setResetN(pHal->fd, true); // De-assert reset with BOOTN de-asserted

    // Delay to ensure fully booted before allowing system to continue.
    delay_us(SH2_BOOT_DELAY_US);

    // Mark as not open
    pHal->is_open = false;

    // Close descriptor
    close(pHal->fd);
}

static int dfu_hal_read(sh2_Hal_t* self, uint8_t* pBuffer, unsigned len, uint32_t* t_us) {
    bno_dfu_hal_t* pHal = (bno_dfu_hal_t*)self;
    int status = 0;
    unsigned len_read = 0;
    uint8_t* pCursor = pBuffer;

    // Read data into pBuffer until we have the full requested length.
    while (len_read < len) {
        status = read(pHal->fd, pCursor, len - len_read);

        if (status > 0) {
            len_read += status;
            pCursor += status;
        }
        if ((status < 0) && (errno != EAGAIN)) {
            break;
        }
    }

    // Grab timestamp
    *t_us = time32_now_us();

    // Return num bytes read
    return len_read;
}

static int dfu_hal_write(sh2_Hal_t* self, uint8_t* pBuffer, unsigned len) {
    bno_dfu_hal_t* pHal = (bno_dfu_hal_t*)self;
    uint8_t* pNext = pBuffer;
    int wrote = 0;

    // Write data from pBuffer until the full length is written.
    while (wrote < len) {
        int status = write(pHal->fd, pNext, len - wrote);
        if (status > 0) {
            wrote += status;
            pNext += status;
        }
        if ((status < 0) && (errno != EAGAIN)) {
            break;
        }
    }

    // Return num bytes written
    return wrote;
}

static uint32_t dfu_hal_getTimeUs(sh2_Hal_t* self) {
    return time32_now_us();
}

// dfu_hal instance data
static bno_dfu_hal_t dfu_hal = {
        .hal_fns =
                {
                        .open = dfu_hal_open,
                        .close = dfu_hal_close,
                        .read = dfu_hal_read,
                        .write = dfu_hal_write,
                        .getTimeUs = dfu_hal_getTimeUs,
                },

        .is_open = false,
        .fd = 0,
        .device_filename = "",
};

sh2_Hal_t* bno_dfu_hal_init(const char* device_filename) {
    // Save reference to device file name, etc.
    dfu_hal.device_filename = device_filename;

    dfu_hal.is_open = false;
    dfu_hal.fd = 0;

    dfu_hal.hal_fns.open = dfu_hal_open;
    dfu_hal.hal_fns.close = dfu_hal_close;
    dfu_hal.hal_fns.read = dfu_hal_read;
    dfu_hal.hal_fns.write = dfu_hal_write;
    dfu_hal.hal_fns.getTimeUs = dfu_hal_getTimeUs;

    // give caller the list of access functions.
    return &(dfu_hal.hal_fns);
}
