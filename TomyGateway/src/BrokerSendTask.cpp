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
 *     Version: 0.1.0
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

	if(_res->getArgc() > 2){
		host = _res->getArgv()[2];
	}
	if(_res->getArgc() > 3){
		port = atoi(_res->getArgv()[3]);
	}

	while(true){

		uint16_t length = 0;
		memset(buffer, 0, SOCKET_MAXBUFFER_LENGTH);

		ev = _res->getBrokerSendQue()->wait();

		clnode = ev->getClientNode();
		srcMsg = clnode->getBrokerSendMessage();

		if(srcMsg->getType() == MQTT_TYPE_PUBLISH){
			D_MQTT("BrokerSendTask acquire MQTT_TYPE_PUBLISH\n");
			MQTTPublish* msg = static_cast<MQTTPublish*>(srcMsg);
			length = msg->serialize(buffer);
		}
		else if(srcMsg->getType() == MQTT_TYPE_PUBACK){
			D_MQTT("BrokerSendTask acquire MQTT_TYPE_PUBACK\n");
			MQTTPubAck* msg = static_cast<MQTTPubAck*>(srcMsg);
			length = msg->serialize(buffer);
		}
		else if(srcMsg->getType() == MQTT_TYPE_PINGREQ){
			D_MQTT("BrokerSendTask acquire MQTT_TYPE_PINGREQ\n");
			MQTTPingReq* msg = static_cast<MQTTPingReq*>(srcMsg);
			length = msg->serialize(buffer);
		}
		else if(srcMsg->getType() == MQTT_TYPE_SUBSCRIBE){
			D_MQTT("BrokerSendTask acquire MQTT_TYPE_SUBSCRIBE\n");
			MQTTSubscribe* msg = static_cast<MQTTSubscribe*>(srcMsg);
			length = msg->serialize(buffer);
		}
		else if(srcMsg->getType() == MQTT_TYPE_UNSUBSCRIBE){
			D_MQTT("BrokerSendTask acquire MQTT_TYPE_UNSUBSCRIBE\n");
			MQTTUnsubscribe* msg = static_cast<MQTTUnsubscribe*>(srcMsg);
			length = msg->serialize(buffer);
		}
		else if(srcMsg->getType() == MQTT_TYPE_CONNECT){
			D_MQTT("BrokerSendTask acquire MQTT_TYPE_CONNECT\n");
			MQTTConnect* msg = static_cast<MQTTConnect*>(srcMsg);
			length = msg->serialize(buffer);
		}
		else if(srcMsg->getType() == MQTT_TYPE_DISCONNECT){
			D_MQTT("BrokerSendTask acquire MQTT_TYPE_DISCONNECT\n");
			MQTTDisconnect* msg = static_cast<MQTTDisconnect*>(srcMsg);
			length = msg->serialize(buffer);
		}

		if(length > 0){
			if( clnode->getSocket()->isValid()){
				clnode->getSocket()->send(buffer, length);
			}else{
				clnode->getSocket()->create();
				if(clnode->getSocket()->connect(host, port)){
					clnode->getSocket()->send(buffer, length);
				}else{
					D_MQTT("%s Can't connect socket Client:%s\n",
							currentDateTime(), clnode->getNodeId()->c_str());
				}
			}
		}

		delete ev;
	}

}


