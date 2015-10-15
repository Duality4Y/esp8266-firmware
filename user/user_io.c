/*
 * user_io.c
 * Handles inputs and outputs
 * Copyright 2015 Renze Nicolai (RN+)
 * License: GPLv2
 */

#include <esp8266.h>
#include "spi_flash.h"
#include "user_io.h"

void ICACHE_FLASH_ATTR handleButtonInterrupt( uint8_t button )
{
  //os_printf("handleButtonInterrupt [%d]\n", button);
  switch (button)
  {
    case 0:
      if (wifi_get_opmode()<0x03) {
        ap_start();
        os_printf("Setup mode enabled!\n");
      } else {
        os_printf("Already in setup mode.\n");
      }
      break;
    case 1:
      os_printf("Sensor 1 got triggered!\n"); //Sensor 1
      board_setOutput(OUTPUT1, !board_getOutput(OUTPUT1));
      break;
    case 2:
      os_printf("Sensor 2 got triggered!\n"); //Sensor 2
      board_setOutput(OUTPUT2, !board_getOutput(OUTPUT2));
      break;
    case 3:
      os_printf("Sensor 3 got triggered!\n"); //Sensor 3
      board_setOutput(OUTPUT3, !board_getOutput(OUTPUT3));
      break;
    case 4:
      os_printf("Sensor 4 got triggered!\n"); //Sensor 4
      //board_setOutput(OUTPUT4, !board_getOutput(OUTPUT4));
      break;
    /*
    case 5:
      os_printf("USER BUTTON #6 WAS PRESSED!!!\n");
      break;
    case 6:
      os_printf("USER BUTTON #7 WAS PRESSED!!!\n");
      break;
    case 7:
      os_printf("USER BUTTON #8 WAS PRESSED!!!\n");
      break; */
    default:
      os_printf("USER BUTTON <UNKNOWN> WAS PRESSED!!!\n");
      break;
  }
}
