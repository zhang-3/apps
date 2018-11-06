/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME)

#include <zephyr.h>
#include <misc/printk.h>

void main(void)
{
	LOG_INF("   [UNISOC Wi-Fi Repeater]\n");

	while(1) {}
}
