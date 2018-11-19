#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <gpio.h>
#include <logging/sys_log.h>
#include <stdbool.h>

#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>

#include <uwp5661/drivers/src/bt/uki_utlis.h>
#include <uwp5661/drivers/src/bt/uki_config.h>


#include "mesh.h"
#include "light.h"
#include "health.h"

#if !defined(NODE_ADDR)
#define NODE_ADDR 0x0b0c
#endif

static struct {
	u8_t *pid;
	u8_t *pst;
	u8_t *mac;
} ali_auth[] = {
	{"767", "38d82edb69246269ddea5fa4dd04aa63", "78da07d435d8"},
	{"767", "44d0fa07fd5493b3ec3328f496f9dbcf", "78da07d435d9"},
	{"767", "bb2accd80b11e72d602fe3856bfa0280", "78da07d435da"},
	{"898", "cec73f2b36e5c2a8151b6d610408eaf1", "78da07ec0de1"},
	{"898", "5ce75a1e7affde9e432f1277bee721e1", "78da07ec0de2"},
	{"898", "89d5085bdfde54d46de147a84227d7b3", "78da07ec0de3"},
};
#define ALI_AUTH_ID 0


struct local_net_t local_net;
static struct mesh_config_t mesh_config;

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_DISABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_DISABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_DISABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif

	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};


static struct bt_mesh_cfg_cli cfg_cli = {
};


BT_MESH_HEALTH_PUB_DEFINE(health_pub, CUR_FAULTS_MAX);
BT_MESH_MODEL_PUB_DEFINE(light_onoff_pub_srv, NULL, 2 + 2);
BT_MESH_HEALTH_PUB_DEFINE(lightness_srv_pub, CUR_FAULTS_MAX);
BT_MESH_HEALTH_PUB_DEFINE(lightness_setup_srv_pub, CUR_FAULTS_MAX);

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, light_onoff_srv_op,
			  &light_onoff_pub_srv, &leds[0]),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, lightness_srv_op,
    &lightness_srv_pub, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV, lightness_setup_srv_op,
    &lightness_setup_srv_pub, NULL),
};


static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void prov_complete(u16_t net_idx, u16_t addr)
{
	BTI("Local node provisioned, net_idx 0x%04x address 0x%04x",
	       net_idx, addr);
	local_net.net_idx = net_idx,
	local_net.local = addr;

	if (mesh_config.health_enabled)
		health_init();

	if (mesh_config.light_enabled)
		light_startup();
}

static void prov_reset(void)
{
	BTI("The local node has been reset and needs reprovisioning");
	bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	BTI("OOB Number: %u", number);
	return 0;
}

static int output_string(const char *str)
{
	BTI("OOB String: %s", str);
	return 0;
}

static const char *bearer2str(bt_mesh_prov_bearer_t bearer)
{
	switch (bearer) {
	case BT_MESH_PROV_ADV:
		return "PB-ADV";
	case BT_MESH_PROV_GATT:
		return "PB-GATT";
	default:
		return "unknown";
	}
}

static void link_open(bt_mesh_prov_bearer_t bearer)
{
	BTI("Provisioning link opened on %s", bearer2str(bearer));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	BTI("Provisioning link closed on %s", bearer2str(bearer));
}

static int input(bt_mesh_input_action_t act, u8_t size)
{
	switch (act) {
	case BT_MESH_ENTER_NUMBER:
		BTI("Enter a number (max %u digits) with: input-num <num>",
		       size);
		break;
	case BT_MESH_ENTER_STRING:
		BTI("Enter a string (max %u chars) with: input-str <str>",
		       size);
		break;
	default:
		BTI("Unknown input action %u (size %u) requested!",
		       act, size);
		return -EINVAL;
	}

	local_net.input_act = act;
	local_net.input_size = size;
	return 0;
}

static u8_t static_auth[16] = {0};

static struct bt_mesh_prov prov = {
	.uuid = local_net.uuid,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.reset = prov_reset,
	.static_val = static_auth,
	.static_val_len = sizeof(static_auth),
	.output_size = 0,
	.output_actions = 0,
	.input_size = 0,
	.input_actions = 0,
	.output_number = output_number,
	.output_string = output_string,
	.input = input,
};


static void alimesh_init(void)
{
	static struct tc_sha256_state_struct sha256_ctx;
	uint8_t *p = local_net.uuid;
	int ret;
	char static_value[100] = {0};
	uint8_t result[100] = {0};

	UINT16_TO_STREAM(p, ALI_COMP_ID);
	*p++ = 0x71;
	UINT32_TO_STREAM(p, mesh_config.ali_pid);
	sys_memcpy_swap(p, mesh_config.ali_mac, sizeof(mesh_config.ali_mac));
	p += sizeof(mesh_config.ali_mac);
	memset(p, 0, local_net.uuid + sizeof(local_net.uuid) - p);
	uki_hex_dump_block("UUID:", local_net.uuid, sizeof(local_net.uuid));

	p = static_value;

	ret = snprintf(p, sizeof(static_value),
		"%08x,", mesh_config.ali_pid);
	p += ret;
	uki_hex(p, mesh_config.ali_mac, sizeof(mesh_config.ali_mac));
	p += sizeof(mesh_config.ali_mac) * 2;

	*p++ = ',';

	uki_hex(p, mesh_config.ali_secret, sizeof(mesh_config.ali_secret));
	BTE("ALI KEY: %s", static_value);

	ret = tc_sha256_init(&sha256_ctx);
    if (ret != TC_CRYPTO_SUCCESS) {
        BTE("sha256 init fail");
    }

    ret = tc_sha256_update(&sha256_ctx, static_value, strlen(static_value));
    if (ret != TC_CRYPTO_SUCCESS) {
        BTE("sha256 udpate fail");
    }

    ret = tc_sha256_final(result, &sha256_ctx);
    if (ret != TC_CRYPTO_SUCCESS) {
        BTE("sha256 final fail");
    }

	memcpy(static_auth, result, sizeof(static_auth));

	uki_hexdump("static value:", static_auth, sizeof(static_auth));
}


static void mesh_enable(int err)
{
	if (err) {
		BTI("Bluetooth init failed (err %d)", err);
		return;
	}

	BTI("Bluetooth initialized");

	light_init();

	if (mesh_config.ali_mesh) {
		alimesh_init();
	}

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		BTI("Initializing mesh failed (err %d)", err);
		return;
	}

	BTI("Mesh initialized");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		BTI("Loading stored settings");
		settings_load();
	}

	if (mesh_config.proved) {
		const u8_t net_key[16] = {
			0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
			0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		};
		const u8_t dev_key[16] = {
			0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
			0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		};
		const u8_t app_key[16] = {
			0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
			0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
		};
		const u16_t net_idx = 0;
		const u16_t app_idx = 0;
		u8_t flags = 0;
		u16_t addr = GROUP_ADDR;
		const u32_t iv_index = 0;


		BTI("provision local node");
		addr = sys_rand32_get() & 0x7FFF;
		BTI("Node address: 0x%04X", addr);

		err = bt_mesh_provision(net_key, net_idx, flags, iv_index, addr,
					dev_key);
		if (err == -EALREADY) {
			BTI("Using stored settings");
		} else if (err) {
			BTI("Provisioning failed (err %d)", err);
			return;
		} else {
			BTI("Provisioning completed & Configuring...");
			
			/* Add Application Key */
			bt_mesh_cfg_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);
			
			if (mesh_config.health_enabled) {
				/* Bind to Health model */
				bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
					BT_MESH_MODEL_ID_HEALTH_SRV, NULL);
			}

			if (mesh_config.light_enabled) {
				bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
					BT_MESH_MODEL_ID_GEN_ONOFF_SRV, NULL);
			}
			
			BTV("Configuration complete");
		}
	} else {
		bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
	}

}


void config_load(void)
{
	memset(&local_net, 0, sizeof(struct local_net_t));
	memset(&mesh_config, 0, sizeof(struct mesh_config_t));

	mesh_config.ali_mesh = 1;
	mesh_config.ali_pid = (uint32_t)(strtol(ali_auth[ALI_AUTH_ID].pid,
		NULL, 10) & 0xFFFFFFFF);
	uki_str2hex(mesh_config.ali_secret, ali_auth[ALI_AUTH_ID].pst,
		sizeof(mesh_config.ali_secret));
	uki_str2hex(mesh_config.ali_mac, ali_auth[ALI_AUTH_ID].mac,
		sizeof(mesh_config.ali_mac));
}

void mesh_init(void)
{
	int err;

	BTI("%s", __func__);


	config_load();
	err = bt_enable(mesh_enable);
	if (err) {
		BTE("Bluetooth init failed (err %d)", err);
		return;
	}
}

struct bt_mesh_model *get_root_modules(void)
{
	return &root_models[0];
}


void mesh_info(void)
{
	BTD("%s", __func__);
	BTI("Current addr: 0x%02X", local_net.local);
}
