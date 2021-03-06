/*
 * GatewayResourcesProvider.h
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
 *  Created on: 2013/10/19
 *  Updated on: 2014/03/20
 *      Author: Tomoaki YAMAGUCHI
 *     Version: 2.0.0
 *
 */

#ifndef GATEWAY_RESOURCES_PROVIDER_H_
#define GATEWAY_RESOURCES_PROVIDER_H_

#include "libmq/ProcessFramework.h"
#include "libmq/Messages.h"
#include "libmq/Socket.h"
#include "libmq/Topics.h"

#define ARGV_GATEWAY_ID  1
#define ARGV_DEVICE_NAME 2
#define ARGV_BROKER_ADDR 3
#define ARGV_BROKER_PORT 4


#define FILE_NAME_CLIENT_LIST "config/clientList.conf"
/*=====================================
        Class MessageQue
 =====================================*/
template<class T> class MessageQue{
public:
	MessageQue();
	~MessageQue();
	T* getMessage();
	void push(T*);
	void pop();
private:
	queue<T*> _que;
	Mutex  _mutex;
};


enum ClientStatus {
	Cstat_Disconnected = 0,
	Cstat_Connecting,
	Cstat_Active,
	Cstat_Asleep,
	Cstat_Awake,
	Cstat_Lost
};

/*=====================================
        Class ClientNode
 =====================================*/
class ClientNode{
public:
	ClientNode();
	~ClientNode();

	MQTTMessage*   getBrokerSendMessage();
	MQTTMessage*   getBrokerRecvMessage();
	MQTTSnMessage* getClientSendMessage();
	MQTTSnMessage* getClientRecvMessage();
	MQTTConnect*   getConnectMessage();
	MQTTSnPubAck*  getWaitedPubAck();
	MQTTSnSubAck*  getWaitedSubAck();


	void setBrokerSendMessage(MQTTMessage*);
	void setBrokerRecvMessage(MQTTMessage*);
	void setClientSendMessage(MQTTSnMessage*);
	void setClientRecvMessage(MQTTSnMessage*);
	void setConnectMessage(MQTTConnect*);
	void setWaitedPubAck(MQTTSnPubAck* msg);
	void setWaitedSubAck(MQTTSnSubAck* msg);

	void deleteBrokerSendMessage();
	void deleteBrokerRecvMessage();
	void deleteClientSendMessage();
	void deleteClientRecvMessage();

	void checkTimeover();
	void updateStatus(MQTTSnMessage*);
	void updateStatus(ClientStatus);
	uint16_t getNextMessageId();
	uint8_t getNextSnMsgId();
	Topics* getTopics();

	Socket* getSocket();
	XBeeAddress64* getAddress64Ptr();
	uint16_t  getAddress16();
	string* getNodeId();
	void setMsb(uint32_t);
	void setLsb(uint32_t);
	void setAddress16(uint16_t addr);
	void setAddress64(XBeeAddress64* addr);
	void setTopics(Topics* topics);
	void setNodeId(string* id);



private:
	void setKeepAlive(MQTTSnMessage* msg);

	MessageQue<MQTTMessage>   _brokerSendMessageQue;
	MessageQue<MQTTMessage>   _brokerRecvMessageQue;
	MessageQue<MQTTSnMessage> _clientSendMessageQue;
	MessageQue<MQTTSnMessage> _clientRecvMessageQue;

	MQTTConnect*   _mqttConnect;

	MQTTSnPubAck*  _waitedPubAck;
	MQTTSnSubAck*  _waitedSubAck;

	uint16_t _msgId;
	uint8_t _snMsgId;
	Topics* _topics;
	ClientStatus _status;
	uint16_t _keepAliveMsec;
	Timer _keepAliveTimer;

    uint16_t _address16;
	Socket _socket;

	XBeeAddress64 _address64;
    string _nodeId;
};

/*=====================================
        Class ClientList
 =====================================*/
class ClientList{
public:
	ClientList();
	~ClientList();
	void authorize(const char* fileName);
	void erase(ClientNode*);
	ClientNode* getClient(XBeeAddress64* addr64);
	ClientNode* createNode(XBeeAddress64* addr64, string* nodeId = NULL);
	uint16_t getClientCount();
	ClientNode* operator[](int);
private:
	vector<ClientNode*>*  _clientVector;
	Mutex _mutex;
	uint16_t _clientCnt;
	bool _authorize;
};

/*=====================================
         Class Event
  ====================================*/
enum EventType{
	Et_NA = 0,
	EtTimeout,

	EtBrokerSend,
	EtBrokerRecv,
	EtClientSend,
	EtClientRecv,
	EtBroadcast,
	EtSocketAlive
};

class Event{
public:
	Event();
	Event(EventType);
	~Event();
	EventType getEventType();
	void setClientSendEvent(ClientNode*);
	void setBrokerSendEvent(ClientNode*);
	void setClientRecvEvent(ClientNode*);
	void setBrokerRecvEvent(ClientNode*);
	void setEvent(MQTTSnMessage*);
	void setTimeout();
	ClientNode* getClientNode();
	MQTTSnMessage* getMqttSnMessage();
private:
	EventType   _eventType;
	ClientNode* _clientNode;
	MQTTSnMessage* _mqttSnMessage;
};

/*=====================================
     Class GatewayResourcesProvider
 =====================================*/
class GatewayResourcesProvider: public MultiTaskProcess{
public:
	GatewayResourcesProvider();
	~GatewayResourcesProvider();

	EventQue<Event>* getGatewayEventQue();
	EventQue<Event>* getClientSendQue();
	EventQue<Event>* getBrokerSendQue();
	ClientList* getClientList();

private:
	ClientList _clientList;
	EventQue<Event> _gatewayEventQue;
	EventQue<Event> _brokerSendQue;
	EventQue<Event> _clientSendQue;
};



/*=====================================
    Class MessageQue Implimentation
 =====================================*/
template<class T> MessageQue<T>::MessageQue(){

}

template<class T> MessageQue<T>::~MessageQue(){
	_mutex.lock();
	while(!_que.empty()){
		delete _que.front();
		_que.pop();
	}
	_mutex.unlock();
}

template<class T> T* MessageQue<T>::getMessage(){
	T* msg;
	if(!_que.empty()){
		_mutex.lock();
		msg = _que.front();
		_mutex.unlock();
		return msg;
	}else{
		return NULL;
	}
}

template<class T> void MessageQue<T>::push(T* msg){
	_mutex.lock();
	_que.push(msg);
	_mutex.unlock();
}

template<class T> void MessageQue<T>::pop(){
	if(!_que.empty()){
		_mutex.lock();
		delete _que.front();
		_que.pop();
		_mutex.unlock();
	}
}

#endif /* GATEWAY_RESOURCES_PROVIDER_H_ */
