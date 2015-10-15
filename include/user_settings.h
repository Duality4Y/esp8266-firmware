#ifndef __USER_SETTINGS_H__
#define __USER_SETTINGS_H__

  #define SETTINGS_VERSION 2
  typedef struct __attribute__((packed, aligned(4))) { //Size must always be a multiple of 4!
    //Description of settings
    uint8_t magic;
    uint8_t version;
    //Device information
    char name[32];
    bool settings_stored;
    //Network
    char ssid[32];
    char password[64];
    bool enable_dhcp;
    uint8_t static_ip_1;
    uint8_t static_ip_2;
    uint8_t static_ip_3;
    uint8_t static_ip_4;
    uint8_t static_gateway_1;
    uint8_t static_gateway_2;
    uint8_t static_gateway_3;
    uint8_t static_gateway_4;
    uint8_t static_netmask_1;
    uint8_t static_netmask_2;
    uint8_t static_netmask_3;
    uint8_t static_netmask_4;
    bool connection_successful;
    //MDNS
    bool enable_mdns;
    //NTP
    bool enable_ntp;
    char ntpserver[64];
    uint8_t timezone;
    bool enable_summertime;
    //Security
    char device_password[64];
    //Digital outputs
    uint8_t bootstate;
    //ESPLight
    uint8_t ledstrip_type;
    uint8_t ledstrip_length;
    uint8_t bootstate_R;
    uint8_t bootstate_G;
    uint8_t bootstate_B;
    //PKA
    uint8_t pka_wb;
    uint8_t pka_wb_time;
  } userSettings_t;

  userSettings_t* settings_get_pointer( void );
  bool settings_load ( userSettings_t *settings );
  void settings_store( userSettings_t *settings );
  void settings_print( userSettings_t *settings );
  void settings_apply( userSettings_t *settings );
  void ap_start( void );
  void ap_stop( void );
#endif
