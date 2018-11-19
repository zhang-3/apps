#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <logging/sys_log.h>
#include <stdbool.h>

#include <led.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>
#include <tinycrypt/hmac_prng.h>

#include <uwp5661/drivers/src/bt/uki_utlis.h>
#include <uwp5661/drivers/src/bt/uki_config.h>

#include "mesh.h"
#include "light.h"

#define GPIO_PORT0		"UWP_GPIO_P0"
#define LED_NAME "led"
#define BANNER_TIMEOUT K_MSEC(800)

static struct light_dev_t dev;
extern struct local_net_t local_net;

struct onoff_state leds[] = {
	{ .pin = LED1_GPIO_PIN,
	  .state = LED_STATE_OFF
	},
	{ .pin = LED2_GPIO_PIN,
	  .state = LED_STATE_OFF
	},
	{ .pin = LED3_GPIO_PIN,
	  .state = LED_STATE_OFF
	},
};

void led_all_turn_on(void)
{
	int i;
	for (i = 0; i < sizeof(leds)/sizeof(struct onoff_state); i++) {
		if (leds[i].state == LED_STATE_OFF) {
			led_on(dev.led, leds[i].pin);
			leds[i].state = LED_STATE_ON;
		}
	}
	dev.state++;
}

void led_all_turn_off(void)
{
	int i;
	for (i = 0; i < sizeof(leds)/sizeof(struct onoff_state); i++) {
		if (leds[i].state == LED_STATE_ON) {
			led_off(dev.led, leds[i].pin);
			leds[i].state = LED_STATE_OFF;
		}
	}
	dev.state--;
}


void button_pressed_worker(struct k_work *unused)
{
	if (dev.banner_running) {
		k_timer_stop(&dev.banner);
		dev.banner_running = 0;
	}

	if (dev.state) {
		led_all_turn_off();
	} else {
		led_all_turn_on();
	}

	NET_BUF_SIMPLE_DEFINE(msg, 2 + 4 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = local_net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = 0xffff,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	BTI("SET LED %d", dev.state);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_SET);
	net_buf_simple_add_u8(&msg, dev.state);
	net_buf_simple_add_u8(&msg, dev.tid++);
	net_buf_simple_add_u8(&msg, 0);
	net_buf_simple_add_u8(&msg, 0);
	if (bt_mesh_model_send(get_root_modules() + 4, &ctx, &msg, NULL, NULL)) {
		BTE("Unable to send On Off Status response");
	}

}

static void banner_timer_task(struct k_timer *work)
{
	static s8_t index = 0;

	if (!index) {
		index = 3;
		led_on(dev.led, leds[index - 1].pin); 
		leds[index - 1].state = LED_STATE_ON;
		BTV("LED: %d ON", index);
		return;
	}
 	
 	if (leds[index - 1].state == LED_STATE_ON) {
		led_off(dev.led, leds[index - 1].pin); 
		leds[index - 1].state = LED_STATE_OFF;
		BTV("LED: %d OFF", index);
	}

	index--;
	if (!index)
		index = 3;

	led_on(dev.led, leds[index - 1].pin);
	leds[index - 1].state = LED_STATE_ON;
	BTV("LED: %d ON", index);

	if (dev.banner_running++ > (sizeof(leds)/sizeof(struct onoff_state))) {
		k_timer_stop(&dev.banner);
		led_all_turn_off();
		dev.banner_running = 0;
	}
}

static void button_pressed(struct device *dev_t, struct gpio_callback *cb,
			   u32_t pins)
{
	BTI("%s", __func__);
	u32_t value = 0;
	gpio_pin_read(dev.button, BUTTON1_GPIO_PIN0, &value);
	if (value == -1)
		k_work_submit(&dev.button_work);
}

void light_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

void light_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u8_t state = net_buf_simple_pull_u8(buf);
	u8_t tran_id = net_buf_simple_pull_u8(buf);
	u8_t tran_time = net_buf_simple_pull_u8(buf);
	u8_t delay = net_buf_simple_pull_u8(buf);

	BTI("%s %d -> %d", __func__, dev.state, state);
	BTI("tran_id: %d, tran_time: 0x%02x, delay, %d", tran_id, tran_time, delay);

	if (dev.banner_running) {
		k_timer_stop(&dev.banner);
		dev.banner_running = 0;
	}

	if (state == dev.state) {
		return;
	}

	if (state) {
		led_all_turn_on();
	} else {
		led_all_turn_off();
	}
}


void light_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	BTD("%s", __func__);
}



const struct bt_mesh_model_op light_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, light_onoff_get },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET, 4, light_onoff_set },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, light_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

static void lightness_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx,
                             struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_linear_get(struct bt_mesh_model *model,
                                 struct bt_mesh_msg_ctx *ctx,
                                 struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_linear_set(struct bt_mesh_model *model,
                                 struct bt_mesh_msg_ctx *ctx,
                                 struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_linear_set_unack(struct bt_mesh_model *model,
                                       struct bt_mesh_msg_ctx *ctx,
                                       struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_linear_status(struct bt_mesh_model *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_last_get(struct bt_mesh_model *model,
                               struct bt_mesh_msg_ctx *ctx,
                               struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_last_status(struct bt_mesh_model *model,
                                  struct bt_mesh_msg_ctx *ctx,
                                  struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_default_get(struct bt_mesh_model *model,
                                  struct bt_mesh_msg_ctx *ctx,
                                  struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_default_status(struct bt_mesh_model *model,
                                     struct bt_mesh_msg_ctx *ctx,
                                     struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_range_get(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_range_status(struct bt_mesh_model *model,
                                   struct bt_mesh_msg_ctx *ctx,
                                   struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}


const struct bt_mesh_model_op lightness_srv_op[] = {
    { BT_MESH_MODEL_OP_2(0x82, 0x4b), 2, lightness_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x4c), 2, lightness_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x4d), 2, lightness_set_unack },
    { BT_MESH_MODEL_OP_2(0x82, 0x4e), 2, lightness_status },
    { BT_MESH_MODEL_OP_2(0x82, 0x4f), 2, lightness_linear_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x50), 2, lightness_linear_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x51), 2, lightness_linear_set_unack },
    { BT_MESH_MODEL_OP_2(0x82, 0x52), 2, lightness_linear_status },
    { BT_MESH_MODEL_OP_2(0x82, 0x53), 2, lightness_last_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x54), 2, lightness_last_status },
    { BT_MESH_MODEL_OP_2(0x82, 0x55), 2, lightness_default_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x56), 2, lightness_default_status },
    { BT_MESH_MODEL_OP_2(0x82, 0x57), 2, lightness_range_get },
    { BT_MESH_MODEL_OP_2(0x82, 0x58), 2, lightness_range_status },
    BT_MESH_MODEL_OP_END,
};



static void lightness_setup_default_set(struct bt_mesh_model *model,
                                        struct bt_mesh_msg_ctx *ctx,
                                        struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_setup_default_set_unack(struct bt_mesh_model *model,
                                              struct bt_mesh_msg_ctx *ctx,
                                              struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_setup_range_set(struct bt_mesh_model *model,
                                      struct bt_mesh_msg_ctx *ctx,
                                      struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}

static void lightness_setup_range_set_unack(struct bt_mesh_model *model,
                                            struct bt_mesh_msg_ctx *ctx,
                                            struct net_buf_simple *buf)
{
	BTI("%s", __func__);
}


const struct bt_mesh_model_op lightness_setup_srv_op[] = {
    { BT_MESH_MODEL_OP_2(0x82, 0x59), 2, lightness_setup_default_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x5a), 2, lightness_setup_default_set_unack },
    { BT_MESH_MODEL_OP_2(0x82, 0x5b), 2, lightness_setup_range_set },
    { BT_MESH_MODEL_OP_2(0x82, 0x5c), 2, lightness_setup_range_set_unack },
    BT_MESH_MODEL_OP_END,
};

void gpio_init(void)
{
	dev.button = device_get_binding(GPIO_PORT0);
	dev.led = device_get_binding(LED_NAME);
	gpio_pin_configure(dev.button, BUTTON1_GPIO_PIN0,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
			   | GPIO_PUD_PULL_UP
			   | GPIO_INT_ACTIVE_LOW));
	unsigned int *p = (unsigned int*)(0x40840010);
	*p = *p | (1 << 8);

	gpio_init_callback(&dev.button_cb, button_pressed,
			   BIT(BUTTON1_GPIO_PIN0));
	gpio_add_callback(dev.button, &dev.button_cb);
	gpio_pin_enable_callback(dev.button, BUTTON1_GPIO_PIN0);

}


void light_startup(void)
{
	k_timer_start(&dev.banner, 0, 800);
}

void light_init(void)
{
	BTI("%s", __func__);

	memset(&dev, 0 , sizeof(struct light_dev_t));
	gpio_init();
	k_work_init(&dev.button_work, button_pressed_worker);
	k_timer_init(&dev.banner, banner_timer_task, NULL);
}
