#include <esp8266.h>
#include "cgi.h"
#include "user_main.h"

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

//Static scan status storage.
static ScanResultData cgiWifiAps;

#define CONNTRY_IDLE 0
#define CONNTRY_WORKING 1
#define CONNTRY_SUCCESS 2
#define CONNTRY_FAIL 3

//Callback the code calls when a wlan ap scan is done. Basically stores the result in
//the cgiWifiAps struct.
void ICACHE_FLASH_ATTR wifiScanDoneCb(void *arg, STATUS status) {
	int n;
	struct bss_info *bss_link = (struct bss_info *)arg;
	os_printf("wifiScanDoneCb %d\n", status);
	if (status!=OK) {
		cgiWifiAps.scanInProgress=0;
		return;
	}

	//Clear prev ap data if needed.
	if (cgiWifiAps.apData!=NULL) {
		for (n=0; n<cgiWifiAps.noAps; n++) os_free(cgiWifiAps.apData[n]);
		os_free(cgiWifiAps.apData);
	}

	//Count amount of access points found.
	n=0;
	while (bss_link != NULL) {
		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//Allocate memory for access point data
	cgiWifiAps.apData=(ApData **)os_malloc(sizeof(ApData *)*n);
	cgiWifiAps.noAps=n;
	os_printf("Scan done: found %d APs\n", n);

	//Copy access point data to the static struct
	n=0;
	bss_link = (struct bss_info *)arg;
	while (bss_link != NULL) {
		if (n>=cgiWifiAps.noAps) {
			//This means the bss_link changed under our nose. Shouldn't happen!
			//Break because otherwise we will write in unallocated memory.
			os_printf("Huh? I have more than the allocated %d aps!\n", cgiWifiAps.noAps);
			break;
		}
		//Save the ap data.
		cgiWifiAps.apData[n]=(ApData *)os_malloc(sizeof(ApData));
		cgiWifiAps.apData[n]->rssi=bss_link->rssi;
		cgiWifiAps.apData[n]->enc=bss_link->authmode;
		strncpy(cgiWifiAps.apData[n]->ssid, (char*)bss_link->ssid, 32);

		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//We're done.
	cgiWifiAps.scanInProgress=0;
}

//Routine to start a WiFi access point scan.
static void ICACHE_FLASH_ATTR wifiStartScan() {
//	int x;
	if (cgiWifiAps.scanInProgress) return;
	cgiWifiAps.scanInProgress=1;
	wifi_station_scan(NULL, wifiScanDoneCb);
}

//This CGI is called from the bit of AJAX-code in wifi.tpl. It will initiate a
//scan for access points and if available will return the result of an earlier scan.
//The result is embedded in a bit of JSON parsed by the javascript in wifi.tpl.
int ICACHE_FLASH_ATTR cgiWiFiScan(HttpdConnData *connData) {
	int pos=(int)connData->cgiData;
	int len;
	char buff[1024];

	if (!cgiWifiAps.scanInProgress && pos!=0) {
		//Fill in json code for an access point
		if (pos-1<cgiWifiAps.noAps) {
			len=os_sprintf(buff, "{\"essid\": \"%s\", \"rssi\": \"%d\", \"enc\": \"%d\"}%s\n",
					cgiWifiAps.apData[pos-1]->ssid, cgiWifiAps.apData[pos-1]->rssi,
					cgiWifiAps.apData[pos-1]->enc, (pos-1==cgiWifiAps.noAps-1)?"":",");
			httpdSend(connData, buff, len);
		}
		pos++;
		if ((pos-1)>=cgiWifiAps.noAps) {
			len=os_sprintf(buff, "]\n}\n}\n");
			httpdSend(connData, buff, len);
			//Also start a new scan.
			wifiStartScan();
			return HTTPD_CGI_DONE;
		} else {
			connData->cgiData=(void*)pos;
			return HTTPD_CGI_MORE;
		}
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	if (cgiWifiAps.scanInProgress==1) {
		//We're still scanning. Tell Javascript code that.
		len=os_sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"1\"\n }\n}\n");
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	} else {
		//We have a scan result. Pass it on.
		len=os_sprintf(buff, "{\n \"result\": { \n\"inProgress\": \"0\", \n\"APs\": [\n");
		httpdSend(connData, buff, len);
		if (cgiWifiAps.apData==NULL) cgiWifiAps.noAps=0;
		connData->cgiData=(void *)1;
		return HTTPD_CGI_MORE;
	}
}

char temperatureString[10] = "Unknown";
char pressureString[10] = "Unknown";

char* ICACHE_FLASH_ATTR updateSensorStrings ( void )
{
  int temperature = board_sensorGetTemperature();
  if (temperature!=-200)
  {
    uint8_t temperature_a = temperature/10;
    uint8_t temperature_b = temperature - (temperature/10);
    os_sprintf(temperatureString,"%d.%d\n\r", temperature_a, temperature_b );
  }
  int pressure = board_sensorGetAirPressure();
  if (pressure!=-200)
  {
    uint8_t pressure_a = pressure/1000;
    uint8_t pressure_b = pressure - (pressure/1000);
    os_sprintf(pressureString, "%d.%d\n\r", pressure_a, pressure_b );
  }
}

void setSwitchString(char* buff, uint32_t pin)
{
  char text2[4] = "on";
  char text[4] = "OFF";
  if (board_getOutput(pin)) {
    os_sprintf(text, "ON");
    os_sprintf(text2, "off");
  }
  os_sprintf(buff, "<a class='button' href='/output?pin=%d&state=%d'>[%s] Switch %s</a>", pin, !board_getOutput(pin), text, text2);
}

//Template code for the homepage
int ICACHE_FLASH_ATTR tplHomepage(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	if (os_strcmp(token, "temperature")==0) {
		updateSensorStrings(); //Called 1st
		os_sprintf(buff, "%s", temperatureString);
	}
	if (os_strcmp(token, "pressure")==0) {
		//updateSensorStrings(); //Called 2nd, update not needed
		os_sprintf(buff, "%s", pressureString);
	}
	if (os_strcmp(token, "hour")==0) {
		updateSensorStrings();
		os_sprintf(buff, "--");
	}
	if (os_strcmp(token, "minute")==0) {
		updateSensorStrings();
		os_sprintf(buff, "--");
	}
	if (os_strcmp(token, "second")==0) {
		updateSensorStrings();
		os_sprintf(buff, "--");
	}
	if (os_strcmp(token, "output1")==0) {
		setSwitchString(buff, OUTPUT1);
	}
	if (os_strcmp(token, "output2")==0) {
		setSwitchString(buff, OUTPUT2);
	}
	if (os_strcmp(token, "output3")==0) {
		setSwitchString(buff, OUTPUT3);
	}
	if (os_strcmp(token, "hidewarning")==0) {
	  userSettings_t* settings = settings_get_pointer();
          if (settings->settings_stored) {
            os_sprintf(buff, " hidden");
          } else {
            os_sprintf(buff, "");
	  }
	}
		
	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiSetOutput(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

  uint32_t pin = 999;
  bool state = false;

	len=httpdFindArg(connData->post->buff, "pin", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "pin", buff, sizeof(buff));
	if (len>0) pin=atoi(buff);
	len=httpdFindArg(connData->post->buff, "state", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "state", buff, sizeof(buff));
	if (len>0) state=atoi(buff);

  bool result = board_setOutput(pin, state);

	len=httpdFindArg(connData->post->buff, "cmd", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "cmd", buff, sizeof(buff));
	if (len>0) {
	  char buff[128];
    if (result) {
	    os_sprintf(buff, "OK");
    } else {
      os_sprintf(buff, "ERROR");
    }
	  httpdSend(connData, buff, strlen(buff));
	  return HTTPD_CGI_DONE;
	}
  else
  {
	  httpdRedirect(connData, "home");
	  return HTTPD_CGI_DONE;
  }
}

int ICACHE_FLASH_ATTR cgiSetPwm(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	bool result = false;
	int red = 0;
	int green = 0;
	int blue = 0;

	len=httpdFindArg(connData->post->buff, "color", buff, sizeof(buff));
	if (len<=0) len=httpdFindArg(connData->getArgs, "color", buff, sizeof(buff));
	if (len==6) {
		//pin=atoi(buff);
		char rstring[3] = {0};
		char gstring[3] = {0};
		char bstring[3] = {0};
		os_memcpy( rstring, buff, 2 );
		os_memcpy( gstring, buff+2, 2 );
		os_memcpy( bstring, buff+4, 2 );
		red = (int)strtol(rstring, NULL, 16);
		green = (int)strtol(gstring, NULL, 16);
		blue = (int)strtol(bstring, NULL, 16);
		setPWM(red, green, blue);
		os_printf("PWM SET TO %x,%x,%x\n\r", red, green, blue);
		result = true;
	}

	len=httpdFindArg(connData->post->buff, "cmd", buff, sizeof(buff));
	if (len<=0) len=httpdFindArg(connData->getArgs, "cmd", buff, sizeof(buff));
	if (len>0) {
		char buff[128];
		if (result) {
			os_sprintf(buff, "OK");
		} else {
			os_sprintf(buff, "ERROR");
		}
		httpdSend(connData, buff, strlen(buff));
		return HTTPD_CGI_DONE;
	}
	else
	{
		httpdRedirect(connData, "pwm");
		return HTTPD_CGI_DONE;
	}
}

int ICACHE_FLASH_ATTR cgiSetWS2812(HttpdConnData *connData) {
	int len;
	char buff[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	bool result = false;
	int red = 0;
	int green = 0;
	int blue = 0;

	len=httpdFindArg(connData->post->buff, "color", buff, sizeof(buff));
	if (len<=0) len=httpdFindArg(connData->getArgs, "color", buff, sizeof(buff));
	if (len==6) {
		//pin=atoi(buff);
		char rstring[3] = {0};
		char gstring[3] = {0};
		char bstring[3] = {0};
		os_memcpy( rstring, buff, 2 );
		os_memcpy( gstring, buff+2, 2 );
		os_memcpy( bstring, buff+4, 2 );
		red = (int)strtol(rstring, NULL, 16);
		green = (int)strtol(gstring, NULL, 16);
		blue = (int)strtol(bstring, NULL, 16);
		uint8_t buffer[64] = {0};
		buffer[0] = green;
		buffer[1] = red;
		buffer[2] = blue;
		board_setWS2812(buffer, 3);
		os_printf("WS2812 SET TO %x,%x,%x\n\r", red, green, blue);
		result = true;
	}

	len=httpdFindArg(connData->post->buff, "cmd", buff, sizeof(buff));
	if (len<=0) len=httpdFindArg(connData->getArgs, "cmd", buff, sizeof(buff));
	if (len>0) {
		char buff[128];
		if (result) {
			os_sprintf(buff, "OK");
		} else {
			os_sprintf(buff, "ERROR");
		}
		httpdSend(connData, buff, strlen(buff));
		return HTTPD_CGI_DONE;
	}
	else
	{
		httpdRedirect(connData, "ws2812");
		return HTTPD_CGI_DONE;
	}
}


//Template code for the settings page
int ICACHE_FLASH_ATTR tplSettings(HttpdConnData *connData, char *token, void **arg)
{
  char buff[128];
  if (token==NULL) return HTTPD_CGI_DONE;
  userSettings_t* settings = settings_get_pointer();
  if (os_strcmp(token, "name")==0) {
    os_memcpy( buff, settings->name, 32 );
  }
  if (os_strcmp(token, "settings_stored")==0) {
    os_sprintf(buff, "%d", settings->settings_stored);
  }
  if (os_strcmp(token, "ssid")==0) {
    os_memcpy( buff, settings->ssid, 32 );
  }
  if (os_strcmp(token, "password")==0) {
    os_memcpy( buff, settings->password, 64 );
  }
  if (os_strcmp(token, "enable_dhcp")==0) {
    if (settings->enable_dhcp) {
      os_sprintf(buff, " checked");
    } else {
      os_sprintf(buff, "");
    }
  }
  if (os_strcmp(token, "static_ip_1")==0) os_sprintf(buff, "%d", settings->static_ip_1);
  if (os_strcmp(token, "static_ip_2")==0) os_sprintf(buff, "%d", settings->static_ip_2);
  if (os_strcmp(token, "static_ip_3")==0) os_sprintf(buff, "%d", settings->static_ip_3);
  if (os_strcmp(token, "static_ip_4")==0) os_sprintf(buff, "%d", settings->static_ip_4);
  if (os_strcmp(token, "static_gateway_1")==0) os_sprintf(buff, "%d", settings->static_gateway_1);
  if (os_strcmp(token, "static_gateway_2")==0) os_sprintf(buff, "%d", settings->static_gateway_2);
  if (os_strcmp(token, "static_gateway_3")==0) os_sprintf(buff, "%d", settings->static_gateway_3);
  if (os_strcmp(token, "static_gateway_4")==0) os_sprintf(buff, "%d", settings->static_gateway_4);
  if (os_strcmp(token, "static_netmask_1")==0) os_sprintf(buff, "%d", settings->static_netmask_1);
  if (os_strcmp(token, "static_netmask_2")==0) os_sprintf(buff, "%d", settings->static_netmask_2);
  if (os_strcmp(token, "static_netmask_3")==0) os_sprintf(buff, "%d", settings->static_netmask_3);
  if (os_strcmp(token, "static_netmask_4")==0) os_sprintf(buff, "%d", settings->static_netmask_4);
  if (os_strcmp(token, "connection_successful")==0) os_sprintf(buff, "%d", settings->connection_successful);
  if (os_strcmp(token, "enable_mdns")==0) {
    if (settings->enable_mdns) {
      os_sprintf(buff, " checked");
    } else {
      os_sprintf(buff, "");
    }
  }
  if (os_strcmp(token, "enable_ntp")==0) {
    if (settings->enable_ntp) {
      os_sprintf(buff, " checked");
    } else {
      os_sprintf(buff, "");
    }
  }
  if (os_strcmp(token, "ntp_server")==0) os_memcpy( buff, settings->ntpserver, 64 );
  if (os_strcmp(token, "timezone")==0) os_sprintf( buff, "%d", settings->timezone );
  if (os_strcmp(token, "ntp_summer")==0) {
    if (settings->enable_summertime) {
      os_sprintf(buff, " checked");
    } else {
      os_sprintf(buff, "");
    }
  }
  if (os_strcmp(token, "bootstate")==0) os_sprintf( buff, "%d", settings->bootstate );
  if (os_strcmp(token, "device_password")==0) os_memcpy( buff, settings->device_password, 64 );
  if (os_strcmp(token, "ledstrip_type")==0) os_sprintf( buff, "%d", settings->ledstrip_type );
  if (os_strcmp(token, "ledstrip_length")==0) os_sprintf( buff, "%d", settings->ledstrip_length );
  if (os_strcmp(token, "bootstate_R")==0) os_sprintf( buff, "%d", settings->bootstate_R );
  if (os_strcmp(token, "bootstate_G")==0) os_sprintf( buff, "%d", settings->bootstate_G );
  if (os_strcmp(token, "bootstate_B")==0) os_sprintf( buff, "%d", settings->bootstate_B );
  if (os_strcmp(token, "pka_wb")==0) os_sprintf( buff, "%d", settings->pka_wb );
  if (os_strcmp(token, "pka_wb_time")==0) os_sprintf( buff, "%d", settings->pka_wb_time );
  httpdSend(connData, buff, -1);
  return HTTPD_CGI_DONE;
}

bool apply_settings_timer_exists = false;
os_timer_t apply_settings_timer;

static void apply_settings_timer_cb(void *arg)
{
  userSettings_t* settings = settings_get_pointer();
  settings_apply( settings );
  os_printf("!!!!!!!!!!!!!!!!!! SETTINGS APPLIED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\r");
}

int ICACHE_FLASH_ATTR cgiSettingsStore(HttpdConnData *connData)
{
  int len;
  char buff[1024];
  userSettings_t* settings = settings_get_pointer();
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
  len=httpdFindArg(connData->post->buff, "name", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "name", buff, sizeof(buff));
  if (len>0) os_memcpy(settings->name, buff, 32);
  len=httpdFindArg(connData->post->buff, "ssid", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "ssid", buff, sizeof(buff));
  if (len>0) os_memcpy(settings->ssid, buff, 32);
  len=httpdFindArg(connData->post->buff, "password", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "password", buff, sizeof(buff));
  if (len>0) os_memcpy(settings->password, buff, 64);
  len=httpdFindArg(connData->post->buff, "enable_dhcp", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "enable_dhcp", buff, sizeof(buff));
  if (len>0) settings->enable_dhcp = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_ip_1", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_ip_1", buff, sizeof(buff));
  if (len>0) settings->static_ip_1 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_ip_2", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_ip_2", buff, sizeof(buff));
  if (len>0) settings->static_ip_2 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_ip_3", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_ip_3", buff, sizeof(buff));
  if (len>0) settings->static_ip_3 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_ip_4", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_ip_4", buff, sizeof(buff));
  if (len>0) settings->static_ip_4 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_gateway_1", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_gateway_1", buff, sizeof(buff));
  if (len>0) settings->static_gateway_1 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_gateway_2", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_gateway_2", buff, sizeof(buff));
  if (len>0) settings->static_gateway_2 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_gateway_3", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_gateway_3", buff, sizeof(buff));
  if (len>0) settings->static_gateway_3 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_gateway_4", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_gateway_4", buff, sizeof(buff));
  if (len>0) settings->static_gateway_4 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_netmask_1", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_netmask_1", buff, sizeof(buff));
  if (len>0) settings->static_netmask_1 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_netmask_2", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_netmask_2", buff, sizeof(buff));
  if (len>0) settings->static_netmask_2 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_netmask_3", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_netmask_3", buff, sizeof(buff));
  if (len>0) settings->static_netmask_3 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "static_netmask_4", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "static_netmask_4", buff, sizeof(buff));
  if (len>0) settings->static_netmask_4 = atoi(buff);
  len=httpdFindArg(connData->post->buff, "connection_successful", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "connection_successful", buff, sizeof(buff));
  if (len>0) settings->connection_successful = atoi(buff);
  len=httpdFindArg(connData->post->buff, "enable_mdns", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "enable_mdns", buff, sizeof(buff));
  if (len>0) settings->enable_mdns = atoi(buff);
  len=httpdFindArg(connData->post->buff, "enable_ntp", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "enable_ntp", buff, sizeof(buff));
  if (len>0) settings->enable_ntp = atoi(buff);
  len=httpdFindArg(connData->post->buff, "ntp_server", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "ntp_server", buff, sizeof(buff));
  if (len>0) os_memcpy(settings->ntpserver, buff, 64);
  len=httpdFindArg(connData->post->buff, "timezone", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "timezone", buff, sizeof(buff));
  if (len>0) settings->timezone = atoi(buff);
  len=httpdFindArg(connData->post->buff, "enable_summertime", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "enable_summertime", buff, sizeof(buff));
  if (len>0) settings->enable_summertime = atoi(buff);
  len=httpdFindArg(connData->post->buff, "device_password", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "device_password", buff, sizeof(buff));
  if (len>0) os_memcpy(settings->device_password, buff, 64);
  len=httpdFindArg(connData->post->buff, "bootstate", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "bootstate", buff, sizeof(buff));
  if (len>0) settings->bootstate = atoi(buff);
  len=httpdFindArg(connData->post->buff, "ledstrip_type", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "ledstrip_type", buff, sizeof(buff));
  if (len>0) settings->ledstrip_type = atoi(buff);
  len=httpdFindArg(connData->post->buff, "ledstrip_length", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "ledstrip_length", buff, sizeof(buff));
  if (len>0) settings->ledstrip_length = atoi(buff);
  len=httpdFindArg(connData->post->buff, "bootstate_R", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "bootstate_R", buff, sizeof(buff));
  if (len>0) settings->bootstate_R = atoi(buff);
  len=httpdFindArg(connData->post->buff, "bootstate_G", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "bootstate_G", buff, sizeof(buff));
  if (len>0) settings->bootstate_G = atoi(buff);
  len=httpdFindArg(connData->post->buff, "bootstate_B", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "bootstate_B", buff, sizeof(buff));
  if (len>0) settings->bootstate_B = atoi(buff);
  len=httpdFindArg(connData->post->buff, "pka_wb", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "pka_wb", buff, sizeof(buff));
  if (len>0) settings->pka_wb = atoi(buff);
  len=httpdFindArg(connData->post->buff, "pka_wb_time", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "pka_wb_time", buff, sizeof(buff));
  if (len>0) settings->pka_wb_time = atoi(buff); 
  settings->settings_stored = true;
  settings_store( settings );
  httpdRedirect(connData, "/settingsstored");
  if (!apply_settings_timer_exists) {
    os_memset(&apply_settings_timer,0,sizeof(os_timer_t));
    os_timer_disarm(&apply_settings_timer);
    os_timer_setfn(&apply_settings_timer, (os_timer_func_t *)apply_settings_timer_cb, NULL);
    apply_settings_timer_exists = true;
    os_printf("Created apply timer!");
  }
  os_timer_arm(&apply_settings_timer, 500, 0);
  return HTTPD_CGI_DONE;
}


int ICACHE_FLASH_ATTR cgiWiFiStatus(HttpdConnData *connData)
{
  int len;
  char buff[256];
  if (connData->conn==NULL) return HTTPD_CGI_DONE;
  len=httpdFindArg(connData->post->buff, "disableap", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "disableap", buff, sizeof(buff));
  if (len>0) {
    userSettings_t* settings = settings_get_pointer();
    os_sprintf(buff, "Accesspoint disabled. To re-enable the accesspoint please press the AP/setup button on the device.");
    httpdSend(connData, buff, strlen(buff));
    settings->connection_successful = 1;
    settings_store( settings );
    if (!apply_settings_timer_exists) {
      os_memset(&apply_settings_timer,0,sizeof(os_timer_t));
      os_timer_disarm(&apply_settings_timer);
      os_timer_setfn(&apply_settings_timer, (os_timer_func_t *)apply_settings_timer_cb, NULL);
      apply_settings_timer_exists = true;
      os_printf("Created apply timer!");
    }
    os_timer_arm(&apply_settings_timer, 500, 0);
    return HTTPD_CGI_DONE;
  }
  len=httpdFindArg(connData->post->buff, "ip", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "ip", buff, sizeof(buff));
  if (len>0) {
    struct ip_info info;
    wifi_get_ip_info( 0x00, &info );
    os_sprintf(buff, IPSTR, IP2STR(&info.ip));
    httpdSend(connData, buff, strlen(buff));
    return HTTPD_CGI_DONE;
  }
  len=httpdFindArg(connData->post->buff, "ssid", buff, sizeof(buff));
  if (len<=0) len=httpdFindArg(connData->getArgs, "ssid", buff, sizeof(buff));
  if (len>0) {
    os_sprintf(buff, "%s", settings_get_pointer()->ssid);
    httpdSend(connData, buff, strlen(buff));
    return HTTPD_CGI_DONE;
  }
  os_sprintf(buff, "%d", wifi_station_get_connect_status());
  httpdSend(connData, buff, strlen(buff));
  return HTTPD_CGI_DONE;
}
