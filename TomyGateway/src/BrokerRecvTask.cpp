/*
 * BrokerRecvTask.cpp
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
 *     Version:
 *
 */

#include "BrokerRecvTask.h"
#include "GatewayResourcesProvider.h"
#include "GatewayDefines.h"
#include "libmq/ProcessFramework.h"
#include "libmq/Messages.h"
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


BrokerRecvTask::BrokerRecvTask(GatewayResourcesProvider* res){
	_res = res;
	_res->attach(this);
}

BrokerRecvTask::~BrokerRecvTask(){

}

void BrokerRecvTask::run(){

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;    // 500 msec

	ClientList* clist = _res->getClientList();
	fd_set readfds;
	int sock;

	while(true){
		FD_ZERO(&readfds);
		int maxSock = 0;

		/*------- Prepare socket list to check -------*/
		for( int i = 0; i < clist->getClientCount(); i++){
			if((*clist)[i]){
				if((*clist)[i]->getSocket()->isValid()){
					sock = (*clist)[i]->getSocket()->getSock();
					FD_SET(sock, &readfds);

					if(sock > maxSock){
						maxSock = sock;
					}
				}
			}else{
				break;
			}
		}

		/*------- Check socket to receive -------*/
		int activity =  select( maxSock + 1 , &readfds , NULL , NULL , &timeout);

		if (activity > 0){
			for( int i = 0; i < clist->getClientCount(); i++){
				if((*clist)[i]){
					if((*clist)[i]->getSocket()->isValid()){
						int sock = (*clist)[i]->getSocket()->getSock();
						if(FD_ISSET(sock, &readfds)){

							recvAndFireEvent((*clist)[i]);
						}
					}
				}else{
					break;
				}
			}
		}
	}
}


/*-----------------------------------------
     Recv socket & Create MQTT Messages
 -----------------------------------------*/
void BrokerRecvTask::recvAndFireEvent(ClientNode* clnode){
	uint8_t buffer[SOCKET_MAXBUFFER_LENGTH];
	memset(buffer, 0, SOCKET_MAXBUFFER_LENGTH);
	RemainingLength remLen;
	bool match = true;

	int cnt = clnode->getSocket()->recv(buffer, SOCKET_MAXBUFFER_LENGTH);
	if(cnt){
		switch(buffer[0] & 0xf0){
			case MQTT_TYPE_PUBACK:{
				D_MQTT("BrokerRecvTask acquires MQTT_TYPE_PUBACK\n");
				MQTTPubAck* puback = new MQTTPubAck();
				puback->deserialize(buffer);
				clnode->setBrokerRecvMessage(puback);
			}
				break;
			case MQTT_TYPE_PUBLISH:{
				D_MQTT("BrokerRecvTask acquires MQTT_TYPE_PUBLISH\n");

				MQTTPublish* publish = new MQTTPublish();
				publish->deserialize(buffer);
				clnode->setBrokerRecvMessage(publish);
			}
				break;
			case MQTT_TYPE_SUBACK:{
				D_MQTT("BrokerRecvTask acquires MQTT_TYPE_SUBACK\n");

				MQTTSubAck* suback = new MQTTSubAck();
				suback->deserialize(buffer);
				clnode->setBrokerRecvMessage(suback);
			}
				break;
			case MQTT_TYPE_PINGRESP:{
				D_MQTT("BrokerRecvTask acquires MQTT_TYPE_PINGRESP\n");

				MQTTPingResp* pingresp = new MQTTPingResp();
				pingresp->deserialize(buffer);
				clnode->setBrokerRecvMessage(pingresp);
			}
				break;
			case MQTT_TYPE_UNSUBACK:{
				D_MQTT("BrokerRecvTask acquires MQTT_TYPE_UNSUBACK\n");

				MQTTUnsubAck* unsuback = new MQTTUnsubAck();
				unsuback->deserialize(buffer);
				clnode->setBrokerRecvMessage(unsuback);
			}
				break;
			case MQTT_TYPE_CONNACK:{
				D_MQTT("BrokerRecvTask acquires MQTT_TYPE_CONNACK\n");

				MQTTConnAck* connack = new MQTTConnAck();
				connack->deserialize(buffer);
				clnode->setBrokerRecvMessage(connack);
			}
				break;
			default:
				match = false;
				break;
		}
		if(match){
			Event* ev = new Event();
			ev->setBrokerRecvEvent(clnode);
			_res->getGatewayEventQue()->post(ev);
		}
	}else{
		clnode->getSocket()->disconnect();
	}
}

