MQTT-S Gateway over XBee 
======
  A gateway for MQTT-S  is under developing.

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

#### Start Gateway  (arguments: device which XBee dongle connected.)  
    argument1: Device which XBee dongle connected
    argument2: Broker IP Address ( default value "localhost" )
    argument3: Port No  ( default value 1883 )
    
    $ TomyGateway /dev/ttyUSB0  ["localhost"] [1883]
         
