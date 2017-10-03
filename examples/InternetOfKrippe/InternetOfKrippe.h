/*
 * eink.h
 *
 *  Created on: Apr 30, 2017
 *      Author: robby
 */
#include "task.h"
#include "FreeRTOS.h"

#ifndef EINK_H_
#define EINK_H_

#define taskDelayInSec(delay) vTaskDelay (delay * 1000/ portTICK_PERIOD_MS)

#define RTC_EINK_PICMODE 0xCFFED00D
#define RTC_EINK_CONFMODE 0xDADAAFFE
#define RTC_EINK_TESTMODE 0xAFFE1234
#define RTC_EINK_SUPERTESTMODE 0x12345678
#define RTC_EINK_BATEMP_MODE 0xBADDEB0D

#define RTC_EINK_MEMPOS 128



/*
 * Bin file creates a header of EINK_HEADER_SIZE
 *
 * Bytes
 * 0       |12        | 3456     |
 * Version |Sleeptime | RTC MODE |
 */
#define EINK_PARAM_MAGICNUMBER 0x90ABCDEF

struct einkparameter {
	uint32_t magicnumber;
	uint8_t version;
	uint16_t sleep;
	uint32_t rtcmode;
};

#define EINK_HEADER_SIZE sizeof(struct einkparameter)

#endif /* EINK_H_ */
