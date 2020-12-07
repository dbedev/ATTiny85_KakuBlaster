# ATTiny85_KakuBlaster

Simple Arduino program to turn an Digispark Attiny85 uController in to 
usb CDC ACM dongle to send 433.92mhz "Klik Aan Klik uit" signals.

Mostly intended for linux hosts, the rules file for /etc/udev/rules.d for easy usage of the device.

Note that the Digispark USB CDC ACM devices are known to give issues including
BSOD's with windows 10 hosts when using clones. 
