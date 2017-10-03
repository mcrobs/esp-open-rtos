/*
 * espwifi.h
 *
 *  Created on: Apr 23, 2017
 *      Author: robby
 */
#include "espressif/esp_wifi.h"
#include "espressif/esp_sta.h"

#ifndef ESPWIFI_H_
#define ESPWIFI_H_


//WiFi access point data
typedef struct {
	char ssid[32];
	char rssi;
	char enc;
} ApData;

//Scan result
typedef struct {
	char scanInProgress; //if 1, don't access the underlying stuff from the webpage.
	ApData **apData;
	int noAps;
} ScanResultData;

static const char * const auth_modes[] = {[AUTH_OPEN] = "Open", [AUTH_WEP
	] = "WEP", [AUTH_WPA_PSK] = "WPA/PSK", [AUTH_WPA2_PSK] = "WPA2/PSK",
	[AUTH_WPA_WPA2_PSK] = "WPA/WPA2/PSK",[AUTH_MAX] = "AUTH/MAX"};

static const char * const conn_state[] = {[STATION_IDLE] = "IDLE",
	[STATION_CONNECTING] = "CONNECTING",
	[STATION_WRONG_PASSWORD] = "WRONG PASSWORD",
	[STATION_NO_AP_FOUND] = "NO AP FOUND",
	[STATION_CONNECT_FAIL] = "CONNECT FAIL",
	[STATION_GOT_IP] = "GOT IP"};

//Static scan status storage.
ScanResultData cgiWifiAps;
void wifiScanDoneCb(void *arg, sdk_scan_status_t status);
void wifiStartScan();
const char* wifiGetCurrentMode();
void wifiGetCurrentSSID(char* str32);
void wifiGetCurrentPass(char* str64);
void wifiSetStationMode();
void wifiConfigSoftAP();
uint8_t wifiIsConnected();
void wifiSetConnection(char* ssid, char* pass);
const char* wifiGetConnectionStatus();

#endif /* ESPWIFI_H_ */
