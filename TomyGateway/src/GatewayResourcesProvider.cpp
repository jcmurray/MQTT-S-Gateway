/*
 * GatewayResourcesProvider.cpp
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
#include "GatewayResourcesProvider.h"
#include "GatewayDefines.h"
#include "libmq/ProcessFramework.h"
#include <iostream>

using namespace std;

extern Process* theProcess;

GatewayResourcesProvider* theGatewayResources = NULL;

/*=====================================
     Class GatewayResourcesProvider
 =====================================*/
GatewayResourcesProvider::GatewayResourcesProvider(): MultiTaskProcess(){
	theMultiTask = this;
	theProcess = this;
}

GatewayResourcesProvider::~GatewayResourcesProvider(){

}

EventQue<Event>* GatewayResourcesProvider::getGatewayEventQue(){
	return &_gatewayEventQue;
}

EventQue<Event>* GatewayResourcesProvider::getClientSendQue(){
	return &_clientSendQue;
}

EventQue<Event>* GatewayResourcesProvider::getBrokerSendQue(){
	return &_brokerSendQue;
}

ClientList* GatewayResourcesProvider::getClientList(){
	return &_clientList;
}

/*=====================================
        Class Client
 =====================================*/
ClientNode::ClientNode(){
	_msgId = 0;
	_snMsgId = 0;
	_status = Cstat_Disconnected;
	_topics = new Topics();

	_address64 = XBeeAddress64();
	_address16 = 0;

	_mqttConnect = 0;

	_waitedPubAck = 0;
	_waitedSubAck = 0;
}

ClientNode::~ClientNode(){
	_socket.disconnect();
	delete _topics;
}

void ClientNode::setWaitedPubAck(MQTTSnPubAck* msg){
	_waitedPubAck = msg;
}

void ClientNode::setWaitedSubAck(MQTTSnSubAck* msg){
	_waitedSubAck = msg;
}

MQTTSnPubAck* ClientNode::getWaitedPubAck(){
	return _waitedPubAck;
}

MQTTSnSubAck* ClientNode::getWaitedSubAck(){
	return _waitedSubAck;
}

uint16_t ClientNode::getNextMessageId(){
	_msgId++;
	if (_msgId == 0){
		_msgId++;
	}
	return _msgId;
}

uint8_t ClientNode::getNextSnMsgId(){
	_snMsgId++;
		if (_snMsgId == 0){
			_snMsgId++;
		}
		return _snMsgId;
}


MQTTMessage* ClientNode::getBrokerSendMessage(){
	return _brokerSendMessageQue.getMessage();
}

MQTTMessage* ClientNode::getBrokerRecvMessage(){
	return _brokerRecvMessageQue.getMessage();
}

MQTTSnMessage* ClientNode::getClientSendMessage(){
	return _clientSendMessageQue.getMessage();
}

MQTTSnMessage* ClientNode::getClientRecvMessage(){
	return _clientRecvMessageQue.getMessage();
}

MQTTConnect*   ClientNode::getConnectMessage(){
	return _mqttConnect;
}

Socket* ClientNode::getSocket(){
	return &_socket;
}


void ClientNode::setBrokerSendMessage(MQTTMessage* msg){
	_brokerSendMessageQue.push(msg);
}

void ClientNode::setBrokerRecvMessage(MQTTMessage* msg){
	_brokerRecvMessageQue.push(msg);
}

void ClientNode::setClientSendMessage(MQTTSnMessage* msg){
	updateStatus(msg);
	_clientSendMessageQue.push(msg);

}

void ClientNode::setClientRecvMessage(MQTTSnMessage* msg){
	updateStatus(msg);
	_clientRecvMessageQue.push(msg);
}

void ClientNode::setConnectMessage(MQTTConnect* msg){
	_mqttConnect = msg;
}


void ClientNode::checkTimeover(){
	if(_status == Cstat_Active && _keepAliveTimer.isTimeup()){
		_status = Cstat_Lost;
	}
}

void ClientNode::setKeepAlive(MQTTSnMessage* msg){
	MQTTSnConnect* cm = static_cast<MQTTSnConnect*>(msg);
	_keepAliveMsec = cm->getDuration() * 1000;
	_keepAliveTimer.start(_keepAliveMsec * 1.5);
}

void ClientNode::updateStatus(ClientStatus stat){
	_status = stat;
}

void ClientNode::updateStatus(MQTTSnMessage* msg){
	if(((_status == Cstat_Disconnected) || (_status == Cstat_Lost)) && 
         msg->getType() == MQTTSN_TYPE_CONNECT){
		setKeepAlive(msg);
		_status = Cstat_Connecting;
		
	}else if(_status == Cstat_Connecting && msg->getType() == MQTTSN_TYPE_CONNACK){
		_status = Cstat_Active;
	}else if(_status == Cstat_Active){
		switch(msg->getType()){
		case MQTTSN_TYPE_PINGREQ:
		case MQTTSN_TYPE_PUBLISH:
		case MQTTSN_TYPE_SUBSCRIBE:
		case MQTTSN_TYPE_UNSUBSCRIBE:
			 _keepAliveTimer.start(_keepAliveMsec * 1.5);
			 break;
		case MQTTSN_TYPE_DISCONNECT:{
			MQTTSnDisconnect* dcm = static_cast<MQTTSnDisconnect*>(msg);
			if(dcm->getDuration()){
				_status = Cstat_Asleep;
				_keepAliveMsec = dcm->getDuration() * 1000;
			}else{
				_status = Cstat_Disconnected;
			}
		}
			break;
		default:
			break;
		}
	
	}else if(_status == Cstat_Asleep){
		if(msg->getType() == MQTTSN_TYPE_CONNECT){
			setKeepAlive(msg);
			_status = Cstat_Connecting;
		}else if( msg->getType() == MQTTSN_TYPE_PINGREQ ){
			MQTTSnPingReq* pr = static_cast<MQTTSnPingReq*>(msg);
			if(pr->getClientId()) {
				_status = Cstat_Awake;
			}
		}
	}else if(_status == Cstat_Awake){
		switch(msg->getType()){
			case MQTTSN_TYPE_CONNECT:
				_status = Cstat_Connecting;
				setKeepAlive(msg);
				break;
			case MQTTSN_TYPE_DISCONNECT:
				_status = Cstat_Disconnected;
				break;
			case MQTTSN_TYPE_PINGRESP:
				_status = Cstat_Asleep;
				break;
			default:
				break;
		}
	}
}

void ClientNode::deleteBrokerSendMessage(){
	_brokerSendMessageQue.pop();
}

void ClientNode::deleteBrokerRecvMessage(){
	_brokerRecvMessageQue.pop();
}

void ClientNode::deleteClientSendMessage(){
	_clientSendMessageQue.pop();
}

void ClientNode::deleteClientRecvMessage(){
	_clientRecvMessageQue.pop();
}

Topics* ClientNode::getTopics(){
	return _topics;
}


XBeeAddress64* ClientNode::getAddress64Ptr(){
	return &_address64;
}

uint16_t ClientNode::getAddress16(){
    return _address16;
}

UTFString* ClientNode::getNodeId(){
    return &_nodeId;
}

void ClientNode::setMsb(uint32_t msb){
    _address64.setMsb(msb);
}

void ClientNode::setLsb(uint32_t lsb){
    _address64.setLsb(lsb);
}

void ClientNode::setAddress64(XBeeAddress64* addr){
	 setMsb(addr->getMsb());
	 setLsb(addr->getLsb());
}

void ClientNode::setAddress16(uint16_t addr){
    _address16 = addr;
}

void ClientNode::setNodeId(UTFString* id){
	_nodeId = *id;
}

void ClientNode::setTopics(Topics* topics){
	_topics = topics;
}


/*=====================================
        Class ClientList
 =====================================*/
ClientList::ClientList(){
	_clientVector = new vector<ClientNode*>();
	_clientVector->reserve(MAX_CLIENT_NODES);
	_clientCnt = 0;
}

ClientList::~ClientList(){
	_mutex.lock();
	vector<ClientNode*>::iterator client = _clientVector->begin();
	while((!_clientVector->empty()) && *client){
		delete(*client);
		_clientVector->erase(client);
	}
	_mutex.unlock();
}

ClientNode* ClientList::createNode(XBeeAddress64* addr64, uint16_t addr16){
	if(_clientCnt < MAX_CLIENT_NODES){
		_mutex.lock();
		vector<ClientNode*>::iterator client = _clientVector->begin();
		while( client != _clientVector->end()){
			if((*client)->getAddress16() == addr16){
				return NULL;
			}else{
				++client;
			}
		}
		ClientNode* node = new ClientNode();
		node->setAddress16(addr16);
		node->setAddress64(addr64);
		_clientVector->push_back(node);
		_clientCnt++;
		_mutex.unlock();
		return node;
	}else{
		return NULL;
	}
}

void ClientList::erase(ClientNode* clnode){

	uint16_t pos = 0;
	_mutex.lock();
	vector<ClientNode*>::iterator client = _clientVector->begin();

	while( (client != _clientVector->end()) && *client){

		if((*client) == clnode){
			delete(*client);
			_clientVector->erase(client);
			_clientCnt--;
			for(; pos < _clientCnt; pos++){
				_clientVector[pos] = _clientVector[pos + 1];
			}
		}else{
			++client;
			++pos;
		}
	}
	_mutex.unlock();
}

ClientNode* ClientList::getClient(uint16_t address16){
	_mutex.lock();
	vector<ClientNode*>::iterator client = _clientVector->begin();
	while( (client != _clientVector->end()) && *client){
			if((*client)->getAddress16() == address16){
				_mutex.unlock();
				return *client;
			}else{
				++client;
			}
	}
	_mutex.unlock();
	return NULL;
}

uint16_t ClientList::getClientCount(){
	return _clientCnt;
}

ClientNode* ClientList::operator[](int pos){
	_mutex.lock();
	ClientNode* node = (*_clientVector)[pos];
	_mutex.unlock();
	return node;
}


/*=====================================
        Class Event
 =====================================*/
Event::Event(){
	_eventType = Et_NA;
	_clientNode = NULL;
	_mqttSnMessage = NULL;
}

Event::Event(EventType type){
	_eventType = type;
	_clientNode = NULL;
	_mqttSnMessage = NULL;
}

Event::~Event(){
	switch(_eventType){
	case EtClientRecv:
		if(_clientNode){
			_clientNode->deleteClientRecvMessage();
		}
		break;
	case EtClientSend:
			if(_clientNode){
				_clientNode->deleteClientSendMessage();
			}
			break;
	case EtBrokerRecv:
			if(_clientNode){
				_clientNode->deleteBrokerRecvMessage();
			}
			break;
	case EtBrokerSend:
			if(_clientNode){
				_clientNode->deleteBrokerSendMessage();
			}
			break;
	case EtBroadcast:
		if(_mqttSnMessage){
			delete _mqttSnMessage;
		}
		break;
	default:     // Et_NA, EtTimeout
		break;
	}
}

EventType Event::getEventType(){
	return  _eventType;
}

void Event::setClientSendEvent(ClientNode* client){
	_clientNode = client;
	_eventType = EtClientSend;
}

void Event::setClientRecvEvent(ClientNode* client){
	_clientNode = client;
	_eventType = EtClientRecv;
}

void Event::setBrokerSendEvent(ClientNode* client){
	_clientNode = client;
	_eventType = EtBrokerSend;
}

void Event::setBrokerRecvEvent(ClientNode* client){
	_clientNode = client;
	_eventType = EtBrokerRecv;
}

void Event::setTimeout(){
	_eventType = EtTimeout;
}

void Event::setEvent(MQTTSnMessage* msg){
	_mqttSnMessage = msg;
	_eventType = EtBroadcast;
}


ClientNode* Event::getClientNode(){
	return _clientNode;
}


MQTTSnMessage* Event::getMqttSnMessage(){
	return _mqttSnMessage;
}


