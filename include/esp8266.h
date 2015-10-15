#ifndef __ESP8266_H__
#define __ESP8266_H__

/* File with all needed includes */

#include "user_config.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "spi_flash.h"
#include "driver/uart.h"
#include "user_settings.h"
#include "board.h"

//Missing include
char *ets_strstr(const char *haystack, const char *needle);

#endif
