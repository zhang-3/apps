/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log.h"
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <logging/sys_log.h>
#include <stdlib.h>
#include <device.h>
#include <gpio.h>
#include <uart.h>
#include <led.h>

#include <intc_uwp.h>
#include <uwp_hal.h>
#include <pinmux.h>

static int cmd_rand(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	u32_t rand = sys_rand32_get();

	shell_fprintf(shell, SHELL_NORMAL,
			"generate rand: 0x%x\n", rand);
	return 0;
}

static void intc_uwp_soft_irq(int channel, void *data)
{
	if (data) {
		LOG_INF("aon soft irq.");
		uwp_aon_irq_clear_soft();
	} else {
		LOG_INF("soft irq.");
		uwp_irq_clear_soft();
	}
}

static int cmd_intc(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uwp_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void *)0);
	uwp_irq_enable(INT_SOFT);

	uwp_irq_trigger_soft();

	uwp_irq_disable(INT_SOFT);
	uwp_intc_unset_irq_callback(INT_SOFT);

	uwp_aon_intc_set_irq_callback(INT_SOFT, intc_uwp_soft_irq, (void *)1);
	uwp_aon_irq_enable(INT_SOFT);

	uwp_aon_irq_trigger_soft();

	uwp_aon_irq_disable(INT_SOFT);
	uwp_aon_intc_unset_irq_callback(INT_SOFT);

	return 0;
}

static int cmd_sleep(const struct shell *shell, size_t argc, char **argv)
{
	int time;

	int err = shell_cmd_precheck(shell, (argc == 2), NULL, 0);

	if (err) {
		return err;
	}

	time = atoi(argv[1]);
	shell_fprintf(shell, SHELL_NORMAL,
			"sleep %ims start...\n", time);

	k_sleep(time);

	shell_fprintf(shell, SHELL_NORMAL,
			"sleep stop.\n");

	return 0;
}

#define GPIO_PORT0	"UWP_GPIO_P0"
#define UART_2			"UART_2"
static int cmd_uart(const struct shell *shell, size_t argc, char **argv)
{
	struct device *uart;
	char *str = "This is a test message from test command.\r\n";

	uart = device_get_binding(UART_2);
	if (uart == NULL) {
		shell_fprintf(shell, SHELL_ERROR,
				"Can not find device %s.\n", UART_2);
		return -1;
	}

	uart_irq_rx_enable(uart);

	uart_fifo_fill(uart, str, strlen(str));

	shell_fprintf(shell, SHELL_NORMAL,
			"test uart %s finish.\n", UART_2);

	return 0;
}

#define LED_NAME "led"
#define SOTAP_LED (1)
#define STA_LED (3)
static int cmd_led(const struct shell *shell, size_t argc, char **argv)
{
	struct device *led_dev;
	int led_pin, led;
	int on_off;
	int err = shell_cmd_precheck(shell, (argc == 3), NULL, 0);

	if (err) {
		return err;
	}

	led_dev = device_get_binding(LED_NAME);
	if (!led_dev) {
		shell_fprintf(shell, SHELL_ERROR,
				"Can not find device %s.\n", LED_NAME);
		return -1;
	}

	led = atoi(argv[1]);
	on_off = atoi(argv[2]);
	led_pin = CONFIG_LED_PIN1;

	switch (led) {
	case 1:
		led_pin = CONFIG_LED_PIN1; break;
	case 2:
		led_pin = CONFIG_LED_PIN2; break;
	case 3:
		led_pin = CONFIG_LED_PIN3; break;
	default:
		return -1;
	}

	if (on_off) {
		led_on(led_dev, led_pin);
	} else {
		led_off(led_dev, led_pin);
	}

	return 0;
}

static struct gpio_callback button_cb;
int count;
#define BUTTON_PIN 0
#define GPIO0_REG (BASE_AON_PIN+0x10)
#define PMUX_DEV "pinmux_drv"

static void led2_on(struct device *port, struct gpio_callback *cb,
				 u32_t pin)
{
	port = device_get_binding(LED_NAME);
	if (!port) {
		printk("Can not find device %s.\n", LED_NAME);
	}
	pin = 3;
	if (count%2 == 0)
		led_on(port, pin);
	else
		led_off(port, pin);
	count++;
}

static int cmd_button(const struct shell *shell, size_t argc, char **argv)
{
	struct device *gpio, *pmx_dev;

	gpio = device_get_binding(GPIO_PORT0);
	if (!gpio) {
		printk("Can not find device %s.\n", GPIO_PORT0);
		return -1;
	}
	pmx_dev = device_get_binding(PMUX_DEV);
	if (!pmx_dev) {
		printk("Can not find device %s.\n", PMUX_DEV);
		return -1;
	}
	count = 0;
	gpio_pin_configure(gpio, BUTTON_PIN, (GPIO_DIR_IN | GPIO_INT
			| GPIO_INT_EDGE | GPIO_PUD_PULL_UP
			| GPIO_INT_ACTIVE_LOW));
	pinmux_pin_pullup(pmx_dev, P_GPIO0, PMUX_FPU_EN);

	gpio_init_callback(&button_cb, led2_on, BIT(BUTTON_PIN));
	gpio_add_callback(gpio, &button_cb);
	gpio_pin_enable_callback(gpio, BUTTON_PIN);

	return 0;
}

#define FUNC3 3
#define PIN10 10
#define PULLUP 1
int reset_misc(void)
{
	struct device *pm_dev, *p0_dev;

	pm_dev = device_get_binding(PMUX_DEV);
	if (!pm_dev) {
		printk("Can not find device %s.\n", PMUX_DEV);
		return -1;
	}
	p0_dev = device_get_binding(GPIO_PORT0);
	if (!p0_dev) {
		printk("Can not find device %s.\n", GPIO_PORT0);
		return -1;
	}
	/*set INT as GPIO10*/
	pinmux_pin_set(pm_dev, INT, FUNC3);
	printk("set INT as GPIO10\n");
	/*set GPIO10 DIR out and enable GPIO10 */
	gpio_pin_configure(p0_dev, PIN10, GPIO_DIR_OUT);
	/*pull up GPIO10*/
	gpio_pin_write(p0_dev, PIN10, PULLUP);

	return 0;
}
static int cmd_reset(const struct shell *shell, size_t argc, char **argv)
{
	printk("resetting ...\n");
	k_sleep(50);				/* wait 50 ms */

	reset_misc();

	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(sub_test)
{
	SHELL_CMD(rand, NULL, "Generate random number.", cmd_rand),
	SHELL_CMD(intc, NULL, "Triger soft irq.", cmd_intc),
	SHELL_CMD(uart, NULL, "Fill a message to UART.", cmd_uart),
	SHELL_CMD(sleep, NULL, "Sleep for (n) micro seconds.\n"
			"Usage: sleep <ms>", cmd_sleep),
	SHELL_CMD(led, NULL, "Control led to turn on/off.\n"
			"Usage: led <led's number> <on/off>\n"
			"parameters:\n"
			"led's number: 1/2/3.\n"
			"\t   on/off: 1/0", cmd_led),
	SHELL_CMD(button, NULL, "Push button to control led3",
			cmd_button),
	SHELL_CMD(reset, NULL, "Reset the board",
			cmd_reset),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};
/* Creating root (level 0) command "test" */
SHELL_CMD_REGISTER(test, &sub_test, "Function test commands", NULL);
