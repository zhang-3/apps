/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <version.h>

#include <led.h>

#define LED_NAME "led"

#include <uwp5661/drivers/src/bt/uki_utlis.h>

u32_t str2hex(char *s)
{
	u32_t val = 0;
	int len;
	int i;
	char c;

	if (strncmp(s, "0x", 2) && strncmp(s, "0X", 2)) {
		BTE("Invalid hex string: %s.", s);
		return 0;
	}

	/* skip 0x */
	s += 2;

	len = strlen(s);
	if (len > 8) {
		BTE("Invalid hex string.");
		return 0;
	}

	for (i = 0; i < len; i++) {
		c = *s;

		/* 0 - 9 */
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else {
			BTE("Invalid hex string.");
			return 0;
		}

		val = val * 16 + c;

		s++;
	}

	return val;
}

u32_t str2data(char *s)
{
	if (strncmp(s, "0x", 2) == 0 || strncmp(s, "0X", 2) == 0)
		return str2hex(s);
	else
		return atoi(s);
}
void read8_cmd_exe(u32_t addr, u32_t len)
{
	int i;

	printk("Read %d bytes from 0x%x:\n", len, addr);

	for (i = 0; i < len; i++, addr++) {
		if (i%16 == 0)
			printk("\n%08x: ", addr);
		printk("%02x ", sys_read8(addr));
	}
	printk("\n");
}

static int cmd_read8(const struct shell *shell, size_t argc, char **argv)
{
	u32_t addr;
	u32_t len;
	int err = shell_cmd_precheck(shell, (argc == 3), NULL, 0);

	if (err) {
		return err;
	}

	addr = (u32_t)str2data(argv[1]);
	len = str2data(argv[2]);

	read8_cmd_exe(addr, len);

	return 0;
}
void read32_cmd_exe(u32_t addr, u32_t len)
{
	int i;

	printk("Read %d words from 0x%x:\n", len, addr);

	for (i = 0; i < len; i++, addr += 4) {
		if (i%4 == 0)
			printk("\n%08x: ", addr);
		printk("%08x ", sys_read32(addr));
	}
	printk("\n");
}

static int cmd_read32(const struct shell *shell, size_t argc, char **argv)
{
	u32_t addr;
	u32_t len;
	int err = shell_cmd_precheck(shell, (argc == 3), NULL, 0);

	if (err) {
		return err;
	}

	addr = (u32_t)str2data(argv[1]);
	len = str2data(argv[2]);

	read32_cmd_exe(addr, len);

	return 0;
}

static int cmd_write(const struct shell *shell, size_t argc, char **argv)
{
	u32_t p;
	u32_t val;
	int err = shell_cmd_precheck(shell, (argc == 3), NULL, 0);

	if (err) {
		return err;
	}

	p = str2data(argv[1]);
	val = str2data(argv[2]);

	sys_write32(val, p);

	shell_fprintf(shell, SHELL_NORMAL,
			"Write data 0x%x to address 0x%x success.",
			val, p);

	return 0;
}


static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL,
			"Zephyr version %s\n", KERNEL_VERSION_STRING);
	return 0;
}


static int cmd_led(const struct shell *shell, size_t argc, char **argv)
{
	static int state;
	struct device *dev;

	dev = device_get_binding(LED_NAME);

	if (state) {
		shell_fprintf(shell, SHELL_NORMAL,
				"LED OFF\n");
		led_off(dev, 1);
		led_off(dev, 2);
		led_off(dev, 3);
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
				"LED ON\n");
		led_on(dev, 1);
		led_on(dev, 2);
		led_on(dev, 3);
	}
	state = !state;

	return 0;
}


SHELL_CREATE_STATIC_SUBCMD_SET(bmesh_cmds) {
	SHELL_CMD(version, NULL, "Void", cmd_version),
	SHELL_CMD(read8, NULL, "Void", cmd_read8),
	SHELL_CMD(read32, NULL, "Void", cmd_read32),
	SHELL_CMD(write, NULL, "Void", cmd_write),
	SHELL_CMD(led, NULL, "Void", cmd_led),

	SHELL_SUBCMD_SET_END
};

static int cmd_bt(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (argc == 1) {
		shell_help_print(shell, NULL, 0);
		/* shell_cmd_precheck returns 1 when help is printed */
		return 1;
	}

	err = shell_cmd_precheck(shell, (argc == 2), NULL, 0);
	if (err) {
		return err;
	}

	shell_fprintf(shell, SHELL_NORMAL, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(bmesh, &bmesh_cmds, "Bluetooth shell commands", cmd_bt);

