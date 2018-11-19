/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#include "mesh.h"

#include <uwp5661/drivers/src/bt/uki_utlis.h>

void main(void)
{
	BTI("\n   [UNISOC Mesh]");

	mesh_init();

	while(1) {}
}
