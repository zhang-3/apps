/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sipc.h>
#include <sblock.h>

#include "bt_utlis.h"

#define SPRD_DP_RW_REG_SHIFT_BYTE 14
#define SPRD_DP_DMA_READ_BUFFER_BASE_ADDR 0x40280000
#define SPRD_DP_DMA_UARD_SDIO_BUFFER_BASE_ADDR (SPRD_DP_DMA_READ_BUFFER_BASE_ADDR + (1 << SPRD_DP_RW_REG_SHIFT_BYTE))
#define WORD_ALIGN 4


static K_THREAD_STACK_DEFINE(rx_thread_stack, 1024);
static struct k_thread rx_thread_data;
static struct k_sem	event_sem;
extern int check_bteut_ready(void);
extern void bt_npi_recv(unsigned char *data, int len);

int sipc_is_opened = 0;

static inline int bt_hwdec_write_word(unsigned int word)
{
  unsigned int *hwdec_addr = (unsigned int *)SPRD_DP_DMA_UARD_SDIO_BUFFER_BASE_ADDR;
  *hwdec_addr = word;
  return WORD_ALIGN;
}

static void recv_callback(int ch)
{
	BT_LOG("recv_callback: %d\n", ch);
	if(ch == SMSG_CH_BT)
		k_sem_give(&event_sem);
}

static void rx_thread(void)
{
	int ret;

	while (1) {
		BT_LOG("wait for data\n");
		k_sem_take(&event_sem, K_FOREVER);
		struct sblock blk;
		ret = sblock_receive(0, SMSG_CH_BT, &blk, 0);
		if (ret < 0) {
			BT_LOG("sblock recv error");
			continue;
		}

		BT_HCIDUMP("<- ", blk.addr, blk.length);

		if(0 != check_bteut_ready()){
			bt_npi_recv((unsigned char*)blk.addr,blk.length);
			goto rx_continue;
		}

rx_continue:;
		sblock_release(0, SMSG_CH_BT, &blk);
	}
}

int bt_sipc_send(unsigned char *data, int len)
{
	unsigned int *align_p, value;
	unsigned char *p;
	int i, word_num, remain_num;

	BT_HCIDUMP("-> ", data, len);

	if (len <= 0)
	  return len;

	word_num = len / WORD_ALIGN;
	remain_num = len % WORD_ALIGN;
	
	if (word_num) {
	  for (i = 0, align_p = (unsigned int *)data; i < word_num; i++) {
	  value = *align_p++;
	  bt_hwdec_write_word(value);
	  }
	}

	if (remain_num) {
	  value = 0;
	  p = (unsigned char *) &value;
	  for (i = len - remain_num; i < len; i++) {
		*p++ = *(data + i);
	  }
	  bt_hwdec_write_word(value);
	}

	return len;
}

int bt_sipc_open(void)
{
	BT_LOG("%s\n", __func__);

	if (1 == sipc_is_opened) {
		BT_LOG("%s, sipc has been opened!\n", __func__);
		return 0;
	}

	k_sem_init(&event_sem, 0, UINT_MAX);

	sblock_create(0, SMSG_CH_BT,BT_TX_BLOCK_NUM, BT_TX_BLOCK_SIZE,
				BT_RX_BLOCK_NUM, BT_RX_BLOCK_SIZE);

	sblock_register_callback(SMSG_CH_BT, recv_callback);

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack),
			(k_thread_entry_t)rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(8),
			0, K_NO_WAIT);

	sipc_is_opened = 1;
	return 0;
}
