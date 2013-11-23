MQTT-S Gateway over XBee 
======
  _A gateway for MQTT-S  is under developing._  
  _Current version is just for reference, but can run._

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
    
    $ TomyGateway /dev/ttyUSB0  ["localhost"] [1883]
    
  argv 1: Device which XBee dongle connected  
  argv 2: Broker IP Address ( default value "localhost" )  
  argv 3: Port No  ( default value 1883 )  
         
XBee configurations
----------------------
  Serial interfacing  of gateway.  
  Coordinator is default setting.
  
    [BD] 6   57600 bps
    [D7] 1   CTS-Flow Control
    [D6] 1   RTS-Flow Control
    [AP] 2   API Enable

  Other values are defaults.
  
  
  
  
  
###Contact


* Author:    Tomoaki YAMAGUCHI
* Email:     tomoaki@tomy-tech.com

