#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

/*
 *    Generic IoT device firmware
 *    (c) Renze Nicolai 2015
 */

/* OS settings */
#define OS_DEBUG 1  //OS debug: Enable or disable os_printf()
#define USER 1 //We are building for user1

/* Flash settings */
#define SETTINGS_SECTOR 0xFC //This is the sector on which our settings are stored (1KB)

#define USER_QUICK_DATA_SECTOR 0xFF //4KB of free flash
#define USER_DATA_SECTOR 0x200 //This points to 1MB of free flash

/* Board settings */
#define DEVICE_NAME "RN+ Relay board (HW V2.1)"
#define DEVICE_TYPE 2

/* Device type list:
 * - 0: No hardware specific code or functions enabled
 * - 1: RN+ Relay board (HW V1.0) [2 relays, DHT22, 2 user inputs]
 * - 2: RN+ Relay board (HW V2.1) [3 relays, BMP180, 5 user inputs, ledstrip support, optional DS18B20]
 * - 3: ESPLight (Development version) [3 pwm outputs, ledstrip support, 2 user inputs]
 */

#define DEFAULT_HOSTNAME "wifiswitch"//+mac

#define DEFAULT_AP_SSID "WIFISWITCH"//+mac

/* Firmware update */
#define UPDATE_SVR "upgrade.sensorcloud.nl"

/* NTP */
#define NTP_DEFAULT_SERVER "time.nist.gov"

/* Webserver */
#define WEBSERVER_NAME "firmwaRe/1.0"


/* --- Automatic stuff, do not touch --- */

//FLASH_MAP_INITIAL_VALUE controls which section of flash is memory mapped by default.
//FLASH_MAP_INITIAL_VALUE should be set to 1 if the program is loaded to user2 instead of user1 (Not tested)
#define FLASH_MAP_INITIAL_VALUE USER-1

#endif
