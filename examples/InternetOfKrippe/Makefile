# Simple makefile for simple example
PROGRAM=InternetOfKrippe
PROGRAM_SRC_DIR=. ./lib ./http_server
PROGRAM_INC_DIR=. ./includes 
#    $(ROOT)include/espressif $(ROOT)include/espressif/esp8266 
#					$(ROOT)/lwip/lwip/espressif/include \
#					$(ROOT)/lwip/lwip/espressif/include/lwip/app \
#					$(ROOT)/

#$(ROOT)lwip/include $(lwip_ROOT)include $(LWIP_DIR)include/posix $(LWIP_DIR)include/ipv4 $(LWIP_DIR)include/ipv4/lwip $(LWIP_DIR)include/lwip

# Source components to compile and link. Each of these are subdirectories
# of the root, with a 'component.mk' file.
EXTRA_COMPONENTS = extras/softuart extras/spiffs extras/httpd extras/dhcpserver extras/mbedtls

COMPONENTS     ?= $(EXTRA_COMPONENTS) FreeRTOS core lwip open_esplibs


EXTRA_CFLAGS=-DLWIP_DEBUG=1 -DLWIP_HTTPD_SSI_MULTIPART=1 -DLWIP_HTTPD_MAX_TAG_INSERT_LEN=192 \
             -DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./http_server/fsdata 

FLASH_SIZE = 32

#ESPBAUD = 460800


# define esp port
ESPPORT=/dev/ttyUSB0

include ../../common.mk

# $(eval $(call make_spiffs_image,spiff_images))

html:
	@echo "Generating fsdata.."
	cd http_server/fsdata && ./makefsdata
