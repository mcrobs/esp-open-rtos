/*
 * Softuart example
 *
 * Copyright (C) 2017 Ruslan V. Uss <unclerus@gmail.com>
 * Copyright (C) 2016 Bernhard Guillon <Bernhard.Guillon@web.de>
 * Copyright (c) 2015 plieningerweb
 *
 * MIT Licensed as described in the file LICENSE
 */
#include <esp/gpio.h>
#include <esp/uart.h>
#include <espressif/esp_common.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include "DFRobotDFPlayerMini.h"
#include <softuart/softuart.h>

#define RX_PIN 5 // D1
#define TX_PIN 4 // D2
#define DATA_PIN 12 // D6

const char MP3_PLAY[] = { 0x7E, 0xFF, 0x06, 0x0D, 0x00, 0x00, 0x00, 0xFE, 0xEE, 0xEF };
const char MP3_QUERRY_STATUS[] = { 0x7E, 0xFF, 0x06, 0x42, 0x00, 0x00, 0x00, 0xFE, 0xB9, 0xEF };
const char MP3_TEST00[] = { 0x03, 0x02, 0x01, 0x00, 0x01, 0x02, 0x03 };
const char MP3_TESTFF[] = { 0x03, 0x02, 0x01, 0xFF, 0x01, 0x02, 0x03 };
const char MP3_TESTAA[] = { 0x03, 0x02, 0x01, 0xAA, 0x01, 0x02, 0x03 };
const char MP3_TEST10[] = { 0xFF, 0xee, 0xdd, 0xcc, 0xaa, 0x00, 0xff, 0x01, 0x02, 0x03  };

bool send_sbin(const char* s, int bytes) {
	printf("TX %d :", bytes);
	while (bytes--) {
		printf("[%02X] ", *s);
		if (!softuart_put(0, *s++))
			return false;
	}
	printf("\n");
	return true;
}

bool send_cmd(const char* s) {
	return send_sbin(s, 10);
}

void user_initx(void) {
	// setup real UART for now
	uart_set_baud(0, 115200);
	printf("SDK version:%s\n\n", sdk_system_get_sdk_version());

	// setup software uart to 9600 8n1
	if (!softuart_open(0, 9600, RX_PIN, TX_PIN)) {
		printf("ERROR OPENING SOFTUART 0\n");
	}

	while (true) {
		if (!softuart_available(0))
			continue;

		char c = softuart_read(0);
		printf("input: %c, 0x%02x\n", c, c);
		softuart_puts(0, "start\r\n");
	}
}

void wait_sec(int sec) {
	vTaskDelay(sec * 1000 / portTICK_PERIOD_MS);
}
void user_init(void) {
	// setup real UART for now
	uart_set_baud(0, 115200);
	printf("SDK version:%s\n\n", sdk_system_get_sdk_version());
	gpio_enable(DATA_PIN, GPIO_OUTPUT);
	gpio_write(DATA_PIN, 0);

	// setup software uart to 9600 8n1
	//softuart_open(0, 9600, RX_PIN, TX_PIN);
	mp3_set_serial(0, 9600, RX_PIN, TX_PIN);

	if (empty_buffer())
		printf("Buffer empty\n");

	int volume = 30;
	while (1) {
		char c = getchar();

		if (c == 'o') {
			printf("ON\nBOOT ");
			gpio_write(DATA_PIN, 1);
			for (int t = 2; t >= 1; t--) {
				printf("%d ", t);
				wait_sec(1);
			}
			printf("0\n");

			mp3_send_cmd3(0x3f, 0, 0);
			printf("MODULE MP3 READY\n\n");
		}
		if (c == '0') {
			printf("OFF\n");
			gpio_write(DATA_PIN, 0);
		}
		if (c == '-') {
			volume--;
			mp3_set_volume(volume);
		}
		if (c == '+') {
			volume++;
			mp3_set_volume(volume);
		}
		if (c == '1') {
			send_sbin(MP3_TEST00, 7);
		}
		if (c == '2') {
			send_sbin(MP3_TESTFF, 7);
		}
		if (c == '3') {
			send_sbin(MP3_TESTAA, 7);
		}
		if (c == '4') {
			send_sbin(MP3_TEST10, 10);
		}
		if (c == 'x') {
			send_cmd(MP3_QUERRY_STATUS);
		}
		if (c == 'p') {
			send_cmd(MP3_PLAY);
		}
		mp3_recv_int_cmd();
		//if (empty_buffer())
		//	printf("Buffer empty\n");

	}
}
