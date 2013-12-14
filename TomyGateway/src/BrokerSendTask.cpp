/*
 * BrokerSendTask.cpp
 *
 *               Copyright (c) 2013, tomy-tech.com
 *                       All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation 
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * 
 *  Created on: 2013/11/03
 *      Author: Tomoaki YAMAGUCHI
 *     Version: 1.0.0
 *
 */

#include "BrokerSendTask.h"
#include "GatewayResourcesProvider.h"
#include "GatewayDefines.h"
#include "libmq/ProcessFramework.h"
#include "libmq/Messages.h"
#include "libmq/Defines.h"
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

extern char* currentDateTime();


BrokerSendTask::BrokerSendTask(GatewayResourcesProvider* res){
	_res = res;
	_res->attach(this);
}

BrokerSendTask::~BrokerSendTask(){

}

void BrokerSendTask::run(){
	Event* ev = NULL;
	MQTTMessage* srcMsg = NULL;
	ClientNode* clnode = NULL;

	uint8_t buffer[SOCKET_MAXBUFFER_LENGTH];

	string host = BROKER_HOST_NAME;

	int port = BROKER_PORT;

	if(_res->getArgc() > ARGV_BROKER_ADDR){
		host = _res->getArgv()[ARGV_BROKER_ADDR];
	}
	if(_res->getArgc() > ARGV_BROKER_PORT){
		port = atoi(_res->getArgv()[ARGV_BROKER_PORT]);
	}

	while(true){

		uint16_t length = 0;
		memset(buffer, 0, SOCKET_MAXBUFFER_LENGTH);

		ev = _res->getBrokerSendQue()->wait();

		clnode = ev->getClientNode();
		srcMsg = clnode->getBrokerSendMessage();

		if(srcMsg->getType() == MQTT_TYPE_PUBLISH){
			MQTTPublish* msg = static_cast<MQTTPublish*>(srcMsg);
			length = msg->serialize(buffer);
			D_MQTT("     PUBLISH      >>>>    Broker    %s\n", msgPrint(buffer,msg));
		}
		else if(srcMsg->getType() == MQTT_TYPE_PUBACK){
			MQTTPubAck* msg = static_cast<MQTTPubAck*>(srcMsg);
			length = msg->serialize(buffer);
			D_MQTT("     PUBACK       >>>>    Broker    %s\n", msgPrint(buffer,msg));

		}
		else if(srcMsg->getType() == MQTT_TYPE_PINGREQ){
			MQTTPingReq* msg = static_cast<MQTTPingReq*>(srcMsg);
			length = msg->serialize(buffer);
			D_MQTT("     PINGREQ      >>>>    Broker    %s\n", msgPrint(buffer,msg));

		}
		else if(srcMsg->getType() == MQTT_TYPE_SUBSCRIBE){
			MQTTSubscribe* msg = static_cast<MQTTSubscribe*>(srcMsg);
			length = msg->serialize(buffer);
			D_MQTT("     SUBSCRIBE    >>>>    Broker    %s\n", msgPrint(buffer,msg));

		}
		else if(srcMsg->getType() == MQTT_TYPE_UNSUBSCRIBE){
			MQTTUnsubscribe* msg = static_cast<MQTTUnsubscribe*>(srcMsg);
			length = msg->serialize(buffer);
			D_MQTT("     UNSUBSCRIBE  >>>>    Broker    %s\n", msgPrint(buffer,msg));

		}
		else if(srcMsg->getType() == MQTT_TYPE_CONNECT){
			MQTTConnect* msg = static_cast<MQTTConnect*>(srcMsg);
			length = msg->serialize(buffer);
			D_MQTT("     CONNECT      >>>>    Broker    %s\n", msgPrint(buffer,msg));

		}
		else if(srcMsg->getType() == MQTT_TYPE_DISCONNECT){
			MQTTDisconnect* msg = static_cast<MQTTDisconnect*>(srcMsg);
			length = msg->serialize(buffer);
			D_MQTT("     DISCONNECT   >>>>    Broker    %s\n", msgPrint(buffer,msg));

		}

		int rc = 0;

		if(length > 0){
			if( clnode->getSocket()->isValid()){
				rc = clnode->getSocket()->send(buffer, length);
				if(rc == -1){
					clnode->getSocket()->disconnect();
					D_MQTT("       Socket is valid. but can't send Client:%s\n", clnode->getNodeId()->c_str());
				}
			}else{
				if(clnode->getSocket()->create()){
					if(clnode->getSocket()->connect(host, port)){
						rc = clnode->getSocket()->send(buffer, length);
						if(rc == -1){
							clnode->getSocket()->disconnect();
							D_MQTT("       Socket is created. but can't send, Client:%s\n", clnode->getNodeId()->c_str());
						}
					}else{
						D_MQTT("%s Can't connect socket Client:%s\n",
								currentDateTime(), clnode->getNodeId()->c_str());
					}
				}else{
					D_MQTT("%s Can't create socket Client:%s\n",
						currentDateTime(), clnode->getNodeId()->c_str());
				}
			}
		}

		delete ev;
	}
}


char*  BrokerSendTask::msgPrint(uint8_t* buffer, MQTTMessage* msg){
	char* buf = _printBuf;

	sprintf(buf, " 0x%02X", *buffer);
	buf += 5;

	for(int i = 0; i < msg->getRemainLength(); i++){
		sprintf(buf, " 0x%02X", *( buffer + 1 + msg->getRemainLengthSize() + i));
		buf += 5;
	}
	*buf = 0;
	return _printBuf;
}

