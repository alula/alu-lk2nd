// SPDX-License-Identifier: GPL-2.0+
// © 2019 Mis012

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libfdt.h>

#include <lib/fs.h>
#include <platform/iomap.h>
#include <platform.h>
#include <decompress.h>

#include "bootimg.h"

int boot_linux_from_ext2(char *kernel_path, char *ramdisk_path, char *devtree_path, char *cmdline) {
	void *kernel_addr = VA((addr_t)(ABOOT_FORCE_KERNEL64_ADDR));
	void *ramdisk_addr = VA((addr_t)(ABOOT_FORCE_RAMDISK_ADDR));
	void *tags_addr = VA((addr_t)(ABOOT_FORCE_TAGS_ADDR));

	unsigned char *kernel_raw = NULL;
	off_t kernel_raw_size = 0;
	off_t ramdisk_size = 0;
	off_t devtree_size = 0;
    uint32_t dtb_offset = 0;
	unsigned int out_len = 0;
    void *dtb;

	kernel_raw_size = fs_get_file_size(kernel_path);
	if(!kernel_raw_size) {
		printf("fs_get_file_size (%s) failed\n", kernel_path);
		return -1;
	}
	kernel_raw = ramdisk_addr; //right where the biggest possible decompressed kernel would end; sure to be out of the way

    printf("Loading kernel: %s\n", kernel_path);
	if(fs_load_file(kernel_path, kernel_raw, kernel_raw_size) < 0) {
		printf("failed loading %s at %p\n", kernel_path, kernel_addr);
		return -1;
	}

	if(is_gzip_package(kernel_raw, kernel_raw_size)) {
        printf("Decompressing Linux...\n");
		if(decompress(kernel_raw, kernel_raw_size, kernel_addr, ABOOT_FORCE_RAMDISK_ADDR - ABOOT_FORCE_KERNEL64_ADDR, 
            &dtb_offset, &out_len)) {
			printf("kernel decompression failed\n");
			return -1;
		}
	} else {
		memmove(kernel_addr, kernel_raw, kernel_raw_size);
	}

    dtb = dev_tree_appended((void*)kernel_raw, (uint32_t) kernel_raw_size, dtb_offset, tags_addr);
    if (!dtb) {
        printf("No appended device tree found.\n");
        devtree_size = fs_get_file_size(devtree_path);
        if (!devtree_size) {
            printf("fs_get_file_size (%s) failed\n", devtree_path);
            return -1;
        }

        printf("Loading device tree: %s\n", devtree_path);
        if(fs_load_file(devtree_path, tags_addr, devtree_size) < 0) {
            printf("failed loading %s at %p\n", devtree_path, tags_addr);
            return -1;
        }
    }

	kernel_raw = NULL; //get rid of dangerous reference to ramdisk_addr before it can do harm

	ramdisk_size = fs_get_file_size(ramdisk_path);
	if (!ramdisk_size) {
		printf("fs_get_file_size (%s) failed\n", ramdisk_path);
		return -1;
	}

    printf("Loading initial ramdisk: %s\n", ramdisk_path);
	if(fs_load_file(ramdisk_path, ramdisk_addr, ramdisk_size) < 0) {
		printf("failed loading %s at %p\n", ramdisk_path, ramdisk_addr);
		return -1;
	}

	boot_linux(kernel_addr, tags_addr, (const char *)cmdline, board_machtype(), ramdisk_addr, (uint32_t) ramdisk_size);
	
	return -1; //something went wrong
}
