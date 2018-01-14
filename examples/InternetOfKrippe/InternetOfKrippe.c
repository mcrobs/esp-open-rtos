#include "FreeRTOSConfig.h"
#include "http_server/http_server.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "InternetOfKrippe.h"
#include "lib/espwifi.h"
#include "lib/DFRobotDFPlayerMini.h"
#include "task.h"
#include "queue.h"
#include "string.h"

uint32_t initRTC() {
	uint32_t rtc_einkmode;
	sdk_system_rtc_mem_read(RTC_EINK_MEMPOS, &rtc_einkmode, 4);
	switch (rtc_einkmode) {
	case RTC_EINK_CONFMODE:
	case RTC_EINK_PICMODE:
	case RTC_EINK_SUPERTESTMODE:
	case RTC_EINK_TESTMODE:
	case RTC_EINK_BATEMP_MODE:
		break;
	default:
		rtc_einkmode = RTC_EINK_CONFMODE;
		sdk_system_rtc_mem_write(RTC_EINK_MEMPOS, &rtc_einkmode, 4);
		break;
	}
	sdk_system_rtc_mem_read(RTC_EINK_MEMPOS, &rtc_einkmode, 4);
	return rtc_einkmode;
}

void user_init(void) {
	uart_set_baud(0, 115200);
	uint32_t rtc_einkmode = initRTC();
	printf("SDK version:%s\n", sdk_system_get_sdk_version());
	testDFPlayer();

/*	if (rtc_einkmode == RTC_EINK_BATEMP_MODE) {
		//                      2147483648
		sdk_system_deep_sleep(30 * 1000000); //sleep time in usecs. 10000000 = 10 secs
	}
	if (rtc_einkmode == RTC_EINK_CONFMODE) {
		xTaskCreate(httpd_task, "HTTPD", 1024, NULL, 2, NULL);
	}
*/
}
