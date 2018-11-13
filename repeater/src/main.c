/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <misc/printk.h>

extern void blues_init(void);
void main(void)
{
	LOG_INF("   [UNISOC Wi-Fi Repeater]\n");

#ifdef CONFIG_BT_UWP5661
	blues_init();
#endif

	while(1) {}
}
