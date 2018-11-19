/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_TEST_BT_UTLIS_H_
#define __BT_TEST_BT_UTLIS_H_

#define BT_TEST_LOG_ON 0

#define DUMP_BLOCK_SIZE 20

#define BT_LOG(fmt, ...) 							\
	do { 										\
		if (BT_TEST_LOG_ON == 1) { 	\
			printk(fmt, ##__VA_ARGS__); 		\
		} 										\
	}while(0)

void hex_dump_block(char *tag, unsigned char *bin, size_t binsz);
char *u_strtok_r(char *str, const char *delim, char **saveptr);

#define BT_HCIDUMP(tag, bin, binsz) 					\
	do {											\
		if (BT_TEST_LOG_ON == 1) { 		\
			hex_dump_block(tag, bin, binsz); 	\
		}											\
	}while(0)

#endif
