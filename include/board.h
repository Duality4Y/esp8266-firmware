#ifndef __BOARD_H__
#define __BOARD_H__
  void board_init( void );
  bool board_setOutput( int output, bool state );
  bool board_getOutput( int output );
  bool board_getInput( uint8_t input );
  int board_sensorGetAirPressure ( void );
  int board_sensorGetTemperature ( void );
  void statusLedTimerInit();
  void statusLedTimerSet(uint32_t ms);
  void statusLedTimerStop( void );
  void board_statusLed( uint8_t mode );
  void statusLed2TimerInit();
  void statusLed2TimerSet(uint32_t ms);
  void statusLed2TimerStop( void );
  void board_statusLed2( uint8_t mode );
  void board_handle_gpio_interrupt( int8_t key );
  void pwmCb( void );
  void setPWM(uint8_t r, uint8_t g, uint8_t b);
  void board_setWS2812( uint8_t * buffer, uint16_t length );
  void board_checkAllInputs( void );

  #if DEVICE_TYPE==1 //Board is RN+ Relay board 1.0
    #define DHT22 14    //To-Do: DHT support
    #define LED 16
    #define OUTPUT1 13
    #define OUTPUT2 12
    #define BUTTON1 0
    #define BUTTON2 2
    #define DEVICE_TYPE_NAME "RN+ Relayboard v1.0"
  #elif DEVICE_TYPE==2 //Board is RN+ Relay board 2.1
    #define LED 16
    #define LED2 93
    #define OUTPUT1 90
    #define OUTPUT2 91
    #define OUTPUT3 92
    #define BUTTON1 0
    #define BUTTON2 97
    #define BUTTON3 96
    #define BUTTON4 95
    #define BUTTON5 94
    #define I2C_EN
    #define I2C_MCP 1
    #define I2C_BMP180
    #define I2C_MCP_INT 12
    #define DEVICE_TYPE_NAME "RN+ Relayboard v2.1"
  #elif DEVICE_TYPE==3 //Board is ESPLight
    #define BUTTON1 0
    #define PULLUP_ENABLE_0
    #define BUTTON2 12
    #define PULLUP_ENABLE_12
    #define OUTPUT1 5 //PWM RED
    #define OUTPUT2 4 //PWM GREEN
    #define OUTPUT3 2 //PWM BLUE
    #define OUTPUT4 13 //DIGITAL DATA
    #define PWM_ENABLE //Enables PWM on output 1,2 and 3
    #define WS2812 13
    #define DEVICE_TYPE_NAME "ESPLight 1.0"
  #elif DEVICE_TYPE==4 //Board is RN+ Earthquake sensor PROTOTYPE 1
    #define LED 16
    #define BUTTON1 0
    #define I2C_EN
    #define I2C_MMA8451
    #define DEVICE_TYPE_NAME "RN+ Earthquake sensor PR.1"
  #endif
  #ifndef OUTPUT1
    #define OUTPUT1 -1
  #endif
  #ifndef OUTPUT2
    #define OUTPUT2 -1
  #endif
  #ifndef OUTPUT3
    #define OUTPUT3 -1
  #endif
  #ifndef OUTPUT4
    #define OUTPUT4 -1
  #endif
  #ifndef OUTPUT5
    #define OUTPUT5 -1
  #endif
  #ifndef OUTPUT6
    #define OUTPUT6 -1
  #endif
  #ifndef OUTPUT7
    #define OUTPUT7 -1
  #endif
  #ifndef OUTPUT8
    #define OUTPUT8 -1
  #endif
  #ifndef BUTTON1
    #define BUTTON1 -1
  #endif
  #ifndef BUTTON2
    #define BUTTON2 -1
  #endif
  #ifndef BUTTON3
    #define BUTTON3 -1
  #endif
  #ifndef BUTTON4
    #define BUTTON4 -1
  #endif
  #ifndef BUTTON5
    #define BUTTON5 -1
  #endif
  #ifndef BUTTON6
    #define BUTTON6 -1
  #endif
  #ifndef BUTTON7
    #define BUTTON7 -1
  #endif
  #ifndef BUTTON8
    #define BUTTON8 -1
  #endif
  #ifndef LED
    #define LED -1
  #endif
  #ifndef LED2
    #define LED2 -1
  #endif
  #ifndef I2C_MCP_INT
    #define I2C_MCP_INT -1
  #endif
#endif
