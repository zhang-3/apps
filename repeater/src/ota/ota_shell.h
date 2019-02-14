
#ifndef __OTA_SHELL__
#define __OTA_SHELL__

#include <zephyr.h>
#include <misc/printk.h>

#define OTA_ERR(fmt, ...)   printf("[ERR] "fmt"\n", ##__VA_ARGS__)
#define OTA_WARN(fmt, ...)  printf("[WARN] "fmt"\n", ##__VA_ARGS__)
#define OTA_INFO(fmt, ...)  printf("[INFO] "fmt"\n", ##__VA_ARGS__)

#define FLASH_AREA_BOOTLOADER                    0
#define FLASH_AREA_IMAGE_0                       1
#define FLASH_AREA_IMAGE_1                       2
#define FLASH_AREA_IMAGE_SCRATCH                 3

#define FLASH_KERNEL_ADDR       FLASH_AREA_IMAGE_1_OFFSET
#define FLASH_KERNEL_SIZE       FLASH_AREA_IMAGE_1_SIZE	/*512KB */

#define FLASH_MODEM_ADDR        (0x02300000ul - CONFIG_FLASH_BASE_ADDRESS)
#define FLASH_MODEM_SIZE        0xC0000	/*768KB */

#define FLASH_ERASE_BLOCK_SIZE	(64 * 1024)

/*"74.207.248.168" "139.196.142.65" */
#define OTA_SVR_ADDR    "192.168.1.102"
#define OTA_SVR_PORT    80

#define OTA_MODEM_BIN_URL    "/ota/wcn-modem-96b_ivy5661.bin"
#define OTA_KERNEL_BIN_URL   "/ota/zephyr-signed-ota-96b_ivy5661.bin"
#define OTA_TEST_BIN_URL     "/sites/default/files/banner/sc9863.jpg"

#define HTTP_RSP_STATS_NOT_FOUND	"Not Found"
#define HTTP_RSP_STATS_PART_CONT	"Partial Content"

#define OTA_COUNT_EACH_ONE      2048	/*64*1024 //64Kb */

#define OTA_MODEM_BIN_SIZE      618512	/*byte */
#define OTA_KERNEL_BIN_SIZE     377520	/*byte */
#define OTA_TEST_BIN_SIZE       147992

#define OTA_HTTP_REQ_TIMEOUT K_SECONDS(10)

#define OTA_HTTP_REPEAT_REQ_MAX 10


#endif /*__OTA_SHELL__*/
