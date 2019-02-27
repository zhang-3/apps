#ifndef __WIFI_EUT_BBAT_H__
#define __WIFI_EUT_BBAT_H__
#include <stdio.h>
#include "engpc.h"

#define BBAT_LOG(fmt,...) printf(fmt, ##__VA_ARGS__)
u32_t sprd_get_reg_addr(u32_t pin);
u32_t sprd_get_gpio(u32_t pin);
u32_t sprd_get_corres_pin(u32_t pin);
u32_t sprd_get_portpin(u32_t gpio_name);
struct device *sprd_get_port(u32_t val);
u32_t sprd_get_pin(u32_t val);
int bbat_test(char *req, char *rsp);
int sprd_corres_pin_read(u32_t pin);
char *int_to_str(u32_t list[20], int count);

#endif
