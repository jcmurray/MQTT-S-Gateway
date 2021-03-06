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
 *     Version: 1.0.0
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

extern char* currentDateTime();

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
	int sockfd;

	while(true){
		FD_ZERO(&readfds);
		int maxSock = 0;

		/*------- Prepare socket list to check -------*/
		for( int i = 0; i < clist->getClientCount(); i++){
			if((*clist)[i]){
				if((*clist)[i]->getSocket()->isValid()){
					sockfd = (*clist)[i]->getSocket()->getSock();
					FD_SET(sockfd, &readfds);

					if(sockfd > maxSock){
						maxSock = sockfd;
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
						int sockfd = (*clist)[i]->getSocket()->getSock();
						if(FD_ISSET(sockfd, &readfds)){

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

	uint8_t sbuff[SOCKET_MAXBUFFER_LENGTH * 5];
	uint8_t buffer[SOCKET_MAXBUFFER_LENGTH];
	memset(buffer, 0, SOCKET_MAXBUFFER_LENGTH);

	uint8_t* packet = buffer;

	int recvLength = clnode->getSocket()->recv(packet, SOCKET_MAXBUFFER_LENGTH);

	if (recvLength == -1){
		D_MQTT(" Client : %s Broker Connection Error\n", clnode->getNodeId()->c_str());
		clnode->updateStatus(Cstat_Disconnected);
	}

	while(recvLength > 0){

		if((*packet & 0xf0) == MQTT_TYPE_PUBACK){
			MQTTPubAck* puback = new MQTTPubAck();
			puback->deserialize(packet);
			puback->serialize(sbuff);
			D_MQTT("%s PUBACK       <---    Broker    %s\n", currentDateTime(), msgPrint(sbuff, puback));

			clnode->setBrokerRecvMessage(puback);

		}else if((*packet & 0xf0) == MQTT_TYPE_PUBLISH){
			MQTTPublish* publish = new MQTTPublish();
			publish->deserialize(packet);
			publish->serialize(sbuff);
			D_MQTT("\n%s PUBLISH      <---    Broker    %s\n", currentDateTime(), msgPrint(sbuff, publish));

			clnode->setBrokerRecvMessage(publish);

		}else if((*packet & 0xf0) == MQTT_TYPE_SUBACK){
			MQTTSubAck* suback = new MQTTSubAck();
			suback->deserialize(packet);
			suback->serialize(sbuff);
			D_MQTT("%s SUBACK       <---    Broker    %s\n", currentDateTime(), msgPrint(sbuff, suback));

			clnode->setBrokerRecvMessage(suback);

		}else if((*packet & 0xf0) == MQTT_TYPE_PINGRESP){
			MQTTPingResp* pingresp = new MQTTPingResp();
			pingresp->deserialize(packet);
			pingresp->serialize(sbuff);
			D_MQTT("%s PINGRESP     <---    Broker    %s\n", currentDateTime(), msgPrint(sbuff, pingresp));

			clnode->setBrokerRecvMessage(pingresp);

		}else if((*packet & 0xf0) == MQTT_TYPE_UNSUBACK){
			MQTTUnsubAck* unsuback = new MQTTUnsubAck();
			unsuback->deserialize(packet);
			unsuback->serialize(sbuff);
			D_MQTT("%s UNSUBACK     <---    Broker    %s\n", currentDateTime(), msgPrint(sbuff, unsuback));

			clnode->setBrokerRecvMessage(unsuback);

		}else if((*packet & 0xf0) == MQTT_TYPE_CONNACK){
			MQTTConnAck* connack = new MQTTConnAck();
			connack->deserialize(packet);
			connack->serialize(sbuff);
			D_MQTT("%s CONNACK      <---    Broker    %s\n", currentDateTime(), msgPrint(sbuff, connack));

			clnode->setBrokerRecvMessage(connack);

		}else{
			D_MQTT("%s UNKOWN_TYPE  packetLength=%d\n",currentDateTime(), recvLength);
			return;
		}

		Event* ev = new Event();
		ev->setBrokerRecvEvent(clnode);
		_res->getGatewayEventQue()->post(ev);

		RemainingLength rl;
		rl.deserialize(packet + 1);
		uint16_t  packetLength = 1 + rl.getSize() + rl.decode();
		recvLength -= packetLength;
		packet += packetLength;
	}
}


char*  BrokerRecvTask::msgPrint(uint8_t* buffer, MQTTMessage* msg){
	char* buf = _printBuf;

	sprintf(buf, " %02X", *buffer);
	buf += 3;

	for(int i = 0; i < msg->getRemainLength(); i++){
		sprintf(buf, " %02X", *( buffer + 1 + msg->getRemainLengthSize() + i));
		buf += 3;
	}
	*buf = 0;
	return _printBuf;
}
