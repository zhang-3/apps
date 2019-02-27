/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_SFC_H__
#define __TEST_SFC_H__

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <stdio.h>
#include <version.h>

#include <flash.h>
#include <device.h>

#define CONFIG_DATA_FLASH_BASE_ADDRESS	0x02180000
#define CONFIG_DATA_FLASH_SIZE			0x4000

#define FLASH_TEST_REGION_OFFSET \
	((CONFIG_DATA_FLASH_BASE_ADDRESS - CONFIG_FLASH_BASE_ADDRESS) + 0x5000)
#define FLASH_SECTOR_SIZE        4096

#define SFC_ERR(fmt, ...)   printf("[ERR] "fmt"\n", ##__VA_ARGS__)
#define SFC_WARN(fmt, ...)  printf("[WARN] "fmt"\n", ##__VA_ARGS__)
#define SFC_INFO(fmt, ...)  printf("[INFO] "fmt"\n", ##__VA_ARGS__)

#endif /* __TEST_SFC_H__ */
