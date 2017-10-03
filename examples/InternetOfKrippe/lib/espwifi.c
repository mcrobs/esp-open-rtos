/*
 * espwifi
 */
#include "espwifi.h"
#include <esp/uart.h>
#include <stdio.h>
#include <espressif/esp_common.h>
#include <espressif/esp_sta.h>
#include <dhcpserver.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <stdlib.h>

void wifiConfigSoftAP() {
	sdk_wifi_set_opmode(STATIONAP_MODE);
	struct sdk_softap_config* config = (struct sdk_softap_config*) calloc(1,
			sizeof(struct sdk_softap_config));
	sdk_wifi_station_set_auto_connect(false);
	sdk_wifi_station_disconnect();
	sdk_wifi_softap_get_config(config); // Get soft-AP config first.
	sprintf((char*) config->ssid, "pic");
	sprintf((char*) config->password, "123454321");
	config->authmode = AUTH_WPA_WPA2_PSK;
	config->ssid_len = 10; // or its actual SSID length
	config->max_connection = 4;
	sdk_wifi_softap_set_config(config); // Set ESP8266 soft-AP config

	free(config);
	struct ip_info info;
	IP4_ADDR(&info.ip, 192, 168, 33, 1); // set IP
	IP4_ADDR(&info.gw, 192, 168, 33, 1); // set gateway
	IP4_ADDR(&info.netmask, 255, 255, 255, 0); // set netmask
	sdk_wifi_set_ip_info(SOFTAP_IF, &info);
	ip_addr_t ip;
	IP4_ADDR(&ip, 192, 168, 33, 100);
	dhcpserver_start(&ip, 3);
}

void wifiSetStationMode() {
	sdk_wifi_set_opmode(STATION_MODE);
	sdk_wifi_station_set_auto_connect(true);
	sdk_wifi_station_connect();
}
//Callback the code calls when a wlan ap scan is done. Basically stores the result in
//the cgiWifiAps struct.
void wifiScanDoneCb(void *arg, sdk_scan_status_t status) {
	int n;
	struct sdk_bss_info *bss_link = (struct sdk_bss_info *) arg;
	// first one is invalid
	printf("wifiScanDoneCb %d\n", status);
	if (status != SCAN_OK) {
		cgiWifiAps.scanInProgress = 0;
		return;
	}

	//Clear prev ap data if needed.
	if (cgiWifiAps.apData != NULL) {
		for (n = 0; n < cgiWifiAps.noAps; n++)
			free(cgiWifiAps.apData[n]);
		free(cgiWifiAps.apData);
	}

	//Count amount of access points found.
	n = 0;
	while (bss_link != NULL) {
		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//Allocate memory for access point data
	cgiWifiAps.apData = (ApData **) malloc(sizeof(ApData *) * n);
	cgiWifiAps.noAps = n;
	printf("Scan done: found %d APs\n", n);

	//Copy access point data to the static struct
	n = 0;
	bss_link = (struct sdk_bss_info *) arg;
	while (bss_link != NULL) {
		if (n >= cgiWifiAps.noAps) {
			//This means the bss_link changed under our nose. Shouldn't happen!
			//Break because otherwise we will write in unallocated memory.
			printf("Huh? I have more than the allocated %d aps!\n",
					cgiWifiAps.noAps);
			break;
		}
		//Save the ap data.
		cgiWifiAps.apData[n] = (ApData *) malloc(sizeof(ApData));
		cgiWifiAps.apData[n]->rssi = bss_link->rssi;
		cgiWifiAps.apData[n]->enc = bss_link->authmode;
		strncpy(cgiWifiAps.apData[n]->ssid, (char*) bss_link->ssid, 32);
		printf("AP: %s", bss_link->ssid);
		bss_link = bss_link->next.stqe_next;
		n++;
	}
	printf("Scan stop");
	//We're done.
	cgiWifiAps.scanInProgress = 0;
}

//Routine to start a WiFi access point scan.
void wifiStartScan() {
//	int x;
	if (cgiWifiAps.scanInProgress)
		return;
	cgiWifiAps.scanInProgress = 1;
	//sdk_wifi_set_opmode(STATIONAP_MODE);
	sdk_wifi_station_scan(NULL, wifiScanDoneCb);
	//vTaskDelay(5000 / portTICK_PERIOD_MS);
}

//Routine to start a WiFi access point scan.
const char* wifiGetCurrentMode() {
	return auth_modes[sdk_wifi_get_opmode()];
}

const char* wifiGetConnectionStatus() {
	return conn_state[sdk_wifi_station_get_connect_status()];
}

uint8_t wifiIsConnected() {
	return sdk_wifi_station_get_connect_status() == STATION_GOT_IP;
}

void wifiGetCurrentSSID(char *str32) {
	struct sdk_station_config cfg;
	sdk_wifi_station_get_config(&cfg);
	strncpy(str32, (char*) cfg.ssid, 32);
}
void wifiGetCurrentPass(char *str64) {
	struct sdk_station_config cfg;

	sdk_wifi_station_get_config(&cfg);
	strncpy(str64, (char*) cfg.password, 64);
}
void wifiSetConnection(char* ssid, char* pass) {
	struct sdk_station_config cfg;
	printf("%s: Connecting to WiFi\n\r", __func__);
	//sdk_wifi_set_opmode(STATION_MODE);
	strncpy((char*) cfg.ssid, ssid, 32);
	strncpy((char*) cfg.password, pass, 64);
	sdk_wifi_station_set_config(&cfg);
	sdk_wifi_station_set_auto_connect(true);
	sdk_wifi_station_connect();
}
