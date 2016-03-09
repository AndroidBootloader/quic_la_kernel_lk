/*
 * Copyright (c) 2016 Gurjant Kalsi <me@gurjantkalsi.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <lib/bootimage.h>
#include <app/moot/fsboot.h>
#include <app/moot/stubs.h>
#include <compiler.h>
#include <stdio.h>
#include <lib/fs.h>
#include <err.h>
#include <lib/bio.h>

#define MAX_FPATH_LEN 64

// Attempt to boot from the filesystem.
void attempt_fs_boot(void)
{
    char *mount_path, *device_name;
    bootimage_t *bi;

    status_t retcode = moot_mount_default_fs(&mount_path, &device_name);
    if (retcode != NO_ERROR) {
        return;
    }

    char fpath[MAX_FPATH_LEN];


    filehandle *handle;
    retcode = fs_open_file(fpath, &handle);
    if (retcode != NO_ERROR) {
        goto finish;
    }

    // Fill in the length of the bootimage
    struct file_stat stat;
    retcode = fs_stat_file(handle, &stat);
    if (retcode != NO_ERROR) {
        goto finish;
    }

    // Get the address of the Bootimage.
    // TODO: At some point we might have a flash device/controller that doesn't
    // support linear mode. In that case we'll have to come up with a mechanism
    // that can verify a bootimg without memory mapping the file.
    unsigned char *address = 0;
    retcode = fs_file_ioctl(handle, FS_IOCTL_GET_FILE_ADDR, &address);
    fs_close_file(handle);

    if (retcode != NO_ERROR) {
        goto finish;
    }

    bdev_t *secondary_flash = bio_open(device_name);
    if (!secondary_flash) {
        goto finish;
    }

    unsigned char *unused = 0;
    retcode = bio_ioctl(secondary_flash, BIO_IOCTL_GET_MEM_MAP, &unused);
    bio_close(secondary_flash);

    if (retcode != NO_ERROR) {
        goto finish;
    }

    retcode = bootimage_open(address, stat.size, &bi);
    if (retcode != NO_ERROR) {
        goto finish;
    }

    size_t imglen;
    const void *imgptr;
    retcode = bootimage_get_file_section(bi, TYPE_LK, &imgptr, &imglen);
    if (retcode != NO_ERROR) {
        goto finish;
    }

    // Flash the new image.
    bdev_t *system_flash = bio_open(moot_system_info.system_flash_name);
    if (!system_flash) {
        goto finish;
    }

    ssize_t n_bytes_erased =
        bio_erase(system_flash, moot_system_info.system_offset, imglen);
    if (n_bytes_erased < (ssize_t)imglen) {
        bio_close(system_flash);
        goto finish;
    }

    ssize_t written =
        bio_write(system_flash, imgptr, moot_system_info.system_offset, imglen);
    bio_close(system_flash);

    if (written < (ssize_t)imglen) {
        goto finish;
    }

finish:
    fs_unmount(mount_path);
}

