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
	int led_pin;
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

	led_pin = atoi(argv[1]);
	on_off = atoi(argv[2]);

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
#define PIN_FPU_EN BIT(8)

static void led3_on(struct device *port, struct gpio_callback *cb,
				 u32_t pin)
{
	port = device_get_binding(LED_NAME);
	pin = 3;
	if (count%2 == 0)
		led_on(port, pin);
	else
		led_off(port, pin);
	count++;
}

static int cmd_button(const struct shell *shell, size_t argc, char **argv)
{
	struct device *gpio;

	gpio = device_get_binding(GPIO_PORT0);
	count = 0;
	gpio_pin_configure(gpio, BUTTON_PIN, (GPIO_DIR_IN | GPIO_INT
			| GPIO_INT_EDGE | GPIO_PUD_PULL_UP
			| GPIO_INT_ACTIVE_LOW));
	unsigned int *q = (unsigned int *)GPIO0_REG;
	*q |= PIN_FPU_EN;

	gpio_init_callback(&button_cb, led3_on, BIT(BUTTON_PIN));
	gpio_add_callback(gpio, &button_cb);
	gpio_pin_enable_callback(gpio, BUTTON_PIN);
	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(sub_test)
{
	SHELL_CMD(rand, NULL, "Generate random number.", cmd_rand),
	SHELL_CMD(intc, NULL, "Triger soft irq.", cmd_intc),
	SHELL_CMD(uart, NULL, "Fill a message to UART.", cmd_uart),
	SHELL_CMD(sleep, NULL, "Sleep for (n) micro seconds.\n"
			"Usage: sleep <ms>", cmd_sleep),
	SHELL_CMD(led, NULL, "Control led to turn on/off.",
			cmd_led),
	SHELL_CMD(button, NULL, "Push button to control led3",
			cmd_button),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};
/* Creating root (level 0) command "test" */
SHELL_CMD_REGISTER(test, &sub_test, "Function test commands", NULL);
