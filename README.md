# Introduction 
A home should provide a comfortable living space for humans. However, it can pose challenges for individuals with mobility impairments. For example, a person with mobility limitations may encounter difficulties reaching the light switch to turn the lights on or off. Similarly, they may face inconvenience when trying to access remote controls for various devices at home, such as the TV or appliances.

This Proof-of-Concept leverages Google Home Matter technology to create a universal remote-control device capable of managing all devices within a home. This remote-control device can be either a mobile phone or a voice assistant device. It enables control over various smart home devices, including light bulbs, TVs, and other appliances.

[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](https://github.com/teamprof/github-we2-doorbell/blob/main/LICENSE)

<a href="https://www.buymeacoffee.com/teamprof" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 28px !important;width: 108px !important;" ></a>


## Sponsors
This project is sponsored by the following suppliers via [hackster.io](https://www.hackster.io/), please buy products from them for supporting this project. 
1. [DFRobot](https://www.hackster.io/dfrobot) 
2. [Seeed Studio](https://www.hackster.io/seeed)
3. [Blues](https://www.hackster.io/blues-wireless)
4. [Nordic Semiconductor](https://www.hackster.io/nordic-semiconductor)
5. [PCBWay](https://www.hackster.io/pcbway)
6. [ARM](https://www.hackster.io/arm)
7. [M5Stack](https://www.hackster.io/m5stack)


## Hardware
1. [DFRobot UNIHIKER](https://www.unihiker.com/): Central home panel
2. [XIAO ESP32S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html): Matter device
3. [nRF52840 DK](https://www.nordicsemi.com/Products/Development-hardware/nRF52840-DK): BLE device
4. [WS2812 LED module](https://www.aliexpress.com/item/1005006524124823.html)
5. [USB LED light](https://m.tb.cn/h.gObRwlZaGCvrZLJ?tk=QUZx3VGt8aU)
6. [Si2302 MOSFET](https://www.aliexpress.com/item/4001032242217.html)

### Connection between XIAO ESP32S3 Sense and WS2812
please refer to the schematic below for wiring between ESP32S3 and WS2812 LED  
[![ep32s3-led sch](/doc/image/esp32s3-ws2812-sch.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/esp32s3-ws2812-sch.png)

photo of physical connection
[![nrf52840-lamp pcb](/doc/image/esp32s3-ws2812-pcb.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/esp32s3-ws2812-pcb.png)


### Connection between nRF52840 DK, Si2302 and USB lamp
please refer to the schematic below for wiring between nRF52840 DK and USB lamp   
[![nrf52840-lamp sch](/doc/image/nrf52840-lamp-sch.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/nrf52840-lamp-sch.png)

photo of physical connection
[![nrf52840-lamp pcb](/doc/image/nrf52840-lamp-pcb.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/nrf52840-lamp-pcb.png)


## Software
### Python lib on DFRobot UNIHIKER
- http://circuitsframework.com/
- https://github.com/hbldh/bleak/tree/master

### SEEED XIAO ESP32S3 Sense
- VS Code with ESPRESSIF ESP-IDF  
  https://docs.espressif.com/projects/esp-idf/en/v4.2.3/esp32/get-started/vscode-setup.html 

### Nordic nRF52840 DK
- VS Code with Nordic nRF Connect  
  https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-VS-Code


# Getting Started
There are three sub-projects in this PoC. They are
1. Home panel runs on DFRobot UNIHIKER
2. Matter enabled fan device runs on SEEED XIAO ESP32S3 
3. BLE controllable lamp device runs on Nordic nRF52840 DK


# Build and Test

## DFRobot UNIHIKER home panel
1. clone this repository by "git clone https://github.com/teamprof/build2gether-smart-home"
2. follow the instruction on https://www.unihiker.com/wiki/connection to setup WiFi connection, and view its IP address
3. copy folder "src-unihiker" to UNIHIKER by typing the following command in Ubuntu terminal 
```
	scp -r src-unihiker root@<IP address from step(2)>:~/
	dfrobot
```
4. login UNIHIKER terminal by typing the following command in Ubuntu terminal
```
	ssh root@<IP address from step(2)>  
	dfrobot
```
5. setup mDNS by editing /etc/systemd/resolved.conf
```
    vi /etc/systemd/resolved.conf
      LLMNR=yes
      MulticastDNS=yes
```
6. start mDNS
```
   systemctl start systemd-resolved.service
   systemd-resolve --set-mdns=yes --interface=wlan0
```
7. install python lib by typing the following commands (note: it takes around 30 minutes on UNIHIKER)
```
	pip install circuits
	pip install bleak
```
8. run unihiker-home-panel by typing the following commands
```
	cd ~/src-unihiker
	python main.py
```
   if everything goes smooth, you should see the followings on UNIHIKER  
[![run unihiker](/doc/image/run-unihiker.jpg)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/run-unihiker.jpg)


## SEEED XIAO ESP32S3 Sense lamp (Matter device)
1. follow the instruction on https://docs.espressif.com/projects/esp-matter/en/latest/esp32s3/developing.html# to install ESSPRESIF ESP32S3 development tool for developing Matter application
2. clone this repository by "git clone https://github.com/teamprof/build2gether-smart-home"
3. launch VS Code to open the folder "src-esp32s3"
4. click ep-idf "build, flash, monitor" icon on the bottom bar  
   If everything goes smooth, you should see the followings on VS Code Output:
[![build/run esp32s3](/doc/image/build-run-esp32s3-mark.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/build-run-esp32s3-mark.png)
5. type the following commands on the monitor terminal
```
   matter device factoryreset
   matter esp wifi connect <wifi ssid> <wifi password>
   matter onboardingcodes onnetwork
```
6. if everything goes smooth, you should see the followings
[![runing esp32s3](/doc/image/running-esp32s3.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/running-esp32s3.png)
7. launch a Ubuntu terminal and type the following commands to pair (note: replace \<wifi ssid\> and \<wifi password\> with your WiFi SSID and password)
```
   chip-tool pairing onnetwork-long 0x7288 20202021 3840
```
8. if everything goes smooth, you should see the followings
[![pairing esp32s3](/doc/image/pairing-esp32s3.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/pairing-esp32s3.png)
9. reboot UNIHIKER, wait a couple seconds and you should see the following on the UNIHIKER screen
[![connect unihiker ble/wifi](/doc/image/connect-unihiker-wifi.jpg)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/connect-unihiker-wifi.jpg)
10. you can click the lamp icon on the upper portion, the LED connected to ESP32S3 Sense should be light on with sequence white -> yellow -> red -> off -> white -> ...
[![esp32s3 led white](/doc/image/led-esp32s3-white.jpg)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/led-esp32s3-white.jpg) 

[![esp32s3 led yellow](/doc/image/led-esp32s3-yellow.jpg)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/led-esp32s3-yellow.jpg) 
 
[![esp32s3 led red](/doc/image/led-esp32s3-red.jpg)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/led-esp32s3-red.jpg) 


11. ONLY the light onoff command is supported on the current version. type the following command to toggle the ESP32S3 LED via Matter protocol
```
	chip-tool onoff toggle 0x7288 1
```



## Nordic nRF52840 DK lamp (BLE device)
1. follow the instruction on https://docs.nordicsemi.com/bundle/nrf-connect-desktop/page/index.html to install Nordic development tool
2. clone this repository by "git clone https://github.com/teamprof/build2gether-smart-home"
3. launch VS Code to open the folder "src-nrf52840"
4. click nrf connect icon on the left panel
5. click "ACTIONS" -> "Build"  
   If everything goes smooth, you should see the followings on VS Code Output:
[![build nRF52840](/doc/image/build-nrf52840-mark.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/build-nrf52840-mark.png)
6. click "ACTIONS" -> "Flash"  
   If everything goes smooth, you should see the followings on VS Code Output:
[![flash nRF52840](/doc/image/flash-nrf52840-mark.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/flash-nrf52840-mark.png)
7. launch a serial terminal and connect to nRF52840 at 115200, 8N1
8. click the "Reset" button on the nRF52840 DK  
   If everything goes smooth, you should see the followings on the serial terminal:
[![run nRF52840](/doc/image/run-nrf52840.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/run-nrf52840.png)
9. wait couple seconds, UNIHIKER should connect nRF52840 DK and you should see the followings on UNIHIKER screen   
[![connect unihiker ble](/doc/image/connect-unihiker-ble.png)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/connect-unihiker-ble.png)
10. You can click the lamp icon on the bottom portion, the USB lamp connected to nRF52840 DK should be toggle on and off
[![nRF52840 lamp on](/doc/image/lamp-nrf52840-on.jpg)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/lamp-nrf52840-on.jpg)
 
[![nRF52840 lamp off](/doc/image/lamp-nrf52840-off.jpg)](https://github.com/teamprof/build2gether-smart-home/blob/main/doc/image/lamp-nrf52840-off.jpg)



## High level code explanation   
## DFRobot UNIHIKER home panel
There are two libraries used in the Python code running on UNIHIKER. The first one is Circuits (http://circuitsframework.com/) for the application framework, and the second one is Bleak (https://github.com/hbldh/bleak/tree/master) for BLE.  
In main.py, it spawns three tasks: AppGui, AppServerTcp, and AppClientBle for handling the GUI, WiFi/TCP connection, and BLE connection respectively. AppGui includes two components, LampEspWidget and LampNrfWidget, for controlling the ESP32S3 and nRF52840 respectively. Once user clicks the lamp icon in the widget, it sends an event to AppGui. AppGui forwards the event to main, which routes the event to AppServerTcp or AppClientBle for sending it via WiFi or BLE correspondingly.


## SEEED XIAO ESP32S3 Sense lamp (Matter device)
The ESP32S3 application leverages ArduProf (https://github.com/teamprof/arduprof) library to provide a generic message communication framework between the main (QueueMain) and ThreadPanel. QueueMain handles Matter event for Google Home while ThreadPanel handles WiFi/TCP event for UNIHIKER. Once event is received, QueueMain/ThreadPanel routes it to LightDevice to proceed the actual lamp operations (e.g. off/white/yellow/red).


## Nordic nRF52840 DK lamp (BLE device)
The nRF52840 application leverages ArduProf (https://github.com/teamprof/arduprof) library to provide a generic message communication framework between the MainTask and BLE callbacks. For example, once BLE LED characteristic is written, the "onLedChrcWrite" callback is involved and it sends an event to MainTask by "MainTask::getInstance()->postEvent(...)"


### Contributions and Thanks
Many thanks to the following companies who sponsor https://www.hackster.io
1. [DFRobot](https://www.hackster.io/dfrobot)
2. [Seeed Studio](https://www.hackster.io/seeed)
3. [Blues](https://www.hackster.io/blues-wireless)
4. [Nordic Semiconductor](https://www.hackster.io/nordic-semiconductor)
5. [PCBWay](https://www.hackster.io/pcbway)
6. [ARM](https://www.hackster.io/arm)
7. [M5Stack](https://www.hackster.io/m5stack)
8. [light bulb icons created by Good Ware - Flaticon](https://www.flaticon.com/free-icons/idea)
9. [cooling icon created by Freepik - Flaticon](https://www.flaticon.com/free-icons/cooling)  



#### Many thanks for everyone for bug reporting, new feature suggesting, testing and contributing to the development of this project.
---

### Contributing
If you want to contribute to this project:
- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library
---

### License
- The project is licensed under GNU GENERAL PUBLIC LICENSE Version 3
---

### Copyright
- Copyright 2024 teamprof.net@gmail.com. All rights reserved.

