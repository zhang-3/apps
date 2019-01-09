/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_TEST_BT_UTLIS_H_
#define __BT_TEST_BT_UTLIS_H_

#include <uwp5661/drivers/src/bt/uki_utlis.h>

#define BT_TEST_LOG_ON 0

#define BT_LOG(fmt, ...) 							\
	do { 										\
		if (BT_TEST_LOG_ON == 1) { 	\
			printk(fmt, ##__VA_ARGS__); 		\
		} 										\
	}while(0)

char *u_strtok_r(char *str, const char *delim, char **saveptr);
int bt_sipc_send(unsigned char *data, int len);
#endif
