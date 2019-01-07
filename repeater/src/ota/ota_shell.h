
#ifndef __OTA_SHELL__
#define __OTA_SHELL__

#define FLASH_KERNEL_ADDR       (0x02180000ul - CONFIG_FLASH_BASE_ADDRESS)
#define FLASH_KERNEL_SIZE       0x80000	/*512KB */

#define FLASH_MODEM_ADDR        (0x02200000ul - CONFIG_FLASH_BASE_ADDRESS)
#define FLASH_MODEM_SIZE        0xC0000	/*768KB */

/*"192.168.1.102" "74.207.248.168" */
#define OTA_SVR_ADDR    "139.196.142.65"
#define OTA_SVR_PORT    80

#define OTA_MODEM_BIN_URL    "/ota/wcn-modem-96b_ivy5661.bin"
#define OTA_KERNEL_BIN_URL   "/ota/zephyr-signed-ota-96b_ivy5661.bin"
#define OTA_TEST_BIN_URL     "/sites/default/files/banner/sc9863.jpg"

#define OTA_COUNT_EACH_ONE      2048	/*64*1024 //64Kb */

#define OTA_MODEM_BIN_SIZE      618512	/*byte */
#define OTA_KERNEL_BIN_SIZE      221080	/*byte */
#define OTA_TEST_BIN_SIZE       147992

#define OTA_HTTP_REQ_TIMEOUT K_SECONDS(10)

#define OTA_HTTP_REPEAT_REQ_MAX 10

#endif /*__OTA_SHELL__*/
