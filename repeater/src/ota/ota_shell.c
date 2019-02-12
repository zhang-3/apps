
/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_MODULE_NAME repeater
#define LOG_LEVEL CONFIG_REPEATER_LOG_LEVEL
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <stdio.h>
#include <version.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/http.h>
#include <net/net_app.h>

#include <flash.h>
#include <device.h>

#include "ota_shell.h"

#define DOWNLOAD_STACK_SIZE 500
#define DOWNLOAD_PRIORITY 5

/*
static struct k_thread do_download_data;
static K_THREAD_STACK_DEFINE(do_download_stack, DOWNLOAD_STACK_SIZE);
*/

#define WAIT_TIME (OTA_HTTP_REQ_TIMEOUT * 2)

/*
 * Note that the http_ctx is quite large so be careful if that is
 * allocated from stack.
 */
static struct http_ctx http_ctx;
static int recv_count;
static char *download_url = OTA_KERNEL_BIN_URL;
static int req_count;
static unsigned long int flash_addr;

/* This will contain the returned HTTP response to a sent request */
#if !defined(RESULT_BUF_SIZE)
#define RESULT_BUF_SIZE 4096
#endif
static u8_t result[RESULT_BUF_SIZE];

#define BIN_BUF_SIZE (10 * 1024)
static u8_t bin_buff[BIN_BUF_SIZE];

static struct device *flash_device;

#define OTA_IMAGE_OK_OFFS(bank_offs) \
			(bank_offs + FLASH_AREA_IMAGE_0_SIZE - 16 - 8)
#define OTA_MAGIC_OFFS(bank_offs) \
			(bank_offs + FLASH_AREA_IMAGE_0_SIZE - 16)

struct waiter {
	struct http_ctx *ctx;
	struct k_sem wait;
	size_t total_len;
	size_t header_len;
};

/* blank check when erased flash */
static int do_checkblank(off_t offset, size_t len)
{
	u8_t temp[64];
	int count = len;
	int read_size = 0;
	int read_addr = offset;

	OTA_INFO("check Addr=0x%08X, len=%d.\n", (unsigned int)offset, len);
	while (count > 0) {
		if (count > sizeof(temp)) {
			read_size = sizeof(temp);
			count -= read_size;
		} else {
			read_size = count;
			count = 0;
		}

		memset(temp, 0, sizeof(temp));
		flash_read(flash_device, read_addr, temp, read_size);

		for (int i = 0; i < read_size; i++) {
			if (temp[i] != 0xff) {
				OTA_ERR("Blank Check Failed, Addr(0x%08X)=%d",
					(read_addr + i), temp[i]);
				return 0;
			}
		}

		read_addr += read_size;
	}

	return 1;
}

/* Erase area, handling write protection and printing on error. */
static int do_erase(off_t offset, size_t size)
{
	int ret = 0;
	int one_size;
	int count = size;
	int offset_temp = offset;

	while (count) {
		if (size > FLASH_ERASE_BLOCK_SIZE) {
			one_size = FLASH_ERASE_BLOCK_SIZE;
		} else {
			one_size = size;
		}
		ret = flash_write_protection_set(flash_device, false);
		if (ret) {
			OTA_ERR("Disable flash Protection Err:%d.\n", ret);
			return ret;
		}

		OTA_INFO("Erase Addr=0x%08X, size=%d.\n",
			 (unsigned int)offset_temp, one_size);
		ret = flash_erase(flash_device, offset_temp, one_size);
		if (ret) {
			OTA_ERR("flash_erase failed (err:%d).\n", ret);
			return ret;
		}

		ret = flash_write_protection_set(flash_device, true);
		if (ret) {
			OTA_ERR("Enable flash Protection Err:%d.\n", ret);
		}

		count -= one_size;
		offset_temp += one_size;
	}

	ret = do_checkblank(offset, size);
	return ret;
}

/* Write bytes, handling write protection and printing on error. */
static int do_write(off_t offset, u8_t *buf, size_t len, bool read_back)
{
	int ret;
	u8_t *read_buf = NULL;

	ret = flash_write_protection_set(flash_device, false);
	if (ret) {
		OTA_ERR("Failed to disable flash protection (err: %d).\n", ret);
		return ret;
	}
	OTA_INFO("Write Addr=0x%08X.\n", (unsigned int)offset);

	ret = flash_write(flash_device, offset, buf, len);
	if (ret) {
		OTA_ERR("flash_write failed (err:%d).\n", ret);
		return ret;
	}
	ret = flash_write_protection_set(flash_device, true);
	if (ret) {
		OTA_ERR("Failed to enable flash protection (err: %d).\n", ret);
		return ret;
	}
	if (read_back) {
		read_buf = malloc(len);
		if (read_buf == NULL) {
			OTA_ERR
			    ("Read back check failed! Can't malloc buffer\n");
			return -1;
		}
		OTA_INFO("Reading back written bytes:%d\n", len);
		memset(read_buf, 0, len);
		flash_read(flash_device, offset, read_buf, len);
		for (int i = 0; i < len; i++) {
			if (read_buf[i] != buf[i]) {
				OTA_ERR
				    ("ERR: i=%d,write:%d,read:%d\n",
				     i, buf[i], read_buf[i]);
				ret = -1;
				break;
			}
		}
		free(read_buf);
	}
	return ret;
}

static void
http_received(struct http_ctx *ctx,
	      struct net_pkt *pkt,
	      int status,
	      u32_t flags, const struct sockaddr *dst, void *user_data)
{
	if (!status) {
		if (pkt) {
			OTA_INFO("Received %d bytes data",
				 net_pkt_appdatalen(pkt));
			net_pkt_unref(pkt);
		}
	} else {
		OTA_ERR("Receive error (%d)", status);

		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

void
response(struct http_ctx *ctx,
	 u8_t *data, size_t buflen,
	 size_t datalen, enum http_final_call data_end, void *user_data)
{
	struct waiter *waiter = user_data;
	int ret;

	if (data_end == HTTP_DATA_MORE) {
		OTA_INFO("Received %zd bytes piece of data", datalen);

		/* Do something with the data here. For this example
		 * we just ignore the received data.
		 */
		waiter->total_len += datalen;

		if (ctx->http.rsp.body_start) {
			/* This fragment contains the start of the body
			 * Note that the header length is not proper if
			 * the header is spanning over multiple recv
			 * fragments.
			 */
			waiter->header_len = ctx->http.rsp.body_start -
			    ctx->http.rsp.response_buf;
		}

		return;
	}

	waiter->total_len += datalen;

	OTA_INFO("HTTP server resp status: %s", ctx->http.rsp.http_status);

	if (ctx->http.parser.http_errno) {
		if (ctx->http.req.method == HTTP_OPTIONS) {
			/* Ignore error if OPTIONS is not found */
			goto out;
		}

		OTA_INFO("HTTP parser status: %s",
			 http_errno_description(ctx->http.parser.http_errno));
		ret = -EINVAL;
		goto out;
	}

	if (ctx->http.req.method != HTTP_HEAD &&
	    ctx->http.req.method != HTTP_OPTIONS) {
		if (ctx->http.rsp.body_found) {
			OTA_INFO("HTTP body: %zd bytes, expected: %zd bytes",
				 ctx->http.rsp.processed,
				 ctx->http.rsp.content_length);
		} else {
			OTA_ERR("Error detected during HTTP msg processing");
		}

		if (waiter->total_len !=
		    waiter->header_len + ctx->http.rsp.content_length) {
			OTA_ERR("Error while receiving data, "
				"received %zd expected %zd bytes",
				waiter->total_len, waiter->header_len +
				ctx->http.rsp.content_length);
		}
	}

out:
	k_sem_give(&waiter->wait);
}

#ifdef DO_ASYNC_HTTP_REQ
static int
do_async_http_req(struct http_ctx *ctx,
		  enum http_method method,
		  const char *url,
		  const char *payload, int index_start, int index_end)
{
	struct http_request reqs = { };
	struct waiter waiter;
	char temp[200];
	int ret;

	reqs.method = method;
	reqs.url = url;
	reqs.protocol = " " HTTP_PROTOCOL;

	k_sem_init(&waiter.wait, 0, 1);

	waiter.total_len = 0;

	memset(temp, 0, sizeof(temp));
	sprintf(temp,
		"Range: bytes=%d-%d\r\nUser-Agent:curl/7.58.0\r\nAccept:*/*\r\n",
		index_start, index_end);

	reqs.header_fields = temp;

	ret = http_client_send_req(ctx, &reqs, response, result, sizeof(result),
				   &waiter, OTA_HTTP_REQ_TIMEOUT);

	if (ret < 0 && ret != -EINPROGRESS) {
		OTA_ERR("Cannot send %s request (%d)", http_method_str(method),
			ret);
		goto out;
	}

	if (k_sem_take(&waiter.wait, WAIT_TIME)) {
		OTA_ERR("Timeout while waiting HTTP response");
		ret = -ETIMEDOUT;
		http_request_cancel(ctx);
		goto out;
	}

	ret = 0;

out:
	http_close(ctx);

	return ret;
}
#endif

static int
do_sync_http_req(struct http_ctx *ctx,
		 enum http_method method,
		 const char *url,
		 const char *content_type,
		 const char *payload, int index_start, int index_end)
{
	struct http_request req = { };
	char temp[200];
	int ret;
	const char *status;

	req.method = method;
	req.url = url;
	req.protocol = " " HTTP_PROTOCOL;
	status = http_errno_description(ctx->http.parser.http_errno);

	memset(temp, 0, sizeof(temp));
	sprintf(temp,
		"Range: bytes=%d-%d\r\nUser-Agent:curl/7.58.0\r\nAccept:*/*\r\n",
		index_start, index_end);

	/*add Range field with http request */
	req.header_fields = temp;

	ret = http_client_send_req(ctx, &req, NULL, result, sizeof(result),
				   NULL, OTA_HTTP_REQ_TIMEOUT);
	if (ret < 0) {
		OTA_ERR("Cannot send %s request (%d)\n",
			http_method_str(method), ret);
		goto out;
	}

	if (!memcmp(ctx->http.rsp.http_status, HTTP_RSP_STATS_NOT_FOUND,
		    strlen(HTTP_RSP_STATS_NOT_FOUND))) {
		OTA_ERR("Http Response: Not Found\n");
		ret = -ENOENT;
		goto out;
	}

	if (ctx->http.rsp.data_len > sizeof(result)) {
		OTA_ERR("Result buffer overflow by %zd bytes\n",
			ctx->http.rsp.data_len - sizeof(result));

		ret = -E2BIG;
	} else {
		if (ctx->http.parser.http_errno) {
			if (method == HTTP_OPTIONS) {
				/* Ignore error if OPTIONS is not found */
				goto out;
			}

			ret = -EINVAL;
			goto out;
		}

		if (method != HTTP_HEAD) {
			if (ctx->http.rsp.body_found) {
				recv_count += ctx->http.rsp.processed;
				if (ctx->http.rsp.processed ==
				    ctx->http.rsp.content_length) {
					memcpy(&bin_buff
					       [index_start % BIN_BUF_SIZE],
					       ctx->http.rsp.body_start,
					       ctx->http.rsp.processed);
				}
			} else {
				ret = -ENOPROTOOPT;
				OTA_ERR("Error detected during HTTP msg "
					"processing.\n");
			}
		}
	}

out:
	http_close(ctx);

	return ret;
}

static int do_download(char *url, int file_length)
{
	int req_count_start = 0;
	int req_count_end = 0;
	int repeate_count = 0;
	int write_size = 0;

	/*init http client */
	int ret = http_client_init(&http_ctx, OTA_SVR_ADDR, OTA_SVR_PORT, NULL,
				   K_FOREVER);

	if (ret < 0) {
		OTA_ERR("HTTP init failed (%d)", ret);
		goto out;
	}

	/*http registe the calllback */
	http_set_cb(&http_ctx, NULL, http_received, NULL, NULL);

	/*request the file from servers and write to flash */
	while (req_count > 0) {
		if (req_count > OTA_COUNT_EACH_ONE) {
			req_count_end =
			    req_count_start + OTA_COUNT_EACH_ONE - 1;
			req_count -= OTA_COUNT_EACH_ONE;
			write_size += OTA_COUNT_EACH_ONE;
		} else {
			req_count_end = req_count_start + req_count - 1;
			write_size += req_count;
			req_count = 0;	/*the last request */
		}

repeate:
		OTA_INFO("req count:req_count_start=%d req_count_end=%d.\n",
			 req_count_start, req_count_end);

#ifdef DO_ASYNC_HTTP_REQ
		ret = do_async_http_req(&http_ctx, HTTP_GET, download_url, NULL,
					req_count_start, req_count_end);
#else
		ret = do_sync_http_req(&http_ctx, HTTP_GET, download_url,
				       NULL, NULL, req_count_start,
				       req_count_end);
#endif

		if (ret < 0) {

			repeate_count++;
			OTA_ERR("HTTP do_async_http_req failed (%d).\n", ret);
			if (repeate_count >= OTA_HTTP_REPEAT_REQ_MAX) {
				goto out;
			}
			k_sleep(500);
			goto repeate;
		}

		/*got BIN_BUF_SIZE, so need to write flash */
		if ((0 == (req_count_end + 1) % BIN_BUF_SIZE)
		    || (req_count == 0)) {
			OTA_INFO
			    ("Now, Start to write flash, startAddr=0x%08X\n",
			     (unsigned int)flash_addr);

			do_write(flash_addr, bin_buff, write_size, true);

			flash_addr = flash_addr + BIN_BUF_SIZE;
			write_size = 0;
			memset(bin_buff, 0, sizeof(bin_buff));
		}

		k_sleep(200);
		req_count_start = req_count_end + 1;
		repeate_count = 0;
	}
	ret = 1;
out:
	http_release(&http_ctx);

	OTA_INFO(" Download Done!\n");

	return ret;
}

static int ota_set_pending(void)
{
	u32_t off = OTA_MAGIC_OFFS(FLASH_AREA_IMAGE_1_OFFSET);
	u32_t boot_img_magic_1[4] = {
		0xf395c277,
		0x7fefd260,
		0x0f505235,
		0x8079b62c,
	};

	do_write(off, (u8_t *) boot_img_magic_1, 16, true);
	return 0;
}

static int ota_download(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int err = shell_cmd_precheck(shell, (argc == 2), NULL, 0);

	if (err) {
		shell_fprintf(shell, SHELL_ERROR,
			      "Invalid input,Please check it.\n");
		return err;
	}

	flash_device = device_get_binding(DT_FLASH_DEV_NAME);
	if (flash_device) {
		printk("Found flash device %s.\n", DT_FLASH_DEV_NAME);
		printk("Flash I/O commands can be run.\n");
	} else {
		printk("**No flash device found!**\n");
		return -1;
	}

	shell_fprintf(shell, SHELL_NORMAL,
		      "Got Download parameter is :%s\n", argv[1]);
	if (!strcmp(argv[1], "-k")) {
		req_count = OTA_KERNEL_BIN_SIZE;
		download_url = OTA_KERNEL_BIN_URL;
		flash_addr = FLASH_KERNEL_ADDR;
		do_erase(FLASH_KERNEL_ADDR, FLASH_KERNEL_SIZE);
	} else if (!strcmp(argv[1], "-m")) {
		req_count = OTA_MODEM_BIN_SIZE;
		download_url = OTA_MODEM_BIN_URL;
		flash_addr = FLASH_MODEM_ADDR;
		do_erase(FLASH_MODEM_ADDR, FLASH_MODEM_SIZE);

	} else if (!strcmp(argv[1], "-t")) {
		req_count = OTA_TEST_BIN_SIZE;
		download_url = OTA_TEST_BIN_URL;
		flash_addr = FLASH_KERNEL_ADDR;
		do_erase(FLASH_KERNEL_ADDR, FLASH_KERNEL_SIZE);
	} else {
		shell_fprintf(shell, SHELL_ERROR,
			      "Invalid input,Parametr is %d.\n", argv[1]);
		return err;
	}

	shell_fprintf(shell, SHELL_NORMAL, "download_url=%s\n", download_url);

	recv_count = 0;
	memset(bin_buff, 0, sizeof(bin_buff));

	ret = do_download(download_url, req_count);

	ota_set_pending();

	return 0;
}

static int ota_verify(const struct shell *shell, size_t argc, char **argv)
{
	unsigned long int check_flash_addr1, check_flash_addr2;
	int check_size = 32;
	char buf[check_size];

	int ret = shell_cmd_precheck(shell, (argc == 2), NULL, 0);

	if (ret) {
		shell_fprintf(shell, SHELL_ERROR,
			      "Invalid input,Please check it.\n");
		return ret;
	}

	/*check the parameter */
	if (!strcmp(argv[1], "-s0")) {
		check_flash_addr1 = FLASH_AREA_IMAGE_0_OFFSET;
		check_flash_addr2 =
		    FLASH_AREA_IMAGE_0_OFFSET + FLASH_KERNEL_SIZE - 32;
		OTA_INFO("Check slot0 Partition...\n");
	} else if (!strcmp(argv[1], "-s1")) {
		check_flash_addr1 = FLASH_AREA_IMAGE_1_OFFSET;
		check_flash_addr2 =
		    FLASH_AREA_IMAGE_1_OFFSET + FLASH_KERNEL_SIZE - 32;
		OTA_INFO("Check slot1 Partition...\n");
	} else {
		shell_fprintf(shell, SHELL_ERROR, "Invalid input.\n");
		return -1;
	}

	flash_device = device_get_binding(DT_FLASH_DEV_NAME);
	if (flash_device) {
		printk("Found flash device %s.\n", DT_FLASH_DEV_NAME);
		printk("Flash I/O commands can be run.\n");
	} else {
		printk("**No flash device found!**\n");
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	ret = flash_read(flash_device, check_flash_addr1, buf, sizeof(buf));

	OTA_INFO("addr1:");
	for (int i = 0; i < sizeof(buf); i++) {
		OTA_INFO(" %02X", buf[i]);
	}
	OTA_INFO("\n");

	memset(buf, 0, sizeof(buf));
	ret = flash_read(flash_device, check_flash_addr2, buf, sizeof(buf));

	OTA_INFO("addr2:");
	for (int i = 0; i < sizeof(buf); i++) {
		OTA_INFO(" %02X", buf[i]);
	}
	OTA_INFO("\n");

	return 0;
}

static int test_flash(const struct shell *shell, size_t argc, char **argv)
{
	char buf[16];
	unsigned long int test_flash_addr = FLASH_AREA_IMAGE_1_OFFSET;

	/*init flash */
	flash_device = device_get_binding(DT_FLASH_DEV_NAME);
	if (flash_device) {
		printk("Found flash device %s.\n", DT_FLASH_DEV_NAME);
		printk("Flash I/O commands can be run.\n");
	} else {
		printk("**No flash device found!**\n");
		return -1;
	}
	shell_fprintf(shell, SHELL_NORMAL, "test_flash_addr=%08X\n",
		      test_flash_addr);
	/*erase flash */
	do_erase(FLASH_AREA_IMAGE_1_OFFSET, FLASH_AREA_IMAGE_1_SIZE);

	memset(buf, 0x55, sizeof(buf));
	/*write and check */
	do_write(test_flash_addr, buf, sizeof(buf), true);
	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(ota_cmd)
{
	SHELL_CMD(download, NULL,
	"download the firmware from internet,include kernel and modem.\n"
	"Usage: download <type> -k:download kernel -m:download modem",
	ota_download),
	SHELL_CMD(verify, NULL,
	"verify the bin code.\n"
	"Usage: download <type> -k:check kernel -m:check modem",
	ota_verify),
	SHELL_CMD(testflash, NULL,
	"test flash for writing and reading.",
	test_flash),
	SHELL_SUBCMD_SET_END	/* Array terminated. */
};

/* Creating root (level 0) command "app" */
SHELL_CMD_REGISTER(ota, &ota_cmd, "OTA with upgrade firmware", NULL);
