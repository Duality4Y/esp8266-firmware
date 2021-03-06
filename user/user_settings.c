/*
 * user_settings.c
 * Handles settings storage
 * Copyright 2015 RN+ (Renze Nicolai)
 * License: GPLv2
 */

#include <esp8266.h>

void ap_start( void )
{
  if (wifi_get_opmode()<0x03) {
    wifi_set_opmode_current( STATIONAP_MODE );
    struct softap_config softapConf;
    char* apssid[32];
    char sta_mac[6] = {0};
    wifi_get_macaddr(STATION_IF, sta_mac);
    os_sprintf(apssid, "%s_%x%x%x", DEFAULT_AP_SSID, sta_mac[3], sta_mac[4], sta_mac[5]);
    os_memcpy( &softapConf.ssid, apssid, 32 );
    os_memcpy( &softapConf.password, "", 64 );
    softapConf.ssid_len=os_strlen( &apssid );
    //softapConf.channel= 6;
    softapConf.authmode= AUTH_OPEN;
    softapConf.max_connection= 2;
    softapConf.ssid_hidden= false;
    wifi_softap_set_config_current(&softapConf);
    wifi_softap_dhcps_start();
    os_printf("Setup network is enabled.\n\r");
    board_statusLed2(1);
  }
}

void ap_stop( void )
{
  if (wifi_get_opmode()!=STATION_MODE) {
    wifi_set_opmode_current( STATION_MODE );
    os_printf("Switched to station mode.\n\r");
    board_statusLed2(0);
  }
}

userSettings_t settings;

userSettings_t* settings_get_pointer( void )
{
  return &settings;
}

void ICACHE_FLASH_ATTR settings_load_default( userSettings_t *settings )
{
  //Description
  settings->magic = 0x42;
  settings->version = SETTINGS_VERSION;
  //Device information
  strcpy(settings->name, DEFAULT_HOSTNAME);
  settings->settings_stored = false;
  //Network
  strcpy(settings->ssid, "");
  strcpy(settings->password, "");
  settings->enable_dhcp = true;
  settings->static_ip_1 = 192;
  settings->static_ip_2 = 168;
  settings->static_ip_3 = 1;
  settings->static_ip_4 = 100;
  settings->static_gateway_1 = 192;
  settings->static_gateway_2 = 168;
  settings->static_gateway_3 = 1;
  settings->static_gateway_4 = 1;
  settings->static_netmask_1 = 255;
  settings->static_netmask_2 = 255;
  settings->static_netmask_3 = 255;
  settings->static_netmask_4 = 0;
  settings->connection_successful = false;
  //MDNS
  settings->enable_mdns = 0;
  //NTP
  settings->enable_ntp = true;
  strcpy(settings->ntpserver, NTP_DEFAULT_SERVER);
  settings->timezone = 13; //UTC+1 (UTC is 12)
  settings->enable_summertime = true; //True for Netherlands
  //Security
  strcpy(settings->device_password, "");
  //Digital outputs
  settings->bootstate = 0;
  //ESPLight
  settings->ledstrip_type = 0;
  settings->ledstrip_length = 0;
  settings->bootstate_R = 0;
  settings->bootstate_G = 0;
  settings->bootstate_B = 0;
  //PKA
  settings->pka_wb = 0;
  settings->pka_wb_time = 0;
}

bool ICACHE_FLASH_ATTR settings_load( userSettings_t *settings )
{
  spi_flash_read(SETTINGS_SECTOR*SPI_FLASH_SEC_SIZE, (uint32 *) settings,sizeof(userSettings_t));
  if (settings->magic==0x42) {
    if (settings->version==SETTINGS_VERSION) {
      return true;
    } else {
      os_printf("Unknown or corrupted settings found (Version '%d').\n\r", settings->version);
      settings_load_default( settings );
      return false;
    }
  } else {
    os_printf("Device has not been configured, loading default settings.\n\r");
    settings_load_default( settings );
  }
}

void ICACHE_FLASH_ATTR settings_store( userSettings_t *settings )
{
  spi_flash_erase_sector(SETTINGS_SECTOR);
  spi_flash_write(SETTINGS_SECTOR*SPI_FLASH_SEC_SIZE, (uint32 *) settings,sizeof(userSettings_t));
  os_printf("Settings saved to flash!\n\r");
}

void ICACHE_FLASH_ATTR settings_print( userSettings_t *settings )
{
  if (settings->magic==0x42)
  {
    os_printf("Version: %d\n\r", settings->version);
    if (settings->version==SETTINGS_VERSION)
    {
      os_printf("Device name: %s\n\r", settings->name);
      os_printf("Settings stored: %d\n\r", settings->settings_stored);
      os_printf("Succesfully connected at least once: %d\n\r", settings->connection_successful);
      os_printf("Network SSID: %s\n\r", settings->ssid);
      os_printf("Network password: %s\n\r", settings->password);
      if (settings->enable_dhcp) {
        os_printf("IP configuration is done using DHCP.\n\r");
      } else {
        os_printf("IP configuration is done manually.\n\r");
        os_printf("IP: %d.%d.%d.%d\n\r", settings->static_ip_1, settings->static_ip_2, settings->static_ip_3, settings->static_ip_4);
        os_printf("Gateway: %d.%d.%d.%d\n\r", settings->static_gateway_1, settings->static_gateway_2, settings->static_gateway_3, settings->static_gateway_4);
        os_printf("Netmask: %d.%d.%d.%d\n\r", settings->static_netmask_1, settings->static_netmask_2, settings->static_netmask_3, settings->static_netmask_4);
      }
      if (settings->enable_mdns) {
        os_printf("MDNS is enabled\n\r");
      } else {
        os_printf("MDNS is disabled\n\r");
      }
      if (settings->enable_ntp) {
        os_printf("NTP is enabled\n\r");
        os_printf("NTP server: %s\n\r", settings->ntpserver);
        os_printf("Timezone (UTC=12): %d\n\r", settings->timezone);
        os_printf("Automatic summertime adjust: %d\n\r", settings->enable_summertime);
      } else {
        os_printf("NTP is disabled\n\r");
      }
      os_printf("Password: %s\n\r", settings->password);
      os_printf("Bootstate: %x\n\r", settings->bootstate);
      os_printf("Ledstrip type: %d\n\r", settings->ledstrip_type);
      os_printf("Ledstrip length: %d\n\r", settings->ledstrip_length);
      os_printf("Bootstate RED: %d\n\r", settings->bootstate_R);
      os_printf("Bootstate GREEN: %d\n\r", settings->bootstate_G);
      os_printf("Bootstate BLUE: %d\n\r", settings->bootstate_B);
      os_printf("PKA WB: %d\n\r", settings->pka_wb);
      os_printf("PKA WB TIME: %d\n\r", settings->pka_wb_time);
    }
    else
    {
      os_printf("Unknown version.\n\r");
    }
  }
  else
  {
    os_printf("Invalid magic.\n\r");
  }
}

void ICACHE_FLASH_ATTR settings_apply( userSettings_t *settings )
{
  //Description
  if (!((settings->magic==0x42)&&(settings->version==SETTINGS_VERSION))) {
    os_printf("<ERROR: CAN NOT APPLY CORRUPT SETTINGS!>\n\r");
    return;
  }
  
  if (settings->connection_successful) {
    ap_stop();
  } else {
    ap_start();
  }
  
  //Device information
  wifi_station_set_hostname(settings->name);
  
  //Network
  struct station_config stationConf;
  wifi_station_get_config( &stationConf );
  stationConf.bssid_set = 0;
  os_memcpy( &stationConf.ssid, settings->ssid, 32 );
  os_memcpy( &stationConf.password, settings->password, 64 );
  wifi_station_set_config_current( &stationConf );
  os_printf("########################## Wifi config set to SSID '%s' and PASSWORD '%s'.\n\r", settings->ssid, settings->password);
  if (settings->enable_dhcp) {
    wifi_station_dhcpc_start();
  } else {
    wifi_station_dhcpc_stop();
    struct ip_info dhcpc_ip_info;
    IP4_ADDR(&dhcpc_ip_info.ip, settings->static_ip_1, settings->static_ip_2, settings->static_ip_3, settings->static_ip_4);
    IP4_ADDR(&dhcpc_ip_info.gw, settings->static_gateway_1, settings->static_gateway_2, settings->static_gateway_3, settings->static_gateway_4);
    IP4_ADDR(&dhcpc_ip_info.netmask, settings->static_netmask_1, settings->static_netmask_2, settings->static_netmask_3, settings->static_netmask_4);
    wifi_set_ip_info(STATION_IF, &dhcpc_ip_info);
  }
  wifi_station_disconnect();
  
  //MDNS
  if (settings->enable_mdns)
  {
    /*wifi_set_broadcast_if(STATIONAP_MODE);
    struct mdns_info *info = (struct mdns_info *) os_zalloc(sizeof(struct mdns_info));
    info->host_name = settings->mdns_hostname;
    info->ipAddr = station_ipconfig.ip.addr; //ESP8266 station IP
    info->server_name = "FirmwaRe MDNS";
    info->server_port = 80;
    info->txt_data[0] = “version = now”;
    info->txt_data[1] = “user1 = data1”;
    info->txt_data[2] = “user2 = data2”;
    espconn_mdns_init(info);*/
  }
  
  //NTP
  if (settings->enable_ntp)
  {
    //settings->ntpserver
    //settings->timezone
    //settings->enable_summertime
  }
  
  //Security
  /* settings->password; */

  //Digital outputs
  #if OUTPUT1>-1
    if (settings->bootstate&1){
      board_setOutput(OUTPUT1, true);
    } else {
      board_setOutput(OUTPUT1, false);
    }
  #endif
  #if OUTPUT2>-1
    if (settings->bootstate&2) {
      board_setOutput(OUTPUT2, true);
    } else {
      board_setOutput(OUTPUT2, false);
    }
  #endif
  #if OUTPUT3>-1
    if (settings->bootstate&4) {
      board_setOutput(OUTPUT3, true);
    } else {
      board_setOutput(OUTPUT3, false);
    }
  #endif
  #if OUTPUT4>-1
    if (settings->bootstate&8) {
      board_setOutput(OUTPUT4, true);
    } else {
      board_setOutput(OUTPUT4, false);
    }
  #endif
  #if OUTPUT5>-1
    if (settings->bootstate&16) {
      board_setOutput(OUTPUT5, true);
    } else {
      board_setOutput(OUTPUT5, false);
    }
  #endif
  #if OUTPUT6>-1
    if (settings->bootstate&32) {
      board_setOutput(OUTPUT6, true);
    } else {
      board_setOutput(OUTPUT6, false);
    }
  #endif
  #if OUTPUT7>-1
    if (settings->bootstate&64) {
      board_setOutput(OUTPUT7, true);
    } else {
      board_setOutput(OUTPUT7, false);
    }
  #endif
  #if OUTPUT8>-1
    if (settings->bootstate&128) {
      board_setOutput(OUTPUT8, true);
    } else {
      board_setOutput(OUTPUT8, false);
    }
  #endif

  //ESPLight
  setPWM(settings->bootstate_R, settings->bootstate_G, settings->bootstate_B);
  //settings->ledstrip_type;
  //settings->ledstrip_length;
  
  //PKA
  /* settings->pka_wb settings->pka_wb_time */
}
