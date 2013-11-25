/*
 * ClientRecvTask.cpp
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
 *  Created on: 2013/10/13
 *      Author: Tomoaki YAMAGUCHI
 *     Version: 0.1.0
 *
 */
#include "ClientRecvTask.h"
#include "GatewayResourcesProvider.h"
#include "libmq/ProcessFramework.h"
#include "libmq/Messages.h"
#include "libmq/ZBStack.h"
#include "libmq/ErrorMessage.h"


#include <unistd.h>
#include <iostream>
#include <fcntl.h>


ClientRecvTask::ClientRecvTask(GatewayResourcesProvider* res){
	_res = res;
	_res->attach(this);
}

ClientRecvTask::~ClientRecvTask(){

}

void ClientRecvTask::run(){

	if(_sp.begin(_res->getArgv()[1], B57600, O_RDONLY) == -1){
		THROW_EXCEPTION(ExFatal, ERRNO_SYS_02, "can't open device.");
	}
	_zb.setSerialPort(&_sp);

	XBResponse* resp = new XBResponse();

	while(true){

		Event* ev = new Event();

		bool eventSetFlg = true;

		if(_zb.getResponse(resp)){

			ClientNode* clnode = _res->getClientList()->getClient(resp->getRemoteAddress16());

			if(!clnode){
				if(resp->getPayloadPtr()[1] == MQTTSN_TYPE_CONNECT){
					ClientNode* node = _res->getClientList()->createNode(resp->getRemoteAddress64(),resp->getRemoteAddress16());
					if(!node){
						delete ev;
						THROW_EXCEPTION(ExWarn, ERRNO_SYS_04, "can't create a clientNode.");
					}
					MQTTSnConnect* msg = new MQTTSnConnect();
					msg->absorb(resp);
					node->setAddress64(resp->getRemoteAddress64());
					node->setAddress16(resp->getRemoteAddress16());
					node->setNodeId(msg->getClientId());
					node->setClientRecvMessage(msg);

					ev->setClientRecvEvent(node);
				}else if(resp->getPayloadPtr()[1] == MQTTSN_TYPE_SEARCHGW){
					MQTTSnSearchGw* msg = new MQTTSnSearchGw();
					msg->absorb(resp);
					ev->setEvent(msg);

				}else{
					eventSetFlg = false;
				}
			}else{
				if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_CONNECT){
					MQTTSnConnect* msg = new MQTTSnConnect();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if(resp->getPayloadPtr()[1] == MQTTSN_TYPE_PUBLISH){
					MQTTSnPublish* msg = new MQTTSnPublish();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if(resp->getPayloadPtr()[1] == MQTTSN_TYPE_PUBACK){
					MQTTSnPubAck* msg = new MQTTSnPubAck();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_CONNECT){
					MQTTSnConnect* msg = new MQTTSnConnect();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_PINGREQ){
					MQTTSnPingReq* msg = new MQTTSnPingReq();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_DISCONNECT){
					MQTTSnDisconnect* msg = new MQTTSnDisconnect();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_REGISTER){
					MQTTSnRegister* msg = new MQTTSnRegister();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_REGACK){
					MQTTSnRegAck* msg = new MQTTSnRegAck();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_UNSUBSCRIBE){
					MQTTSnUnsubscribe* msg = new MQTTSnUnsubscribe();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_SUBSCRIBE){
					MQTTSnSubscribe* msg = new MQTTSnSubscribe();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_WILLTOPIC){
					MQTTSnWillTopic* msg = new MQTTSnWillTopic();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if (resp->getPayloadPtr()[1] == MQTTSN_TYPE_WILLMSG){
					MQTTSnWillMsg* msg = new MQTTSnWillMsg();
					msg->absorb(resp);
					clnode->setClientRecvMessage(msg);
					ev->setClientRecvEvent(clnode);

				}else if(resp->getPayloadPtr()[1] == MQTTSN_TYPE_SEARCHGW){
					MQTTSnSearchGw* msg = new MQTTSnSearchGw();
					msg->absorb(resp);
					ev->setEvent(msg);
				}else{
					eventSetFlg = false;
				}
			}
			if(eventSetFlg){
				_res->getGatewayEventQue()->post(ev);
			}else{
				delete ev;
			}
		}
	}
	delete resp;
}





