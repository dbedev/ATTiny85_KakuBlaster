# ATTiny85_KakuBlaster

Simple Arduino program to turn an Digispark Attiny85 uController in to 
usb CDC ACM dongle to send 433.92mhz "Klik Aan Klik uit" signals.

Mostly intended for linux hosts, put the .rules file for /etc/udev/rules.d for easy usage of the device.

Use a program like https://github.com/dbedev/HassioKakuBlasterCtrl/tree/main/KakuSend to control the
/dev/ttyXXX node as the digispark tty somehow requires a short wait period before closing the node and
can be picky on the serial port settings. 

If you echo data directly into the device node, it will probably only work the first time you write to 
it as afterwards either the linux usb driver or the attiny85 usb gets stuck. 

Note that the Digispark USB CDC ACM devices are known to give issues including
BSOD's with windows 10 hosts when using clones. 
