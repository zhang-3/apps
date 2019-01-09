/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <device.h>
#include <uart.h>
#include <bluetooth/hci.h>

#include "bqb.h"
#include "bt_utlis.h"

#define HCI_OP_PSKEY 0xFCA0
#define HCI_OP_RF 0xFCA2
#define HCI_OP_ENABLE 0xFCA1
#define HCI_CMD_PREAMBLE_SIZE 3


#define NOTIFY_BQB_ENABLE ("\r\n+SPBQBTEST OK: ENABLED\r\n")
#define NOTIFY_BQB_DISABLE ("\r\n+SPBQBTEST OK: DISABLE\r\n")
#define NOTIFY_BQB_ENABLE_ERROR ("\r\n+SPBQBTEST ERROR: LOAD CP FAILED\r\n")


#define UINT16_TO_STREAM(p, u16) {*(p)++ = (u8_t)(u16); *(p)++ = (u8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (u8_t)(u8);}
#define STREAM_TO_UINT16(u16, p) {u16 = ((u16_t)(*(p)) + (((u16_t)(*((p) + 1))) << 8));}

static bt_bqb_state_t bqb_state =BQB_CLOSED;

K_THREAD_STACK_MEMBER(transmiter_stack, 1024);
static struct k_thread transmiter;
k_tid_t trans_tid;

static struct k_sem rx_sem;
static struct k_sem cb_sem;
static struct k_sem sync_sem;

static int running;
static struct device * userial_fd =NULL;
static packet_receive_data_t *recv_serial_buf;

static int fw_cfging =0;

static const u8_t preamble_sizes[] = {
	HCI_COMMAND_PREAMBLE_SIZE,
	HCI_ACL_PREAMBLE_SIZE,
	HCI_SCO_PREAMBLE_SIZE,
	HCI_EVENT_PREAMBLE_SIZE
};

typedef struct {
	u16_t len;
	u8_t data[1300];
} BQB_HDR;

static BQB_HDR rx_data;

extern int get_enable_buf(void *buf);
extern int get_disable_buf(void *buf);
extern int marlin3_rf_preload(void *buf);
extern int get_pskey_buf(void *buf);

static int bqb_write2pc(unsigned char* diag_data, int r_cnt, struct device *uart) {
	int offset = 0;
	int w_cnt = 0;

	BTD("rcnt is : %d\n", r_cnt);
	do {
		w_cnt = uart_fifo_fill(uart, diag_data + offset, r_cnt);
		if (w_cnt < 0) {
			BTD("bqb_write2pc no data write:%d ,%d\n", w_cnt, errno);
		}
		BTD("bqb_write2pc ,write : %d\n",w_cnt);
		r_cnt -= w_cnt;
		offset += w_cnt;
		BTD("bqb_write2pc, rcnt : %d\n", r_cnt);
	} while (r_cnt > 0);

	BTD("bqb_write2pc ,write finished, return offset : %d\n",offset);
	return offset;
}

static void userial_transmit(void)
{
	do {
		BTD("userial_transmit wait for write\n");
		k_sem_take(&rx_sem, K_FOREVER);
		BTD("userial_transmit write len: %d\n", rx_data.len);
		bqb_write2pc(rx_data.data,  rx_data.len, userial_fd);
		k_sem_give(&cb_sem);
	} while (running);
	return NULL;
}

void handle_recv_data(unsigned char *data, int len)
{
	BTD("%s\n", __func__);
	unsigned char *rxmsg = NULL;
	u16_t opcode = 0;

	rxmsg = data;
	if ((rxmsg[0] == DATA_TYPE_EVENT) && (rxmsg[1] == BT_HCI_EVT_CMD_COMPLETE)) {
		STREAM_TO_UINT16(opcode,rxmsg + 4);
		BTD("opcode 0x%04x\n", opcode);
		if ((opcode == HCI_OP_PSKEY)||(opcode == HCI_OP_RF)) {
			k_sem_give(&sync_sem);
		}
		if (opcode == HCI_OP_ENABLE) {
			k_sem_give(&sync_sem);
			fw_cfging = 0;
		}
	}
}

void bqb_recv_cb(unsigned char *data, int len)
{
	BTD("%s\n", __func__);

	if (fw_cfging ==1) {
		BTD("%s,fw_cfging\n", __func__);
		handle_recv_data(data,len);
		return;
	}

	k_sem_take(&cb_sem,K_FOREVER);
	rx_data.len = len;
	BTD("bqb_recv_cb copy data: %d\n", len);
	memcpy(rx_data.data, data, len);
	k_sem_give(&rx_sem);
}

void send_hci_cmd(u16_t cmd, u8_t *payload, u8_t len)
{
	u8_t *p_buf;
	u8_t *p;

	p_buf = k_calloc(1,HCI_CMD_PREAMBLE_SIZE + len + 1);
	if (p_buf) {
		p = (u8_t *)(p_buf);
		UINT8_TO_STREAM(p, 0x01);
		UINT16_TO_STREAM(p, cmd);
		*p++ = len;
		memcpy(p, payload, len);

		bt_sipc_send(p_buf,HCI_CMD_PREAMBLE_SIZE + len + 1);
		k_free(p_buf);
	}
}

void bt_on(void) {
	BTD("%s\n", __func__);
	int size;
	char data[256] = {0};

	fw_cfging = 1;
	k_sem_init(&sync_sem, 0, 1);

	BTD("send pskey\n");
	size = get_pskey_buf(data);
	send_hci_cmd(HCI_OP_PSKEY,data,size);
	k_sem_take(&sync_sem,K_FOREVER);

	BTD("send rfkey\n");
	size = marlin3_rf_preload(data);
	send_hci_cmd(HCI_OP_RF,data,size);
	k_sem_take(&sync_sem,K_FOREVER);

	BTD("send enable\n");
	size = get_enable_buf(data);
	send_hci_cmd(HCI_OP_ENABLE,data,size);
	k_sem_take(&sync_sem,K_FOREVER);
}

void bt_off(void) {
	BTD("%s\n", __func__);
	int size;
	char data[256] = {0};

	fw_cfging = 1;
	BTD("send disable\n");
	size = get_disable_buf(data);
	send_hci_cmd(HCI_OP_ENABLE,data,size);
	k_sem_take(&sync_sem,K_FOREVER);
}

int bqb_enable(struct device *uart)
{
	BTD("%s\n", __func__);

	bqb_state = BQB_OPENED;

	recv_serial_buf = (packet_receive_data_t*)k_malloc(sizeof(packet_receive_data_t));
	memset(recv_serial_buf, 0, sizeof(packet_receive_data_t));
	if (recv_serial_buf == NULL) {
		BTD("%s malloc recv_serial_buf failed\n",__func__);
		return -1;
	}

	userial_fd = uart;
	bt_on();
	memset(&rx_data, 0, sizeof(BQB_HDR));
	k_sem_init(&rx_sem, 0, 1);
	k_sem_init(&cb_sem, 1, 1);
	running = 1;

	trans_tid = k_thread_create(&transmiter, transmiter_stack,
				K_THREAD_STACK_SIZEOF(transmiter_stack),
				(k_thread_entry_t)userial_transmit, NULL, NULL, NULL,
				K_PRIO_COOP(8),
				0, K_NO_WAIT);

	uart_fifo_fill(userial_fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
	return 0;
}

int bqb_disable(void)
{
	BTD("%s\n", __func__);

	k_sem_reset(&rx_sem);
	k_sem_reset(&cb_sem);
	running = 0;
	k_thread_abort(trans_tid);

	bt_off();
	uart_fifo_fill(userial_fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));

	if (recv_serial_buf != NULL) {
		BTD("%s ,free recv_serial_buf\n", __func__);
		k_free(recv_serial_buf);
		recv_serial_buf = NULL;
	}
	bqb_state = BQB_CLOSED;
	return 0;
}

static void parse_frame(unsigned char *data, int len, packet_receive_data_t *receive_data)
{
	uint8_t byte;
	while (len) {
		STREAM_TO_UINT8(byte, data);
		len--;
		switch (receive_data->state) {
			case BRAND_NEW:
				if (byte > DATA_TYPE_EVENT || byte < DATA_TYPE_COMMAND) {
					BTD("%s, unknow head: 0x%02x\n", __func__, byte);
					break;
				}
				receive_data->type = byte;
				/* Initialize and prepare to jump to the preamble reading state*/
				receive_data->bytes_remaining = preamble_sizes[PACKET_TYPE_TO_INDEX(receive_data->type)] + 1;
				memset(receive_data->preamble, 0, PREAMBLE_BUFFER_SIZE);
				receive_data->index = 0;
				receive_data->state = PREAMBLE;
			case PREAMBLE:
				receive_data->preamble[receive_data->index] = byte;
				receive_data->index++;
				receive_data->bytes_remaining--;

				if (receive_data->bytes_remaining == 0) {
					/* For event and sco preambles, the last byte we read is the length*/
					receive_data->bytes_remaining = (receive_data->type == DATA_TYPE_ACL) ? RETRIEVE_ACL_LENGTH(receive_data->preamble) : byte;
					size_t buffer_size = receive_data->index + receive_data->bytes_remaining;
					BTD("%s, recv data size =: 0x%02x\n", __func__, buffer_size);

					memcpy(receive_data->buffer, receive_data->preamble, receive_data->index);
					receive_data->state = receive_data->bytes_remaining > 0 ? BODY : FINISHED;
				}
				break;
			case BODY:
				receive_data->buffer[receive_data->index] = byte;
				receive_data->index++;
				receive_data->bytes_remaining--;

				size_t bytes_read = (len >= receive_data->bytes_remaining) ? receive_data->bytes_remaining : len;
				memcpy(receive_data->buffer + receive_data->index, data, bytes_read);
				receive_data->index += bytes_read;
				receive_data->bytes_remaining -= bytes_read;

				receive_data->state = receive_data->bytes_remaining == 0 ? FINISHED : receive_data->state;
				len = len - bytes_read;
				break;
			case IGNORE:
				BTD("%s, PARSE IGNORE\n",__func__);
				receive_data->bytes_remaining--;
				if (receive_data->bytes_remaining == 0) {
					receive_data->state = BRAND_NEW;
					return;
				}
				break;
			case FINISHED:
				BTD("%s ,the state machine should not have been left in the finished state.\n", __func__);
			break;
			default:
				BTD("%s ,PARSE DEFAULT\n",__func__);
			break;
		}

		if (receive_data->state == FINISHED) {
			bt_sipc_send(receive_data->buffer, receive_data->index);
			receive_data->state = BRAND_NEW;
			return;
		}
	}
}

void bqb_userial_data_handle(unsigned char *buf, int len)
{
	BTD("%s ,len = %d\n", __func__,len);

	if (len > 0) {
		parse_frame(buf, len ,recv_serial_buf);
	}
}

int get_bqb_state(void)
{
	return (bqb_state==BQB_OPENED ? 1 : 0);
}
