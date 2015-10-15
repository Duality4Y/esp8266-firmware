#!/bin/bash
python2 ~/esptool.py erase_flash --port /dev/ttyUSB0
bash do_espfs.sh
bash do_make.sh
bash do_flash_full.sh
