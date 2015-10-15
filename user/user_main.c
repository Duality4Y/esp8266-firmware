#include <esp8266.h>
#include "user_main.h"
#include "gpio.h"

//Sprites webserver
#include "httpd.h"
#include "httpdespfs.h"
#include "espfs.h"

#include "cgi.h"

HttpdBuiltInUrl builtInUrls[] = {
  //{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
  {"/", cgiRedirect, "/home"},
  {"/home", cgiEspFsTemplate, tplHomepage},
  {"/output", cgiSetOutput, NULL},
  {"/pwmset", cgiSetPwm, NULL},
  {"/WS2812set", cgiSetWS2812, NULL},
  {"/settings", cgiEspFsTemplate, tplSettings},
  {"/settings/store", cgiSettingsStore, NULL},
  {"/wifi", cgiRedirect, "/settings"},
  {"/wifi/scan", cgiWiFiScan, NULL},
  {"/wifi/status", cgiWiFiStatus, NULL},
  //{"/flash/download", cgiReadFlash, NULL},
  //{"/flash/next", cgiGetFirmwareNext, &uploadParams},
  //{"/flash/upload", cgiUploadFirmware, &uploadParams},
  //{"/flash/reboot", cgiRebootFirmware, NULL},
  //{"/websocket/ws.cgi", cgiWebsocket, myWebsocketConnect},
  {"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
  {NULL, NULL, NULL}
};

os_timer_t heapTimer;
void debugConnectionStatus( void );
static void heapTimerCb(void *arg)
{
  //system_print_meminfo();
  debugConnectionStatus();
}

bool disable_ap_after_connect = false;

os_timer_t disable_ap_timer;

static void disable_ap_timer_cb(void *arg)
{
  disable_ap_after_connect = false;
  userSettings_t* settings = settings_get_pointer();
  settings->connection_successful = true;
  settings_apply( settings );
  os_printf("Disable_ap_timer_cb\n\r");
}

void set_disable_ap_after_connect( bool v )
{
  disable_ap_after_connect = v;
}

void wifi_handle_event_cb(System_Event_t *evt)
{
  os_printf("event %x\n", evt->event);
  switch (evt->event) {
    case EVENT_STAMODE_CONNECTED:
      os_printf("WiFi Event: Connected to %s on channel %d.\n\r",
      evt->event_info.connected.ssid,
      evt->event_info.connected.channel);
      userSettings_t* settings = settings_get_pointer();
      if (wifi_get_opmode()==0x03) {
        settings->connection_successful = true;
        settings_store( settings );
        os_timer_arm(&disable_ap_timer, 15000, 0);
        board_statusLed2(3);
      }
      board_statusLed(3);
      break;
    case EVENT_STAMODE_DISCONNECTED:
      os_printf("WiFi Event: Disconnected from %s, reason %d\n\r",
      evt->event_info.disconnected.ssid,
      evt->event_info.disconnected.reason);
      board_statusLed(0);
      break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
      os_printf("WiFi Event: Authmode has changed from %d to %d.\n\r",
      evt->event_info.auth_change.old_mode,
      evt->event_info.auth_change.new_mode);
      break;
    case EVENT_STAMODE_GOT_IP:
      os_printf("WiFi Event: Got ip address " IPSTR " with subnetmask " IPSTR " from gateway " IPSTR "\n\r",
      IP2STR(&evt->event_info.got_ip.ip),
      IP2STR(&evt->event_info.got_ip.mask),
      IP2STR(&evt->event_info.got_ip.gw));
      board_statusLed(1);
      break;
    case EVENT_SOFTAPMODE_STACONNECTED:
      os_printf("WiFi Event: A client connected to the AP (" MACSTR ", AID = %d).\n\r",
      MAC2STR(evt->event_info.sta_connected.mac),
      evt->event_info.sta_connected.aid);
      break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
      os_printf("WiFi Event: A client disconnected from the AP (" MACSTR ", AID = %d).\n\r",
      MAC2STR(evt->event_info.sta_disconnected.mac),
      evt->event_info.sta_disconnected.aid);
      break;
    case EVENT_STAMODE_DHCP_TIMEOUT:
      os_printf("Wifi Event: DHCP timeout\n\r");
      break;
    case EVENT_SOFTAPMODE_PROBEREQRECVED:
      os_printf("Wifi Event: PROBEREQRECVED\n\r");
      break;
    case EVENT_MAX:
      os_printf("Wifi Event: MAX\n\r");
      break;
    default:
      //os_printf("WiFi Event: Unknown event.\n\r");
      break;
  }
}

void debugConnectionStatus(void)
{
  uint8_t status = wifi_station_get_connect_status();
  switch (status) {
    case STATION_IDLE:
      //os_printf("WiFi is idle, connecting...\n\r");
      wifi_station_connect();
      board_statusLed(0);
    break;
    case STATION_CONNECTING:
      //os_printf("Status: Connecting...\n\r");
    break;
    case STATION_WRONG_PASSWORD:
      //os_printf("Status: Wrong password\n\r");
    break;
    case STATION_NO_AP_FOUND:
      //os_printf("Status: No ap found\n\r");
    break;
    case STATION_CONNECT_FAIL:
      //os_printf("Status: Connect fail\n\r");
    break;
    case STATION_GOT_IP:
      /*os_printf("Status: Got IP ");
      struct ip_info info;
      wifi_get_ip_info(STATION_IF, &info);
      os_printf("[%d.%d.%d.%d]\r\n",IP2STR(&info.ip));*/
    break;
    default:
      //os_printf("Status: Unknown\n\r");
    break;
  }
}

//[1] Pre-os start function
void user_rf_pre_init(void)
{
  //Do nothing
}

//[3] System init done (system init happens after user_init)
void system_init_done(void)
{
  os_printf("System init done. Starting services...\n");
  espFsInit((void*)(0x40200000 + 0x1FC000));
  httpdInit(builtInUrls, 80);
}

//[2] Init function
void user_init(void)
{
  uart_init( BIT_RATE_115200,BIT_RATE_115200 );
  system_set_os_print( OS_DEBUG );
  os_printf( "FirmwaRe [v0.1.5] [%s] [%d] [%s] [%d,%d]\n\r",
    system_get_sdk_version(),
    system_get_boot_version(),
    DEVICE_TYPE_NAME,
    system_get_boot_mode(),
    system_get_userbin_addr()
  );

  /* Hardware */
  board_init();
  board_statusLed(4);
  board_statusLed2(0);

  /* Load settings */
  settings_load( settings_get_pointer() );
  settings_apply( settings_get_pointer() );

  /* Print settings (debug) */
  settings_print( settings_get_pointer() );

  os_memset(&heapTimer,0,sizeof(os_timer_t));
  os_timer_disarm(&heapTimer);
  os_timer_setfn(&heapTimer, (os_timer_func_t *)heapTimerCb, NULL);
  os_timer_arm(&heapTimer, 1000, 1);

  os_memset(&disable_ap_timer,0,sizeof(os_timer_t));
  os_timer_disarm(&disable_ap_timer);
  os_timer_setfn(&disable_ap_timer, (os_timer_func_t *)disable_ap_timer_cb, NULL);

  /* Tell os about post system start callback */
  system_init_done_cb(system_init_done);

  /* Set wifi event handler */
  wifi_set_event_handler_cb(wifi_handle_event_cb);
}
