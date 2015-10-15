#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int tplHomepage(HttpdConnData *connData, char *token, void **arg);
int tplSettings(HttpdConnData *connData, char *token, void **arg);
int cgiSetOutput(HttpdConnData *connData);
int cgiSetPwm(HttpdConnData *connData);
int cgiSetWS2812(HttpdConnData *connData);
int cgiWiFiScan(HttpdConnData *connData);
int cgiSettingsStore(HttpdConnData *connData);
int ICACHE_FLASH_ATTR cgiWiFiStatus(HttpdConnData *connData);

/*int tplSettings(HttpdConnData *connData, char *token, void **arg);
int tplScheduler(HttpdConnData *connData, char *token, void **arg);
int tplApi(HttpdConnData *connData, char *token, void **arg);
int cgiSettings(HttpdConnData *connData);
int cgiApi(HttpdConnData *connData);*/

#endif
