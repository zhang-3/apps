#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <gpio.h>
#include <logging/sys_log.h>
#include <stdbool.h>

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
#include "health.h"
#include "uwp_hal.h"

static u8_t cur_faults[CUR_FAULTS_MAX];
static u8_t reg_faults[CUR_FAULTS_MAX * 2];

extern struct local_net_t local_net;

static void hl_get_faults(u8_t *faults, u8_t faults_size, u8_t *dst, u8_t *count)
{
	u8_t i, limit = *count;

	for (i = 0, *count = 0; i < faults_size && *count < limit; i++) {
		if (faults[i]) {
			*dst++ = faults[i];
			(*count)++;
		}
	}
}

static int hl_fault_get_cur(struct bt_mesh_model *model, u8_t *test_id,
			 u16_t *company_id, u8_t *faults, u8_t *fault_count)
{
	BTI("Sending current faults");

	*test_id = 0x00;
	*company_id = BT_COMP_ID_LF;

	hl_get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int hl_fault_get_reg(struct bt_mesh_model *model, u16_t cid,
			 u8_t *test_id, u8_t *faults, u8_t *fault_count)
{
	if (cid != BT_COMP_ID_LF) {
		BTE("Faults requested for unknown Company ID 0x%04x", cid);
		return -EINVAL;
	}

	BTI("Sending registered faults");

	*test_id = 0x00;

	hl_get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int hl_fault_clear(struct bt_mesh_model *model, uint16_t cid)
{
	if (cid != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int hl_fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t cid)
{
	if (cid != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	if (test_id != 0x00) {
		return -EINVAL;
	}

	return 0;
}

static void hl_attention_on(struct bt_mesh_model *model)
{
	BTI("attention_on()");
}

static void hl_attention_off(struct bt_mesh_model *model)
{
	BTI("attention_off()");
}


static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = hl_fault_get_cur,
	.fault_get_reg = hl_fault_get_reg,
	.fault_clear = hl_fault_clear,
	.fault_test = hl_fault_test,
	.attn_on = hl_attention_on,
	.attn_off = hl_attention_off,
};

struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};


void health_init(void)
{
	BTI("%s", __func__);
	struct bt_mesh_cfg_hb_pub pub = {
		.dst = GROUP_ADDR,
		.count = 0xff,
		.period = 0x05,
		.ttl = 0x07,
		.feat = 0,
		.net_idx = BT_MESH_KEY_DEV,
	};
	
	bt_mesh_cfg_hb_pub_set(local_net.net_idx, local_net.local, &pub, NULL);
	BTI("Publishing heartbeat messages");
}

