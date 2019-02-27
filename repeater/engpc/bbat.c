#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gpio.h>
#include <device.h>
#include <zephyr/types.h>
#include <uwp_hal.h>

#include "bbat.h"

#define PIN_UP 1
#define PIN_DOWN 0

#define GPIO_PORT0	DT_GPIO_PO_UWP_NAME
#define GPIO_PORT1	"UWP_GPIO_P1"
#define GPIO_PORT2	"UWP_GPIO_P2"

#define PIN38		38

#define pintest_print(fmt, ...) \
	printf(fmt, ##__VA_ARGS__)
#define PINTEST_SET(regaddr, set) \
	sci_write32(regaddr, (*(volatile u32_t *)regaddr)|set)
#define PINTEST_FUNC_CLEAR(regaddr) \
	sci_write32(regaddr, (*(volatile u32_t *)regaddr)& \
		(~(PIN_FPD_EN | PIN_FPU_EN)))

static int pin_list[] = {
	50, 35, 46, 47, 40, 42, 41, 43, 29,
};

static void delay(u32_t cycles)
{
	volatile u32_t i, j;

	for (i = 0; i < cycles ;++i) {
		for(j = 0; j < cycles; ++j) {
			__asm__ __volatile__ ("nop");
		}
	}
}

u32_t sprd_get_reg_addr(u32_t pin)
{
	u32_t reg_addr = 0;
	switch(pin)
	{
		case 50: reg_addr = PIN_INT_REG;break;
		case 24: reg_addr = PIN_GPIO1_REG;break;
		case 35: reg_addr = PIN_RFCTL7_REG;break;
		case 46: reg_addr = PIN_U1TXD_REG;break;
		case 47: reg_addr = PIN_U1RXD_REG;break;
		case 40: reg_addr = PIN_U0TXD_REG;break;
		case 42: reg_addr = PIN_U0RTS_REG;break;
		case 41: reg_addr = PIN_U0RXD_REG;break;
		case 43: reg_addr = PIN_U0CTS_REG;break;
		case 27: reg_addr = PIN_IISLRCK_REG;break;
		case 28: reg_addr = PIN_IISDI_REG;break;
		case 29: reg_addr = PIN_IISDO_REG;break;
		case 30: reg_addr = PIN_IISCLK_REG;break;
		case 18: reg_addr = PIN_SD_CLK_REG;break;
		case 17: reg_addr = PIN_SD_CMD_REG;break;
		case 21: reg_addr = PIN_SD_D0_REG;break;
		case 22: reg_addr = PIN_SD_D1_REG;break;
		case 20: reg_addr = PIN_SD_D2_REG;break;
		case 19: reg_addr = PIN_SD_D3_REG;break;
	}
	return reg_addr;
}
 u32_t sprd_get_gpio(u32_t pin)
{
	u32_t gpio_name = 0;
	switch(pin)
	{
		case 50: gpio_name = 10;break;
		case 24: gpio_name = 1;break;
		case 35: gpio_name = 39;break;
		case 46: gpio_name = 21;break;
		case 47: gpio_name = 22;break;
		case 40: gpio_name = 43;break;
		case 42: gpio_name = 45;break;
		case 41: gpio_name = 44;break;
		case 43: gpio_name = 46;break;
		case 27: gpio_name = 6;break;
		case 28: gpio_name = 7;break;
		case 29: gpio_name = 4;break;
		case 30: gpio_name = 5;break;
		case 18: gpio_name = 19;break;
		case 17: gpio_name = 20;break;
		case 21: gpio_name = 18;break;
		case 22: gpio_name = 17;break;
		case 20: gpio_name = 16;break;
		case 19: gpio_name = 15;break;
	}
	return gpio_name;
}
u32_t sprd_get_corres_pin(u32_t pin)
{
	u32_t corres_pin = 0;
	switch(pin)
	{
		case 38: corres_pin = 27;break;
		case 50: corres_pin = 24;break;
		case 24: corres_pin = 50;break;
		case 35: corres_pin = 28;break;
		case 46: corres_pin = 18;break;
		case 47: corres_pin = 17;break;
		case 40: corres_pin = 21;break;
		case 42: corres_pin = 22;break;
		case 41: corres_pin = 20;break;
		case 43: corres_pin = 19;break;
		case 27: corres_pin = 28;break;
		case 28: corres_pin = 27;break;
		case 29: corres_pin = 30;break;
		case 30: corres_pin = 29;break;
		case 18: corres_pin = 35;break;
		case 17: corres_pin = 46;break;
		case 21: corres_pin = 22;break;
		case 22: corres_pin = 21;break;
		case 20: corres_pin = 19;break;
		case 19: corres_pin = 20;break;
	}
	return corres_pin;
}

struct device *sprd_get_port(u32_t val)
{
	struct device *port;
	u32_t tmp = val / 16;
	if(tmp == 0)
		port = device_get_binding(GPIO_PORT0);
	else if (tmp == 1)
		port = device_get_binding(GPIO_PORT1);
	else
		port = device_get_binding(GPIO_PORT2);
	printk("port:%p\n",port);
	return port;
}

u32_t sprd_get_pin(u32_t val)
{
	u32_t pin = val % 16;
	printk("pin:%p\n", (void *)pin);
	return pin;
}

int sprd_pull_pin(u32_t pin_value, u32_t level)
{
	struct device *port;
	u32_t pin = 0, gpio_name = 0;
	u32_t regaddr = 0;

	regaddr = sprd_get_reg_addr(pin_value);
	if(pin != 24)
	{
		PINTEST_SET(regaddr, PIN_FUNC_3);
	}

	gpio_name = sprd_get_gpio(pin_value);
	port = sprd_get_port(gpio_name);
	pin = sprd_get_pin(gpio_name);

	gpio_pin_configure(port, pin, GPIO_DIR_OUT);
	gpio_pin_write(port, pin, level);

	return 0;
}

int sprd_corres_pin_read(u32_t pin_value)
{
	struct device *port;
	u32_t pin = 0, gpio_name = 0;
	u32_t result;
	u32_t correspin = 0;
	u32_t corres_regaddr = 0;

	correspin = sprd_get_corres_pin(pin_value);
	corres_regaddr = sprd_get_reg_addr(correspin);
	if(correspin != 24)
	{
		PINTEST_SET(corres_regaddr, PIN_FUNC_3);
	}
	gpio_name = sprd_get_gpio(correspin);
	port = sprd_get_port(gpio_name);
	pin = sprd_get_pin(gpio_name);
	gpio_pin_configure(port, pin, GPIO_DIR_IN);
	gpio_pin_read(port, pin, &result);

	return result;
}

char *itostr(int num, int count)
{
	char pin_num[5] = { };
	static char str[10] = { };
	char *pstr = str;
	int k = 0;
	int high, low;
	char hi, lo, ch;
	if(num/10) {
		high = num / 10;
		low = num % 10;
		hi = high + '0';
		lo = low + '0';
		pin_num[k] = hi;
		k++;
		pin_num[k] = lo;
	}
	else {
		ch = num + '0';
		pin_num[k] = ch;
	}
	strcat(pstr, pin_num);
	return pstr;
}

char *int_to_str(u32_t list[20], int count)
{
	static char str[100] = {0};
	char *pstr = str;
	int i , high, low;
	char hi, lo, ch;

	for(i=0;i < count;i++) {
		int k = 0;
		int num = list[i];
		char pin_num[5] = {'0'};
		if(num/10) {
			high = num / 10;
			low = num % 10;
			hi = high + '0';
			lo = low + '0';
			pin_num[k] = hi;
			k++;
			pin_num[k] = lo;
		}
		else {
			ch = num + '0';
			pin_num[k] = ch;
		}
			strcat(pstr, pin_num);
			strcat(pstr, " ");
	}
	return pstr;
}

int sprd_get_value(u32_t pinvalue)
{
	struct device *port;
	u32_t pin = 0, gpio_name = 0;
	u32_t result;

	gpio_name = sprd_get_gpio(pinvalue);
	port = sprd_get_port(gpio_name);
	pin = sprd_get_pin(gpio_name);
	int regaddr = sprd_get_reg_addr(pinvalue);
	if(pinvalue != 24)
	{
		PINTEST_SET(regaddr, PIN_FUNC_3);
	}
	gpio_pin_read(port, pin, &result);

	return result;
}

int bbat_test(char *req, char *rsp)
{
	u32_t powerlevel, pinvalue;
	u32_t result, res;
	int i;
	u32_t wrong_pin[20] = {};
	char log[2048] = {};
	char *str_log = log;
	u32_t cnt = 0,regaddr;

	for (i = 0; i < 9; i++) {
		pinvalue = pin_list[i];
		powerlevel = PIN_UP;

		strcat(str_log, "p:");
		strcat(str_log, itostr(pinvalue, 1));
		strcat(str_log, " ");
		strcat(str_log, itostr(powerlevel, 1));
		strcat(str_log, "\n");

		res = sprd_get_value(pinvalue);
		sprd_pull_pin(pinvalue, powerlevel);
		regaddr = sprd_get_reg_addr(pinvalue);
		pintest_print("******p %d res: %d\n", pinvalue, res);
		u32_t correspin = sprd_get_corres_pin(pinvalue);
		if(correspin == 38) {
			strcat(str_log, "p38 manual test\n");
		}
		result = sprd_corres_pin_read(pinvalue);
		pintest_print("******corres_p %d res: %d\n", correspin, result);

		strcat(str_log, itostr(correspin, 1));
		strcat(str_log, " ");
		strcat(str_log, itostr(result, 1));
		strcat(str_log, "\n");
		if (result == powerlevel) {
			powerlevel = PIN_DOWN;
			strcat(str_log, "p:");
			strcat(str_log, itostr(pinvalue, 1));
			strcat(str_log, " ");
			strcat(str_log, itostr(powerlevel, 1));
			strcat(str_log, "\n");
			sprd_pull_pin(pinvalue, powerlevel);
			pintest_print("******p %d res: %d\n", pinvalue, res);
			result = sprd_corres_pin_read(pinvalue);
			strcat(str_log, itostr(correspin, 1));
			strcat(str_log, " ");
			strcat(str_log, itostr(result, 1));
			strcat(str_log, "\n");
			pintest_print("******corres_p %d res: %d\n", correspin, result);
			if(result == powerlevel)
				;
			else {
				wrong_pin[cnt] = pinvalue;
				cnt++;
				wrong_pin[cnt] = sprd_get_corres_pin(pinvalue);
				cnt++;
			}
		}
		else {
			wrong_pin[cnt] = pinvalue;
			cnt++;
			wrong_pin[cnt] = sprd_get_corres_pin(pinvalue);
			cnt++;
		}
		delay(50);
		PINTEST_FUNC_CLEAR(regaddr);
		sprd_pull_pin(pinvalue, res);
	}

	result = sprd_corres_pin_read(PIN38);
	if(result)
		;
	else {
		wrong_pin[cnt] = PIN38;
		cnt++;
		wrong_pin[cnt] = sprd_get_corres_pin(PIN38);
		cnt++;
	}
	strcat(str_log, "read:");
	strcat(str_log, itostr(sprd_get_corres_pin(PIN38), 1));
	strcat(str_log, " ");
	strcat(str_log, itostr(result, 1));
	strcat(str_log, "\n");

	if (cnt == 0) {
		strcpy(rsp, str_log);
		strcat(rsp, "+SPWCNTEST:OK");
	}
	else {
		strcpy(rsp, str_log);
		strcat(rsp, "res: ");
		strcat(rsp, int_to_str(wrong_pin, cnt));
		strcat(rsp, "\n");
	}

	delay(50);

	PINTEST_FUNC_CLEAR(regaddr);

	return 0;
}
