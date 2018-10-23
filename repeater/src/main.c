/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#include <gpio.h>
#include <watchdog.h>
#include <device.h>
#include <console.h>
#include <flash.h>
#include <shell/shell.h>
#include <string.h>
#include <misc/util.h>
#include <fs.h>
#include <uart.h>
#include <stdlib.h>
#include <logging/sys_log.h>

#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <version.h>
#include <logging/log.h>
#include <random/rand32.h>

#include "uwp_hal.h"
#if 0
LOG_MODULE_REGISTER(app);

SHELL_UART_DEFINE(shell_transport_uart);
SHELL_DEFINE(uart_shell, "uart:~$ ", &shell_transport_uart, 10);

extern void foo(void);

void timer_expired_handler(struct k_timer *timer)
{
	LOG_INF("Timer expired.");

	/* Call another module to present logging from multiple sources. */
	//foo();
}

K_TIMER_DEFINE(log_timer, timer_expired_handler, NULL);

static int cmd_log_test_start(const struct shell *shell, size_t argc,
		char **argv, u32_t period)
{
	if (!shell_cmd_precheck(shell, argc == 1, NULL, 0)) {
		return 0;
	}

	k_timer_start(&log_timer, period, period);
	shell_fprintf(shell, SHELL_NORMAL, "Log test started\r\n");
	return 0;
}

static int cmd_log_test_start_demo(const struct shell *shell, size_t argc,
		char **argv)
{
	cmd_log_test_start(shell, argc, argv, 200);
	return 0;
}

static int cmd_log_test_start_flood(const struct shell *shell, size_t argc,
		char **argv)
{
	cmd_log_test_start(shell, argc, argv, 10);
	return 0;
}

static int cmd_log_test_stop(const struct shell *shell, size_t argc,
		char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!shell_cmd_precheck(shell, argc == 1, NULL, 0)) {
		return 0;
	}

	k_timer_stop(&log_timer);
	shell_fprintf(shell, SHELL_NORMAL, "Log test stopped\r\n");

	return 0;
}
SHELL_CREATE_STATIC_SUBCMD_SET(sub_log_test_start)
{
	/* Alphabetically sorted. */
	SHELL_CMD(demo, NULL,
			"Start log timer which generates log message every 200ms.",
			cmd_log_test_start_demo),
		SHELL_CMD(flood, NULL,
				"Start log timer which generates log message every 10ms.",
				cmd_log_test_start_flood),
		SHELL_SUBCMD_SET_END /* Array terminated. */
};
SHELL_CREATE_STATIC_SUBCMD_SET(sub_log_test)
{
	/* Alphabetically sorted. */
	SHELL_CMD(start, &sub_log_test_start, "Start log test", NULL),
		SHELL_CMD(stop, NULL, "Stop log test.", cmd_log_test_stop),
		SHELL_SUBCMD_SET_END /* Array terminated. */
};

SHELL_CMD_REGISTER(log_test, &sub_log_test, "Log test", NULL);

static int cmd_demo_ping(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL, "pong\r\n");

	return 0;
}

static int cmd_demo_params(const struct shell *shell, size_t argc, char **argv)
{
	int cnt;

	shell_fprintf(shell, SHELL_NORMAL, "argc = %d\r\n", argc);
	for (cnt = 0; cnt < argc; cnt++) {
		shell_fprintf(shell, SHELL_NORMAL,
				"  argv[%d] = %s\r\n", cnt, argv[cnt]);
	}
	return 0;
}

static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL,
			"Zephyr version %s\r\n", KERNEL_VERSION_STRING);
	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(sub_demo)
{
	/* Alphabetically sorted. */
	SHELL_CMD(params, NULL, "Print params command.", cmd_demo_params),
		SHELL_CMD(ping, NULL, "Ping command.", cmd_demo_ping),
		SHELL_SUBCMD_SET_END /* Array terminated. */
};

SHELL_CMD_REGISTER(demo, &sub_demo, "Demo commands", NULL);

SHELL_CMD_REGISTER(version, NULL, "Show kernel version", cmd_version);


void main(void)
{
	(void)shell_init(&uart_shell, NULL, true, true, LOG_LEVEL_INF);
}
#endif

#if 0
#include <fs.h>
#include <ff.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include "uwp_hal.h"
#include "bluetooth/blues.h"

#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN	"UNISOC"

#define UART_2			"UART_2"

typedef void (*uwp_intc_callback_t) (int channel, void *user);
extern void uwp_intc_set_irq_callback(int channel,
		uwp_intc_callback_t cb, void *arg);
extern void uwp_intc_unset_irq_callback(int channel);
extern void uwp_aon_intc_set_irq_callback(int channel,
		uwp_intc_callback_t cb, void *arg);
extern void uwp_aon_intc_unset_irq_callback(int channel);

void device_list_get(struct device **device_list, int *device_count);

static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb,
		u32_t mgmt_event,
		struct net_if *iface)
{
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];

		if (iface->config.ip.ipv4->unicast[i].addr_type !=
				NET_ADDR_DHCP) {
			continue;
		}

		SYS_LOG_WRN("Your address: %s",
				net_addr_ntop(AF_INET,
					&iface->config.ip.ipv4->unicast[i].address.in_addr,
					buf, sizeof(buf)));
		SYS_LOG_WRN("Lease time: %u seconds",
				iface->config.dhcpv4.lease_time);
		SYS_LOG_WRN("Subnet: %s",
				net_addr_ntop(AF_INET,
					&iface->config.ip.ipv4->netmask,
					buf, sizeof(buf)));
		SYS_LOG_WRN("Router: %s",
				net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw,
					buf, sizeof(buf)));
	}
}

int dhcp_client(int argc, char **argv)
{
	struct device *dev;
	struct net_if *iface;

	SYS_LOG_WRN("Run dhcpv4 client");

	net_mgmt_init_event_callback(&mgmt_cb, handler,
			NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	dev = device_get_binding(CONFIG_WIFI_UWP_STA_NAME);
	if (!dev) {
		SYS_LOG_ERR( "failed to get device %s!\n", CONFIG_WIFI_UWP_STA_NAME);
		return -1;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		SYS_LOG_ERR("failed to get iface %s!\n", CONFIG_WIFI_UWP_STA_NAME);
		return -1;
	}

	net_dhcpv4_start(iface);

	return 0;
}

void print_devices(void)
{
	int i;
	int count = 0;

	static struct device *device_list;

	device_list_get(&device_list, &count);

	SYS_LOG_INF("device list(%d):", count);
	for(i = 0; i < count; i++) {
		SYS_LOG_INF(" %s ", device_list[i].config->name);
	}
}

void uart_test(void)
{
	struct device *uart;
	char *str = "This is a test message from test command.\r\n";

	uart = device_get_binding(UART_2);
	if(uart == NULL) {
		SYS_LOG_ERR("Can not find device %s.", UART_2);
		return;
	}

	uart_irq_rx_enable(uart);

	uart_fifo_fill(uart, str, strlen(str));

	SYS_LOG_INF("test uart %s finish.", UART_2);
}

static void intc_uwp_soft_irq(int channel, void *data)
{
	if(data) {
		SYS_LOG_INF("aon soft irq.");
		uwp_aon_irq_clear_soft();
	} else {
		SYS_LOG_INF("soft irq.");
		uwp_irq_clear_soft();
	}
}

static int test_cmd(int argc, char *argv[])
{
	if(argc < 2) return -EINVAL;

	if(!strcmp(argv[1], "flash")) {
		//spi_flash_test();
	}else if(!strcmp(argv[1], "uart")) {
		uart_test();
	}else if(!strcmp(argv[1], "intc")) {
		uwp_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void*)0);
		uwp_irq_enable(INT_SOFT);

		uwp_irq_trigger_soft();

		uwp_irq_disable(INT_SOFT);
		uwp_intc_unset_irq_callback(INT_SOFT);
	}else if(!strcmp(argv[1], "aon_intc")) {
		uwp_aon_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void*)1);
		uwp_aon_irq_enable(INT_SOFT);

		uwp_aon_irq_trigger_soft();

		uwp_aon_irq_disable(INT_SOFT);
		uwp_aon_intc_unset_irq_callback(INT_SOFT);
	}else {
		return -EINVAL;
	}

	return 0;
}

static int info_cmd(int argc, char *argv[])
{
	if(argc != 1) return -EINVAL;

	SYS_LOG_INF("System Info: \n");

	print_devices();

	return 0;
}

static int sleep_cmd(int argc, char *argv[])
{
	int time;

	if(argc != 2) return -EINVAL;

	time = atoi(argv[1]);
	SYS_LOG_INF("sleep %ims start...", time);

	k_sleep(time);

	SYS_LOG_INF("sleep stop.");

	return 0;
}

static int dev_cmd(int argc, char **argv)
{
	struct device *devices;
	int count;
	int i;

	device_list_get(&devices, &count);

	SYS_LOG_WRN("System device lists:");
	for (i = 0; i < count; i++, devices++) {
		if(strcmp(devices->config->name, "") == 0)
			continue;
		SYS_LOG_WRN("%d: %s.", i, devices->config->name);
	}

	return 0;
}

u32_t str2hex(char *s)
{
	u32_t val = 0;
	int len;
	int i;
	char c;

	if (strncmp(s, "0x", 2) && strncmp(s, "0X", 2)) {
		SYS_LOG_ERR("Invalid hex string: %s.", s);
		return 0;
	}

	/* skip 0x */
	s += 2;

	len = strlen(s);
	if(len > 8) {
		SYS_LOG_ERR("Invalid hex string.");
		return 0;
	}

	for (i = 0; i < len; i++) {
		c = *s;

		/* 0 - 9 */
		if(c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'F') 
			c = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else {
			SYS_LOG_ERR("Invalid hex string.");
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
	SYS_LOG_WRN("Read %d bytes from 0x%x:", len, addr);

	for(i = 0; i < len; i++, addr++) {
		if(i%16 == 0) printk("\n%08x: ", addr );
		printk("%02x ", sys_read8(addr));
	}
	printk("\n");
}

static int read8_cmd(int argc, char **argv)
{
	u32_t addr;
	u32_t len;
	if(argc != 3) return -EINVAL;

	addr = (u32_t)str2data(argv[1]);
	len = str2data(argv[2]);

	read8_cmd_exe(addr, len);

	return 0;
}
void read32_cmd_exe(u32_t addr, u32_t len)
{
	int i;
	SYS_LOG_WRN("Read data from 0x%x:", addr);

	for(i = 0; i < len; i++, addr += 4) {
		if(i%4 == 0) printk("\n%08x: ", addr );
		printk("%08x ", sys_read32(addr));
	}
	printk("\n");
}

static int read32_cmd(int argc, char **argv)
{
	u32_t addr;
	u32_t len;
	if(argc != 3) return -EINVAL;

	addr = (u32_t)str2data(argv[1]);
	len = str2data(argv[2]);

	read32_cmd_exe(addr, len);

	return 0;
}

static int write_cmd(int argc, char **argv)
{
	u32_t p;
	u32_t val;
	if(argc != 3) return -EINVAL;

	p = str2data(argv[1]);
	val = str2data(argv[2]);

	sys_write32(val, p);

	SYS_LOG_INF("Write data 0x%x to address 0x%x success.",
			val, p);

	return 0;
}

extern int dhcp_client(int argc, char **argv);

static const struct shell_cmd zephyr_cmds[] = {
	{ "info", info_cmd, "" },
	{ "sleep", sleep_cmd, "time(ms)" },
	{ "test", test_cmd, "<flash|uart|intc|aon_intc>" },
	{ "device", dev_cmd, "" },
	{ "read8", read8_cmd, "adress len" },
	{ "read32", read32_cmd, "adress len" },
	{ "write32", write_cmd, "adress value" },
	{ "dhcp_client", dhcp_client, "" },
	{ NULL, NULL }
	//{ "iwnpi", iwnpi_cmd, "adress value" },
	//	{ "flash", flash_cmd, "mount|read|write address value" },
};

#define FATFS_MNTP "/NAND:"
/* FatFs work area */
static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

void mount_file_system(void)
{
	int ret;
	ret = fs_mount(&fatfs_mnt);
	if (ret < 0) {
		SYS_LOG_ERR("Error mounting fs [%d]\n", ret);
	}
}

static void __ramfunc ramfunc_test(void)
{
	printk("RUN IN RAM: %p\n", ramfunc_test);
}
#endif

#define GPIO_PORT0      "UWP_GPIO_P0"
#define GPIO0       0
#define GPIO2       2

struct gpio_callback cb;

static void gpio_callback(struct device *dev,
		struct gpio_callback *gpio_cb, u32_t pins)
{
	SYS_LOG_INF("main gpio int.\n");
}

void gpio_init(void)
{
	struct device *gpio;

	gpio = device_get_binding(GPIO_PORT0);
	if(gpio == NULL) {
		SYS_LOG_ERR("Can not find device %s.", GPIO_PORT0);
		return;
	}

	gpio_pin_configure(gpio, GPIO2, GPIO_DIR_OUT
			| GPIO_PUD_PULL_DOWN);

	gpio_pin_write(gpio, GPIO2, 1);
#if 1
	gpio_pin_disable_callback(gpio, GPIO0);

	gpio_pin_configure(gpio, GPIO0,
			GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL | \
			GPIO_INT_ACTIVE_LOW | GPIO_PUD_PULL_UP);

	gpio_init_callback(&cb, gpio_callback, BIT(GPIO0));
	gpio_add_callback(gpio, &cb);

	gpio_pin_enable_callback(gpio, GPIO0);
#endif
}

#define UWP_WDG     CONFIG_WDT_UWP_DEVICE_NAME
#define ON      1
#define OFF     0

void led_switch(struct device *dev)

{
	static int sw = ON;

	sw = (sw == ON) ? OFF : ON;
	gpio_pin_write(dev, GPIO2, sw);
}

/* WDT Requires a callback, there is no interrupt enable / disable. */
void wdt_example_cb(struct device *dev, int channel_id)
{
	struct device *gpio;

	gpio = device_get_binding(GPIO_PORT0);
	if(gpio == NULL) {
		SYS_LOG_ERR("Can not find device %s.", GPIO_PORT0);
		return;
	}

	led_switch(gpio);
}

void wdg_init(void)
{
	int ret;
	struct wdt_timeout_cfg wdt_cfg;
	struct wdt_config wr_cfg;
	struct wdt_config cfg;
	struct device *wdt_dev;

	wdt_cfg.callback = wdt_example_cb;
	wdt_cfg.flags = WDT_FLAG_RESET_SOC;
	wdt_cfg.window.min = 0;
	wdt_cfg.window.max = 4000;

	wr_cfg.timeout = 4000;
	wr_cfg.mode = WDT_MODE_INTERRUPT_RESET;
	wr_cfg.interrupt_fn = wdt_example_cb;

	wdt_dev = device_get_binding(UWP_WDG);
	if(wdt_dev == NULL) {
		SYS_LOG_ERR("Can not find device %s.", UWP_WDG);
		return;
	}
	wdt_set_config(wdt_dev, &wr_cfg);
	wdt_enable(wdt_dev);


	wdt_get_config(wdt_dev, &cfg);
}

static void intc_uwp_soft_irq(int channel, void *data)
{
	if(data) {
		SYS_LOG_INF("aon soft irq.");
		uwp_aon_irq_clear_soft();
	} else {
		SYS_LOG_INF("soft irq.");
		uwp_irq_clear_soft();
	}
}

static void intc_test(void)
{
	uwp_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void*)0);
	uwp_irq_enable(INT_SOFT);

	uwp_irq_trigger_soft();

	uwp_irq_disable(INT_SOFT);
	uwp_intc_unset_irq_callback(INT_SOFT);

	uwp_aon_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void*)1);
	uwp_aon_irq_enable(INT_SOFT);

	uwp_aon_irq_trigger_soft();

	uwp_aon_irq_disable(INT_SOFT);
	uwp_aon_intc_unset_irq_callback(INT_SOFT);
}

void main(void)
{
	SYS_LOG_WRN("Unisoc Wi-Fi Repeater.");

	u32_t rand = sys_rand32_get();

	SYS_LOG_WRN("rand: %d", rand);

	intc_test();
	gpio_init();
	wdg_init();

	//SHELL_REGISTER("zephyr", zephyr_cmds);

	//ramfunc_test();

	//mount_file_system();

	//blues_init();

	while(1) {}
}
