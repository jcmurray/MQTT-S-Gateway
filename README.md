MQTT-S Gateway over XBee 
======
 
  _Documents in TomyGateway directory will help you to understand the architecture of this program._     
  Raspberry Pi can run this Gateway.  
  http://www.youtube.com/watch?v=INa5YznfR-8&feature=youtu.be    
  
  ![outlook](https://github.com/TomoakiYAMAGUCHI/MQTT-S-Gateway/blob/master/TomyGateway/documents/MQTT-S_outlook.PNG?raw=true)
  
Supported functions
-------------------

*  QOS Level 0 and 1
*  CONNECT, WILLTOPICREQ, WILLTOPIC, WILLMSGREQ, WILLMSG
*  REGISTER, SUBSCRIBE, PUBLISH, UNSUBSCRIBE, DISCONNECT 
*  CONNACK, REGACK, SUBACK, PUBACK, UNSUBACK
*  ADVERTIZE, GWINFO 

Usage
------
####1) Minimum requirements
*  Three XBee S2 devices,  one coordinator, one gateway and one client.
*  Linux  ( Windows can not execute this program.)
*  pthread, rt liblaries to be linked.

####2) How to Build

    $ make
    
  Makefile is in TomyGateway directory.  
  TomyGateway (Executable) is created in Build directory.
    
####3)  Start Gateway  
    
    $ TomyGateway 1 /dev/ttyUSB0  ["localhost"] [1883]
    
  argv 1: Gateway Id (numeric 1 - 255)  
  argv 2: Device which XBee dongle connected  
  argv 3: Broker IP Address ( default value "localhost" )  
  argv 4: Port No  ( default value 1883 )  
         
XBee configurations
----------------------
  Serial interfacing  of gateway.  
  Coordinator is default setting.
  
    [BD] 6   57600 bps
    [D7] 1   CTS-Flow Control
    [D6] 1   RTS-Flow Control
    [AP] 2   API Enable

  Other values are defaults.
  
RaspberryPi instalation
----------------------
####1)  Prepare Bootup SD card  
*  Download img.zip file from    
   
*  Unzip archilinux-mqtt-sn-gateway4RaspberryPi.img.zip  
*  Copy img file to SD card (4GB)  

####2)  How to connect XBee to RaspberryPi
        RaspberryPi           XBee   
        3.3V  Pin 1  ----------  Pin 1  Vcc   
        GND   Pin 6  ----------  Pin 10 GND    
        Tx    Pin 8  ----------  Pin 3  Rx    
        Rx    Pin 10 ----------  Pin 2  Tx    

####3)  How to start 

*  Boot up RaspberyPi & login via ssh.  
    $ ssh 'RaspberryPi IP Address' -p 22022 -l gw  
      	password is gw.  
      	Raspberry IP address is asigned by DHCP.  
*  Change gw password. (gw can use sudo command) 
      	root's password is root.
*  Invoke Gateway 
    [gw@MQTT-SnGateway01~]$ ./TomyGateway 1 /dev/ttyAMA0 85.119.83.194 1883
    	IP address 85.119.83.194 is test.mosquito.org.

  
  
  
###Contact


* Author:    Tomoaki YAMAGUCHI
* Email:     tomoaki@tomy-tech.com
* Twitter:   [@ty4tw]

[@ty4tw]:     http://twitter.com/ty4tw

