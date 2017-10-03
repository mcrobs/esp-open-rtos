#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <FreeRTOS.h>
#include <task.h>
//#include <ssid_config.h>
#include <httpd/httpd.h>
#include <lwip/ip_addr.h>
#include <dhcpserver/include/dhcpserver.h>
#include "../InternetOfKrippe.h"
#include "../lib/espwifi.h"


#define LED_PIN 2

enum {
	SSI_UPTIME,
	SSI_FREE_HEAP,
	SSI_LED_STATE,
	JSON_SCANAP,
	SSI_APMODE,
	SSI_WIFISSID,
	SSI_WIFIPASS,
	SSI_PICURL,
	SSI_WIFISTATE
};

//Stupid li'l helper function that returns the value of a hex char.
static int httpdHexVal(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return 0;
}

//Decode a percent-encoded value.
//Takes the valLen bytes stored in val, and converts it into at most retLen bytes that
//are stored in the ret buffer. Returns the actual amount of bytes used in ret. Also
//zero-terminates the ret buffer.
int httpdUrlDecode(char *val, int valLen, char *ret, int retLen) {
	int s = 0, d = 0;
	int esced = 0, escVal = 0;
	while (s < valLen && d < retLen) {
		if (esced == 1) {
			escVal = httpdHexVal(val[s]) << 4;
			esced = 2;
		} else if (esced == 2) {
			escVal += httpdHexVal(val[s]);
			ret[d++] = escVal;
			esced = 0;
		} else if (val[s] == '%') {
			esced = 1;
		} else if (val[s] == '+') {
			ret[d++] = ' ';
		} else {
			ret[d++] = val[s];
		}
		s++;
	}
	if (d < retLen)
		ret[d] = 0;
	return d;
}

char *gpio_cgi_handler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {
	for (int i = 0; i < iNumParams; i++) {
		if (strcmp(pcParam[i], "on") == 0) {
			uint8_t gpio_num = atoi(pcValue[i]);
			gpio_enable(gpio_num, GPIO_OUTPUT);
			gpio_write(gpio_num, true);
		} else if (strcmp(pcParam[i], "off") == 0) {
			uint8_t gpio_num = atoi(pcValue[i]);
			gpio_enable(gpio_num, GPIO_OUTPUT);
			gpio_write(gpio_num, false);
		} else if (strcmp(pcParam[i], "toggle") == 0) {
			uint8_t gpio_num = atoi(pcValue[i]);
			gpio_enable(gpio_num, GPIO_OUTPUT);
			gpio_toggle(gpio_num);
		}
	}
	return "/index.shtml";
}

char *reboot_cgi_handler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {
	uint32_t rtcmem = RTC_EINK_CONFMODE;
	for (int i = 0; i < iNumParams; i++) {
		if (strcmp(pcParam[i], "mode") == 0) {
			uint8_t mode = atoi(pcValue[i]);
			switch (mode) {
			case 1:
				printf("rtc set next reboot is RTC_EINK_PICMODE\n");
				rtcmem = RTC_EINK_PICMODE;
				break;
			case 2:
				printf("rtc set next reboot is RTC_EINK_TESTMODE\n");
				rtcmem = RTC_EINK_TESTMODE;
				break;
			case 3:
				printf("rtc set next reboot is RTC_EINK_SUPERTESTMODE\n");
				rtcmem = RTC_EINK_SUPERTESTMODE;
				break;
			default:
				printf("rtc set next reboot is RTC_EINK_CONFMODE\n");
				rtcmem = RTC_EINK_CONFMODE;
				break;
			}
		}
	}
	sdk_system_rtc_mem_write(RTC_EINK_MEMPOS, &rtcmem, 4);
	sdk_system_restart();
	return "/index.shtml";
}

char *admin_cgi_handler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {
	for (int i = 0; i < iNumParams; i++) {
	}
	return "/admin.shtml";
}

char *connect_cgi_handler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {

	char pass[64];
	char ssid[32];
	for (int i = 0; i < iNumParams; i++) {
		if (strcmp(pcParam[i], "passwd") == 0) {
			printf("passwd:%s\n", pcValue[i]);
			httpdUrlDecode(pcValue[i], (strlen(pcValue[i])) * sizeof(char),
					pass, (strlen(pcValue[i]) + 1) * sizeof(char));
		} else if (strcmp(pcParam[i], "essid") == 0) {
			httpdUrlDecode(pcValue[i], (strlen(pcValue[i])) * sizeof(char),
					ssid, (strlen(pcValue[i]) + 1) * sizeof(char));
			printf("essid:%s\n", pcValue[i]);
		}
	}
	printf("Connection SSID:%s, PASS:%s\n", ssid, pass);
	wifiSetConnection(ssid, pass);
	return "/connect.shtml";
}

char *about_cgi_handler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {
	return "/about.html";
}

char *wifiscan_cgi_handler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {
	return "/wifiscan.js";
}

void scanForAPs(int32_t iInsertLen, char* pcInsert, u16_t current_tag_part,
		u16_t *next_tag_part) {
	int i;
	printf(
			"StartScanForAPs: iInsertLen:%d, current_tag_part:%d, next_tag_part:%d\n",
			iInsertLen, current_tag_part, *next_tag_part);
	if (cgiWifiAps.apData == NULL)
		cgiWifiAps.noAps = 0;

	if (cgiWifiAps.scanInProgress == 1) {
		//We're still scanning. Tell Javascript code that.
		snprintf(pcInsert, iInsertLen,
				"{\n \"result\": { \n\"inProgress\": \"1\"\n }\n}\n");
	} else {
		if (current_tag_part == 0) {
			snprintf(pcInsert, iInsertLen,
					"{\n \"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
			*next_tag_part = 1;
		} else {
			i = current_tag_part - 1;
			if (i < cgiWifiAps.noAps) {
				//Fill in json code for an access point
				snprintf(pcInsert, iInsertLen,
						"{\"essid\": \"%s\", \"rssi\": \"%d\", \"enc\": \"%d\"}%s\n",
						cgiWifiAps.apData[i]->ssid, cgiWifiAps.apData[i]->rssi,
						cgiWifiAps.apData[i]->enc,
						(i == cgiWifiAps.noAps - 1) ? "" : ",");
				printf("Accesspoint:%s %d from %d\n",
						cgiWifiAps.apData[i]->ssid, i + 1, cgiWifiAps.noAps);
				// count one up
				*next_tag_part = current_tag_part + 1;
			} else {
				// i == cgiWifiAps.noAp then close json object
				snprintf(pcInsert, iInsertLen, "]\n}\n}\n");
				wifiStartScan();
				// do not count next_tag_part this is the last one

			}
		}
	}
}

int32_t ssi_handler(int32_t iIndex, char *pcInsert, int32_t iInsertLen,
		u16_t current_tag_part, u16_t *next_tag_part) {
	char s[64];

	switch (iIndex) {
	case SSI_UPTIME:
		snprintf(pcInsert, iInsertLen, "%d",
				xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
		break;
	case SSI_FREE_HEAP:
		snprintf(pcInsert, iInsertLen, "%d", (int) xPortGetFreeHeapSize());
		break;
	case SSI_LED_STATE:
		snprintf(pcInsert, iInsertLen,
				(GPIO.OUT & BIT(LED_PIN)) ? "Off" : "On");
		break;
	case JSON_SCANAP:
		scanForAPs(iInsertLen, pcInsert, current_tag_part, next_tag_part);
		break;
	case SSI_APMODE:
		snprintf(pcInsert, iInsertLen, "%s", wifiGetCurrentMode());
		break;
	case SSI_WIFISSID:
		wifiGetCurrentSSID(s);
		snprintf(pcInsert, iInsertLen, "%s", s);
		break;
	case SSI_WIFIPASS:
		wifiGetCurrentPass(s);
		snprintf(pcInsert, iInsertLen, "%s", s);
		break;
	case SSI_WIFISTATE:
		snprintf(pcInsert, iInsertLen, "%s", wifiGetConnectionStatus());
		break;
	default:
		snprintf(pcInsert, iInsertLen, "N/A");
		break;
	}

	/* Tell the server how many characters to insert */
	return (strlen(pcInsert));
}

void httpd_task(void *pvParameters) {
	printf("+++++++++++++++++++++++++++++\n");
	printf("++++++    HTTP TASK     +++++\n");
	printf("+++++++++++++++++++++++++++++\n");
	vTaskDelay(100);
	wifiConfigSoftAP();
	uint32_t rtcmagic;
	tCGI pCGIs[] = { { "/", (tCGIHandler) gpio_cgi_handler }, { "/admin",
			(tCGIHandler) admin_cgi_handler }, { "/gpio",
			(tCGIHandler) gpio_cgi_handler }, { "/about",
			(tCGIHandler) about_cgi_handler }, { "/wifiscan.js",
			(tCGIHandler) wifiscan_cgi_handler }, { "/connect.shtml",
			(tCGIHandler) connect_cgi_handler }, { "/reboot",
			(tCGIHandler) reboot_cgi_handler }, };

	const char *pcConfigSSITags[] = { "uptime", // SSI_UPTIME
			"heap",   // SSI_FREE_HEAP
			"led",     // SSI_LED_STATE
			"scanap", // JSON_SCANAP
			"wifimode", "wifissid", "wifipass", "picurl", "wifistat" };

	/* register handlers and start the server */
	http_set_cgi_handlers(pCGIs, sizeof(pCGIs) / sizeof(pCGIs[0]));
	http_set_ssi_handler((tSSIHandler) ssi_handler, pcConfigSSITags,
			sizeof(pcConfigSSITags) / sizeof(pcConfigSSITags[0]));
	httpd_init();
	int cnt = 30;
	while (cnt--) {
		sdk_system_rtc_mem_read(128, &rtcmagic, 4);
		if (wifiIsConnected() && (rtcmagic != RTC_EINK_PICMODE)) {
			uint32_t rtcmem = RTC_EINK_PICMODE;
			sdk_system_rtc_mem_write(RTC_EINK_MEMPOS, &rtcmem, 4);
			printf("RTC set next reboot is RTC_EINK_PICMODE\n");
		}
		printf("reboot in %d ...", cnt);
		taskDelayInSec(10);
	}
	printf("+++ HTTPD END +++\nREBOOT in \n\n");
	uint32_t rtcmem = RTC_EINK_PICMODE;
	sdk_system_rtc_mem_write(RTC_EINK_MEMPOS, &rtcmem, 4);
	sdk_system_restart();
}
