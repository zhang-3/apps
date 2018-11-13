/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zephyr.h>
#include <bluetooth/hci.h>

#include "bt_eng.h"
#include "bt_utlis.h"
#include "../../..//zephyr/ext/hal/unisoc/uwp5661/drivers/src/bt/bt_configure.h"

struct bt_npi_dev bt_npi_dev = {
	.sync      = _K_SEM_INITIALIZER(bt_npi_dev.sync, 0, 1),
};

#define CMD_TIMEOUT      K_SECONDS(5)

#define HCI_CMD_PREAMBLE_SIZE 3
#define HCI_EVT			0x04

#define DEV_CLASS_LEN   3
#define BD_ADDR_LEN     6
#define LAP_LEN         3
typedef uint8_t LAP[LAP_LEN];
#define BT_EVENT_MASK_LEN  8
typedef uint8_t BT_EVENT_MASK[BT_EVENT_MASK_LEN];

#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}
#define DEVCLASS_TO_STREAM(p, a) {register int ijk; for (ijk = 0; ijk < DEV_CLASS_LEN;ijk++) *(p)++ = (uint8_t) a[DEV_CLASS_LEN - 1 - ijk];}
#define ARRAY_TO_STREAM(p, a, len) {register int ijk; for (ijk = 0; ijk < len;        ijk++) *(p)++ = (uint8_t) a[ijk];}
#define BDADDR_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < BD_ADDR_LEN;  ijk++) *(p)++ = (uint8_t) a[BD_ADDR_LEN - 1 - ijk];}
#define LAP_TO_STREAM(p, a)      {register int ijk; for (ijk = 0; ijk < LAP_LEN;      ijk++) *(p)++ = (uint8_t) a[LAP_LEN - 1 - ijk];}
#define ARRAY8_TO_STREAM(p, a)   {register int ijk; for (ijk = 0; ijk < 8;            ijk++) *(p)++ = (uint8_t) a[7 - ijk];}

#define CMD_MAX_LEN 258

enum { DUAL_MODE = 0, CLASSIC_MODE, LE_MODE };
enum { DISABLE_BT = 0, ENABLE_BT };

extern int bt_sipc_open(void);
extern int bt_sipc_send(unsigned char *data, int len);

static int bt_get_disable_buf(void *buf)
{
    uint8_t *p, msg_req[CMD_MAX_LEN];
    int size;

    p = msg_req;

    UINT16_TO_STREAM(p, DUAL_MODE);
    UINT8_TO_STREAM(p, DISABLE_BT);
    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

static int bt_get_enable_buf(void *buf)
{
    uint8_t *p, msg_req[CMD_MAX_LEN];
    int size;

    p = msg_req;

    UINT16_TO_STREAM(p, DUAL_MODE);
    UINT8_TO_STREAM(p, ENABLE_BT);
    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

static int bt_get_rf_buf(void *buf)
{
    uint8_t *p, msg_req[CMD_MAX_LEN];
    int i, size;

    p = msg_req;

    for (i = 0; i < 6; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_GainValue_A[i]);
    }

    for (i = 0; i < 10; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_ClassicPowerValue_A[i]);
    }

    for (i = 0; i < 16; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEPowerValue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_BRChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_EDRChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEChannelpwrvalue_A[i]);
    }

    for (i = 0; i < 6; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_GainValue_B[i]);
    }

    for (i = 0; i < 10; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_ClassicPowerValue_B[i]);
    }


    for (i = 0; i < 16; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEPowerValue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_BRChannelpwrvalue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_EDRChannelpwrvalue_B[i]);
    }

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_rf_config.g_LEChannelpwrvalue_B[i]);
    }

    UINT16_TO_STREAM(p, marlin3_rf_config.LE_fix_powerword);

    UINT8_TO_STREAM(p, marlin3_rf_config.Classic_pc_by_channel);
    UINT8_TO_STREAM(p, marlin3_rf_config.LE_pc_by_channel);
    UINT8_TO_STREAM(p, marlin3_rf_config.RF_switch_mode);
    UINT8_TO_STREAM(p, marlin3_rf_config.Data_Capture_Mode);
    UINT8_TO_STREAM(p, marlin3_rf_config.Analog_IQ_Debug_Mode);
    UINT8_TO_STREAM(p, marlin3_rf_config.RF_common_rfu_b3);

    for (i = 0; i < 5; i++) {
        UINT32_TO_STREAM(p, marlin3_rf_config.RF_common_rfu_w[i]);
    }

    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}


static int bt_get_pskey_buf(void *buf)
{
    uint8_t *p, msg_req[CMD_MAX_LEN];
    int i, size;

    p = msg_req;
    UINT32_TO_STREAM(p, marlin3_pskey.device_class);

    for (i = 0; i < 16; i++) {
        UINT8_TO_STREAM(p, marlin3_pskey.feature_set[i]);
    }

    for (i = 0; i < 6; i++) {
        UINT8_TO_STREAM(p, marlin3_pskey.device_addr[i]);
    }

    UINT16_TO_STREAM(p, marlin3_pskey.comp_id);
    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_uart0_communication_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.cp2_log_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.LogLevel);
    UINT8_TO_STREAM(p, marlin3_pskey.g_central_or_perpheral);

    UINT16_TO_STREAM(p, marlin3_pskey.Log_BitMask);
    UINT8_TO_STREAM(p, marlin3_pskey.super_ssp_enable);
    UINT8_TO_STREAM(p, marlin3_pskey.common_rfu_b3);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.common_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.le_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.lmp_rfu_w[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.lc_rfu_w[i]);
    }

    UINT16_TO_STREAM(p, marlin3_pskey.g_wbs_nv_117);
    UINT16_TO_STREAM(p, marlin3_pskey.g_wbs_nv_118);
    UINT16_TO_STREAM(p, marlin3_pskey.g_nbv_nv_117);
    UINT16_TO_STREAM(p, marlin3_pskey.g_nbv_nv_118);

    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sco_transmit_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.audio_rfu_b1);
    UINT8_TO_STREAM(p, marlin3_pskey.audio_rfu_b2);
    UINT8_TO_STREAM(p, marlin3_pskey.audio_rfu_b3);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.audio_rfu_w[i]);
    }

    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sleep_in_standby_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sleep_master_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.g_sys_sleep_slave_supported);
    UINT8_TO_STREAM(p, marlin3_pskey.power_rfu_b1);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.power_rfu_w[i]);
    }

    UINT32_TO_STREAM(p, marlin3_pskey.win_ext);

    UINT8_TO_STREAM(p, marlin3_pskey.edr_tx_edr_delay);
    UINT8_TO_STREAM(p, marlin3_pskey.edr_rx_edr_delay);
    UINT8_TO_STREAM(p, marlin3_pskey.tx_delay);
    UINT8_TO_STREAM(p, marlin3_pskey.rx_delay);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.bb_rfu_w[i]);
    }

    UINT8_TO_STREAM(p, marlin3_pskey.agc_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.diff_or_eq);
    UINT8_TO_STREAM(p, marlin3_pskey.ramp_mode);
    UINT8_TO_STREAM(p, marlin3_pskey.modem_rfu_b1);

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.modem_rfu_w[i]);
    }

    UINT32_TO_STREAM(p, marlin3_pskey.BQB_BitMask_1);
    UINT32_TO_STREAM(p, marlin3_pskey.BQB_BitMask_2);

    for (i = 0; i < 8; i++) {
        UINT16_TO_STREAM(p, marlin3_pskey.bt_coex_threshold[i]);
    }

    for (i = 0; i < 2; i++) {
        UINT32_TO_STREAM(p, marlin3_pskey.other_rfu_w[i]);
    }

    size = p - msg_req;
    memcpy(buf, msg_req, size);
    return size;
}

static void cmd_complete_handle(unsigned char *data, u8_t total_len)
{
	struct bt_hci_evt_cmd_complete *evt = (void *)data;
	u16_t opcode = sys_le16_to_cpu(evt->opcode);
	u8_t evt_len = sizeof(struct bt_hci_evt_cmd_complete);

	BT_LOG("%s, opcode 0x%04x\n", __func__, opcode);

	if (bt_npi_dev.last_cmd != opcode) {
		BT_LOG("OpCode 0x%04x completed instead of expected 0x%04x\n",
			opcode, bt_npi_dev.last_cmd);
		return;
	}
	bt_npi_dev.opcode = opcode;
	bt_npi_dev.len = total_len - evt_len;

	memset(bt_npi_dev.data, 0, sizeof(bt_npi_dev.data));
	memcpy(bt_npi_dev.data, &data[evt_len], bt_npi_dev.len);
	k_sem_give(&bt_npi_dev.sync);
}

static void cmd_status_handle(unsigned char *data)
{
	struct bt_hci_evt_cmd_status *evt = (void *)data;
	u16_t opcode = sys_le16_to_cpu(evt->opcode);

	BT_LOG("%s, opcode 0x%04x\n", __func__, opcode);

	if (bt_npi_dev.last_cmd != opcode) {
		BT_LOG("OpCode 0x%04x completed instead of expected 0x%04x\n",
			opcode, bt_npi_dev.last_cmd);
		return;
	}
	bt_npi_dev.opcode = opcode;
	bt_npi_dev.len = 1;

	memset(bt_npi_dev.data, 0, sizeof(bt_npi_dev.data));
	memcpy(bt_npi_dev.data, &evt->status, bt_npi_dev.len);
	k_sem_give(&bt_npi_dev.sync);
}

void bt_npi_recv(unsigned char *data, int len)
{
	BT_LOG("%s\n", __func__);
	u8_t packet_type;
	u8_t hci_evt;

	BT_LOG("%s, len = %d\n", __func__,len);

	packet_type = data[0];
	hci_evt = data[1];
	BT_LOG("%s, packet_type = 0x%02x,hci_evt = 0x%02x\n", __func__,packet_type,hci_evt);

	switch (packet_type) {
	case HCI_EVT:
		switch (hci_evt) {
		case BT_HCI_EVT_CMD_COMPLETE:
			cmd_complete_handle(&data[3], data[2]);
			break;
		case BT_HCI_EVT_CMD_STATUS:
			cmd_status_handle(&data[3]);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

int hci_cmd_send_sync(u16_t cmd, u8_t *payload, u8_t len)
{
	u8_t *p_buf;
	u8_t *p;
	int err;
	u16_t cmd_len;

	cmd_len = HCI_CMD_PREAMBLE_SIZE + len + 1;
	p_buf = k_calloc(1,cmd_len);
	if (p_buf)
	{
		p = (u8_t *)(p_buf);
		UINT8_TO_STREAM(p, 0x01);
		UINT16_TO_STREAM(p, cmd);
		*p++ = len;
		memcpy(p, payload, len);

		bt_sipc_send(p_buf,cmd_len);
		k_free(p_buf);
		bt_npi_dev.last_cmd = cmd;
		err = k_sem_take(&bt_npi_dev.sync,CMD_TIMEOUT);
		__ASSERT(err == 0, "k_sem_take failed with err %d", err);
		return 0;
	}else{
		BT_LOG("k_calloc fail\n");
		return -1;
	}
}

void vendor_init()
{
	int size;
	char data[256] = {0};

	BT_LOG("send pskey\n");
	size = bt_get_pskey_buf(data);
	hci_cmd_send_sync(HCI_OP_PSKEY,data,size);

	BT_LOG("send rfkey\n");
	size = bt_get_rf_buf(data);
	hci_cmd_send_sync(HCI_OP_RF,data,size);

	BT_LOG("send enable\n");
	size = bt_get_enable_buf(data);
	hci_cmd_send_sync(HCI_OP_ENABLE,data,size);
}

int engpc_bt_on(void) {
	BT_LOG("%s\n",__func__);

	bt_sipc_open();

	vendor_init();
	return 0;
}

int engpc_bt_off(void) {
	BT_LOG("%s\n",__func__);
	int size;
	char data[256] = {0};

	BT_LOG("send disable\n");
	size = bt_get_disable_buf(data);
	hci_cmd_send_sync(HCI_OP_ENABLE,data,size);
	return 0;
}

bool btsnd_hcic_set_event_filter (uint8_t filt_type, uint8_t filt_cond_type,
									uint8_t *filt_cond, uint8_t filt_cond_len)
{
	uint8_t *p, msg_req[258];
	int size;
	p = msg_req;

	if (filt_type)
	{
		UINT8_TO_STREAM (p, filt_type);
		UINT8_TO_STREAM (p, filt_cond_type);

		if (filt_cond_type == HCI_FILTER_COND_DEVICE_CLASS)
		{
			DEVCLASS_TO_STREAM (p, filt_cond);
			filt_cond += DEV_CLASS_LEN;
			DEVCLASS_TO_STREAM (p, filt_cond);
			filt_cond += DEV_CLASS_LEN;
			filt_cond_len -= (2 * DEV_CLASS_LEN);
		}
		else if (filt_cond_type == HCI_FILTER_COND_BD_ADDR)
		{
			BDADDR_TO_STREAM (p, filt_cond);
			filt_cond += BD_ADDR_LEN;
			filt_cond_len -= BD_ADDR_LEN;
		}

		if (filt_cond_len)
			ARRAY_TO_STREAM (p, filt_cond, filt_cond_len);
	}
	else
	{
		UINT8_TO_STREAM (p, filt_type);
	}

	size = p - msg_req;
	hci_cmd_send_sync(HCI_SET_EVENT_FILTER, msg_req, size);
	return true;
}

bool btsnd_hcic_write_scan_enable (uint8_t flag)
{
	uint8_t *p, msg_req[258];
	int size;
	p = msg_req;

	UINT8_TO_STREAM  (p, flag);

	size = p - msg_req;
	hci_cmd_send_sync(HCI_WRITE_SCAN_ENABLE, msg_req, size);
	return true;
}

bool btsnd_hcic_write_cur_iac_lap (uint8_t num_cur_iac, LAP * const iac_lap)
{
	uint8_t *p, msg_req[258];
	int i,size;
	p = msg_req;

	UINT8_TO_STREAM (p, num_cur_iac);

	for (i = 0; i < num_cur_iac; i++)
		LAP_TO_STREAM (p, iac_lap[i]);

	size = p - msg_req;
	hci_cmd_send_sync(HCI_WRITE_CURRENT_IAC_LAP, msg_req, size);
	return true;
}

bool btsnd_hcic_set_event_mask(BT_EVENT_MASK event_mask)
{
	uint8_t *p, msg_req[258];
	int size;
	p = msg_req;

	ARRAY8_TO_STREAM (p, event_mask);

	size = p - msg_req;
	hci_cmd_send_sync(HCI_SET_EVENT_MASK, msg_req, size);
	return true;
}

bool btsnd_hcic_enable_test_mode (void)
{
	hci_cmd_send_sync(HCI_ENABLE_DEV_UNDER_TEST_MODE, NULL, 0);
	return true;
}

int dut_mode_enable(void)
{
	uint8_t cond = HCI_DO_AUTO_ACCEPT_CONNECT;
	uint8_t scan_mode = HCI_PAGE_SCAN_ENABLED;
	uint8_t general_inq_lap[3] = {0x9e,0x8b,0x33};

	BT_LOG("BTM: BTM_EnableTestMode\n");

	if (!btsnd_hcic_set_event_filter(HCI_FILTER_CONNECTION_SETUP,
									HCI_FILTER_COND_NEW_DEVICE,
									&cond, sizeof(cond)))
		return -1;

	if (!btsnd_hcic_write_scan_enable(scan_mode))
		return -1;

	if (!btsnd_hcic_write_cur_iac_lap (1, (LAP * const) &general_inq_lap))
		return -1;

	scan_mode |= HCI_INQUIRY_SCAN_ENABLED;
	if (!btsnd_hcic_write_scan_enable(scan_mode))
		return -1;

	if (!btsnd_hcic_set_event_mask((uint8_t *)"\x00\x00\x00\x00\x00\x00\x00\x00"))
		return -1;

	if (!btsnd_hcic_enable_test_mode ())
		return -1;

	return 0;
}

int dut_mode_disable(void)
{
    return hci_cmd_send_sync(HCI_RESET, NULL, 0);
}

int engpc_bt_dut_mode_configure(uint8_t enable) {
	if (enable)
		return dut_mode_enable();
	else
		return dut_mode_disable();
}

int engpc_bt_set_nonsig_tx_testmode(uint16_t enable,
	uint16_t le, uint16_t pattern, uint16_t channel,
	uint16_t pac_type, uint16_t pac_len, uint16_t power_type,
	uint16_t power_value, uint16_t pac_cnt)
{
	uint16_t opcode;
	BT_LOG("%s\n", __FUNCTION__);

	BT_LOG("enable  : %X\n", enable);
	BT_LOG("le      : %X\n", le);

	BT_LOG("pattern : %X\n", pattern);
	BT_LOG("channel : %X\n", channel);
	BT_LOG("pac_type: %X\n", pac_type);
	BT_LOG("pac_len : %X\n", pac_len);
	BT_LOG("power_type   : %X\n", power_type);
	BT_LOG("power_value  : %X\n", power_value);
	BT_LOG("pac_cnt      : %X\n", pac_cnt);

	if(enable){
		opcode = le ? NONSIG_LE_TX_ENABLE : NONSIG_TX_ENABLE;
	}else{
		opcode = le ? NONSIG_LE_TX_DISABLE : NONSIG_TX_DISABLE;
	}

	if(enable){
		uint8_t buf[11];
		memset(buf,0x0,sizeof(buf));

		buf[0] = (uint8_t)pattern;
		buf[1] = (uint8_t)channel;
		buf[2] = (uint8_t)(pac_type & 0x00FF);
		buf[3] = (uint8_t)((pac_type & 0xFF00) >> 8);
		buf[4] = (uint8_t)(pac_len & 0x00FF);
		buf[5] = (uint8_t)((pac_len & 0xFF00) >> 8);
		buf[6] = (uint8_t)power_type;
		buf[7] = (uint8_t)(power_value & 0x00FF);
		buf[8] = (uint8_t)((power_value & 0xFF00) >> 8);
		buf[9] = (uint8_t)(pac_cnt & 0x00FF);
		buf[10] = (uint8_t)((pac_cnt & 0xFF00) >> 8);

		BT_LOG("send hci cmd, opcode = 0x%X\n", opcode);
		hci_cmd_send_sync(opcode, buf, sizeof(buf));
	}else{/* disable */
		BT_LOG("send hci cmd, opcode = 0x%X\n",opcode);
		hci_cmd_send_sync(opcode, NULL, 0);
	}
	return 0;
}

int engpc_bt_set_nonsig_rx_testmode(uint16_t enable,
	uint16_t le, uint16_t pattern, uint16_t channel,
	uint16_t pac_type,uint16_t rx_gain, bt_bdaddr_t *addr)
{
	uint16_t opcode;
	BT_LOG("%s\n",__FUNCTION__);

	BT_LOG("enable  : %X\n",enable);
	BT_LOG("le      : %X\n",le);

	BT_LOG("pattern : %d\n",pattern);
	BT_LOG("channel : %d\n",channel);
	BT_LOG("pac_type: %d\n",pac_type);
	BT_LOG("rx_gain : %d\n",rx_gain);
	BT_LOG("addr    : %02X:%02X:%02X:%02X:%02X:%02X\n",
		addr->address[0],addr->address[1],addr->address[2],
		addr->address[3],addr->address[4],addr->address[5]);

	if(enable){
		opcode = le ? NONSIG_LE_RX_ENABLE : NONSIG_RX_ENABLE;
	}else{
		opcode = le ? NONSIG_LE_RX_DISABLE : NONSIG_RX_DISABLE;
	}

	if(enable){
		uint8_t buf[11];
		memset(buf,0x0,sizeof(buf));

		buf[0] = (uint8_t)pattern;
		buf[1] = (uint8_t)channel;
		buf[2] = (uint8_t)(pac_type & 0x00FF);
		buf[3] = (uint8_t)((pac_type & 0xFF00) >> 8);
		buf[4] = (uint8_t)rx_gain;
		buf[5] = addr->address[5];
		buf[6] = addr->address[4];
		buf[7] = addr->address[3];
		buf[8] = addr->address[2];
		buf[9] = addr->address[1];
		buf[10] = addr->address[0];
		hci_cmd_send_sync(opcode, buf, sizeof(buf));
	}else{
		hci_cmd_send_sync(opcode, NULL, 0);
	}
	return 0;
}

int engpc_bt_dut_mode_send(uint16_t opcode, uint8_t *buf, uint8_t len)
{
	if (opcode == HCI_DUT_SET_TXPWR || opcode == HCI_DUT_SET_RXGIAN) {
		return hci_cmd_send_sync(opcode, buf, len);
	} else {
		return -1;
	}
}

static void handle_nonsig_rx_data(hci_cmd_complete_t *p, char *des, uint16_t *read_len)
{
	uint8_t result,rssi;
	uint32_t pkt_cnt,pkt_err_cnt;
	uint32_t bit_cnt,bit_err_cnt;
	uint8_t *buf;
	char data[255] = { 0 };

	BT_LOG("%s\n", __FUNCTION__);
	BT_LOG("opcode = 0x%X\n", p->opcode);
	BT_LOG("param_len = 0x%X\n", p->param_len);

	if (p->param_len != 18) {
		snprintf(data, sizeof(data), "%s %s", "FAIL", "response from controller is invalid");
		goto error;
	}

	buf = p->p_param_buf;
	result = *buf;
	rssi = *(buf + 1);
	pkt_cnt = *(uint32_t *)(buf + 2);
	pkt_err_cnt = *(uint32_t *)(buf + 6);
	bit_cnt = *(uint32_t *)(buf + 10);
	bit_err_cnt = *(uint32_t *)(buf + 14);

	BT_LOG("ret:0x%X, rssi:0x%X, pkt_cnt:0x%X, pkt_err_cnt:0x%X, bit_cnt:0x%X, pkt_err_cnt:0x%X\n",
		result, rssi, pkt_cnt, pkt_err_cnt, bit_cnt, bit_err_cnt);

	if(result == 0){
		snprintf(data, sizeof(data), "%s rssi:%d, pkt_cnt:%d, pkt_err_cnt:%d, bit_cnt:%d, bit_err_cnt:%d",
			"OK", rssi, pkt_cnt, pkt_err_cnt, bit_cnt, bit_err_cnt);
	}else{
		snprintf(data, sizeof(data), "%s %s", "FAIL", "response from controller is invalid");
	}

error:
	*read_len = strlen(data);
	memcpy(des, data, strlen(data));
	BT_LOG("get %d bytes %s\n", strlen(data), des);
}

int engpc_bt_get_nonsig_rx_data(uint16_t le, char *buf, uint16_t buf_len, uint16_t *read_len)
{
	BT_LOG("get_nonsig_rx_data LE=%d\n",le);
	uint16_t opcode;
	hci_cmd_complete_t vcs_cplt_params;

	opcode = le ? NONSIG_LE_RX_GETDATA : NONSIG_RX_GETDATA;
	hci_cmd_send_sync(opcode, NULL, 0);

	vcs_cplt_params.opcode = opcode;
	vcs_cplt_params.param_len = bt_npi_dev.len;
	vcs_cplt_params.p_param_buf = bt_npi_dev.data;

	handle_nonsig_rx_data(&vcs_cplt_params, buf,read_len);
	return 0;
}

int engpc_bt_le_enhanced_receiver(uint8_t channel, uint8_t phy, uint8_t modulation_index)
{
	uint8_t buf[3];

	BT_LOG("%s channel: 0x%02x, phy: 0x%02x, modulation_index: 0x%02x\n", __func__, channel, phy, modulation_index);
	buf[0] = channel;
	buf[1] = phy;
	buf[2] = modulation_index;
	hci_cmd_send_sync(HCI_BLE_ENHANCED_RECEIVER_TEST, buf, sizeof(buf));
	return 0;
}

int engpc_bt_le_enhanced_transmitter(uint8_t channel, uint8_t length, uint8_t payload, uint8_t phy)
{
	uint8_t buf[4];

	BT_LOG("%s channel: 0x%02x, length: 0x%02x, payload: 0x%02x, phy: 0x%02x\n", __func__, channel, length, payload, phy);
	buf[0] = channel;
	buf[1] = length;
	buf[2] = payload;
	buf[3] = phy;
	hci_cmd_send_sync(HCI_BLE_ENHANCED_TRANSMITTER_TEST, buf, sizeof(buf));
	return 0;
}

int engpc_bt_le_test_end(void)
{
	return hci_cmd_send_sync(HCI_BLE_TEST_END, NULL, 0);
}

int engpc_bt_set_rf_path(uint8_t path)
{
	return hci_cmd_send_sync(HCI_DUT_SET_RF_PATH, &path, sizeof(path));
}

int engpc_bt_get_rf_path(void)
{
	uint8_t status, rf_path;
	hci_cmd_send_sync(HCI_DUT_GET_RF_PATH, NULL, 0);
	status = bt_npi_dev.data[0];
	rf_path = bt_npi_dev.data[1];
	BT_LOG("%s, status = %d, rf_path = %d\n", __func__, status, rf_path);
	return rf_path;
}

static void handle_dut_rx_data(hci_cmd_complete_t *p, char *des, uint16_t *read_len)
{
	uint8_t *buf;
	uint8_t status;
	uint8_t rssi;
	char data[255] = { 0 };

	BT_LOG("%s\n", __FUNCTION__);
	BT_LOG("opcode = 0x%X\n", p->opcode);
	BT_LOG("param_len = 0x%X\n", p->param_len);

	if (p->param_len != 2) {
		snprintf(data, sizeof(data), "%s %s", "FAIL", "response from controller is invalid");
		goto error;
	}

	buf = p->p_param_buf;
	status = *buf;
	rssi = *(buf + 1);

	BT_LOG("status = %d, RSSI = %d\n", status, rssi);

	if(status == 0){
		snprintf(data, sizeof(data), "%s HCI_DUT_GET_RXDATA status:%d, rssi:%d", "OK", status, rssi);
	}else{
		snprintf(data, sizeof(data), "%s HCI_DUT_GET_RXDATA status:%d, rssi:%d", "FAIL", status, rssi);
	}

error:
	*read_len = strlen(data);
	memcpy(des, data, strlen(data));
	BT_LOG("get %d bytes %s\n", strlen(data), des);
}

int engpc_bt_dut_mode_get_rx_data(char *buf, uint16_t buf_len, uint16_t *read_len)
{
	uint16_t opcode;
	hci_cmd_complete_t vcs_cplt_params;

	opcode = HCI_DUT_GET_RXDATA;
	hci_cmd_send_sync(opcode, NULL, 0);

	vcs_cplt_params.opcode = opcode;
	vcs_cplt_params.param_len = bt_npi_dev.len;
	vcs_cplt_params.p_param_buf = bt_npi_dev.data;

	handle_dut_rx_data(&vcs_cplt_params, buf, read_len);
	return 0;
}
