/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_TEST_BQB_H__
#define __BT_TEST_BQB_H__

#define BQB_MODE_ON ("AT+SPBQBTEST=1")
#define BQB_MODE_OFF ("AT+SPBQBTEST=0")

#define PREAMBLE_BUFFER_SIZE 5
#define PACKET_TYPE_TO_INDEX(type) ((type) - 1)
#define HCI_COMMAND_PREAMBLE_SIZE 3
#define HCI_ACL_PREAMBLE_SIZE 4
#define HCI_SCO_PREAMBLE_SIZE 3
#define HCI_EVENT_PREAMBLE_SIZE 2
#define RETRIEVE_ACL_LENGTH(preamble) ((((preamble)[4] & 0xFFFF) << 8) | (preamble)[3])
#define HCI_HAL_SERIAL_BUFFER_SIZE 1026

#define BYTE_ALIGNMENT 8

#define STREAM_TO_UINT8(u8, p)   {u8 = (uint8_t)(*(p)); (p) += 1;}

typedef enum {
	BRAND_NEW,
	PREAMBLE,
	BODY,
	IGNORE,
	FINISHED
}receive_state_t;

typedef enum {
	DATA_TYPE_COMMAND = 1,
	DATA_TYPE_ACL     = 2,
	DATA_TYPE_SCO     = 3,
	DATA_TYPE_EVENT   = 4
} serial_data_type_t;

typedef enum {
	BQB_CLOSED = 1,
	BQB_OPENED,
} bt_bqb_state_t;

typedef struct {
	receive_state_t state;
	uint16_t bytes_remaining;
	uint8_t type;
	uint8_t preamble[PREAMBLE_BUFFER_SIZE];
	uint16_t index;
	uint8_t buffer[HCI_HAL_SERIAL_BUFFER_SIZE + BYTE_ALIGNMENT];
} packet_receive_data_t;

typedef int (*frame_complete_cb)(unsigned char *data, int len);

int bqb_enable(struct device *uart);
int bqb_disable(void);
void bqb_userial_data_handle(unsigned char *buf, int len);
int get_bqb_state(void);
void bqb_recv_cb(unsigned char *data, int len);
#endif
