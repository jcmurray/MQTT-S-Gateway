MQTT-S Gateway over XBee 
======
*  A gateway for MQTT-S  is under developing.

Supported functions
-------------------

*  QOS Level 0 and 1
*  CONNECT, WILLTOPICREQ, WILLTOPIC, WILLMSGREQ, WILLMSG
*  REGISTER, SUBSCRIBE, PUBLISH, UNSUBSCRIBE, DISCONNECT 
*  CONNACK, REGACK, SUBACK, PUBACK, UNSUBACK
*  ADVERTIZE, GWINFO 

Usage
------
####Minimum requirements
  Three XBee S2 devices,  one coordinator, one gateway and one client.

#### Start Gateway  
    argument 1: Device which XBee dongle connected
    argument 2: Broker IP Address ( default value "localhost" )
    argument 3: Port No  ( default value 1883 )
    
    $ TomyGateway /dev/ttyUSB0  ["localhost"] [1883]
         
XBee configurations
----------------------
  Serial interfacing  of gateway.  
  Coordinator is default setting.
  
    [BD] 6   57600 bps
    [D7] 1   CTS-Flow Control
    [D6] 1   RTS-Flow Control
    [AP] 2   API Enable

  Other values are defaults.
