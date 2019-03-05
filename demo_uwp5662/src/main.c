/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log.h"
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr.h>
#include <misc/printk.h>

void main(void)
{
	LOG_INF("   [UNISOC UWP5662 DEMO]\n");

	while(1) {}
}
