/*
 * GatewayControlTask.cpp
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
 *  Created on: 2013/11/02
 *      Author: Tomoaki YAMAGUCHI
 *     Version: 1.0.0
 *
 */

#include "GatewayControlTask.h"
#include "libmq/ProcessFramework.h"
#include "GatewayResourcesProvider.h"
#include "GatewayDefines.h"
#include "libmq/Messages.h"
#include <stdio.h>
#include <iostream>
#include <string.h>


extern char* currentDateTime();

/*=====================================
        Class GatewayControlTask
 =====================================*/
GatewayControlTask::GatewayControlTask(GatewayResourcesProvider* res){
	_res = res;
	_res->attach(this);
}

GatewayControlTask::~GatewayControlTask(){

}

void GatewayControlTask::run(){
	Timer advertiseTimer;
	Timer sendUnixTimer;
	Event* ev = NULL;

	_eventQue = _res->getGatewayEventQue();

	advertiseTimer.start(KEEP_ALIVE_TIME);
	sendUnixTimer.start(SEND_UNIXTIME_PERIOD);

	while(true){

		ev = _eventQue->timedwait(TIMEOUT_PERIOD);

		/*------     Check Client is Lost    ---------*/
		if(ev->getEventType() == EtTimeout){
			ClientList* clist = _res->getClientList();

			for( int i = 0; i < clist->getClientCount(); i++){
				if((*clist)[i]){
					(*clist)[i]->checkTimeover();
				}else{
					break;
				}
			}

			/*------ Check Keep Alive Timer & send Advertise ------*/
			if(advertiseTimer.isTimeup()){
				MQTTSnAdvertise* adv = new MQTTSnAdvertise();
				adv->setGwId(atoi(_res->getArgv()[ARGV_GATEWAY_ID]));
				adv->setDuration(KEEP_ALIVE_TIME / 1000);
				Event* ev = new Event();
				ev->setEvent(adv);
				_res->getClientSendQue()->post(ev);
				advertiseTimer.start(KEEP_ALIVE_TIME);
			}

			/*------ Check Timer & send UixTime ------*/
			if(sendUnixTimer.isTimeup()){
				MQTTSnPublish* msg = new MQTTSnPublish();
				long int tm = time(NULL);

				msg->setTopicId(MQTTS_TOPICID_PREDEFINED_TIME);
				msg->setTopicIdType(1);
				msg->setData((uint8_t*)&tm, sizeof(long int));
				msg->setQos(0);
				msg->setTopicIdType(1);

				Event* ev = new Event();
				ev->setEvent(msg);
				_res->getClientSendQue()->post(ev);
				sendUnixTimer.start(SEND_UNIXTIME_PERIOD);
			}
		}

		/*------   Check  SEARCHGW & send GWINFO      ---------*/
		else if(ev->getEventType() == EtBroadcast){
			MQTTSnMessage* msg = ev->getMqttSnMessage();
			D_MQTT("\n     SERCHGW      <<<<    Client    %s\n", msgPrint(msg));

			if(msg->getType() == MQTTSN_TYPE_SEARCHGW){
				MQTTSnGwInfo* gwinfo = new MQTTSnGwInfo();
				gwinfo->setGwId(atoi(_res->getArgv()[ARGV_GATEWAY_ID]));
				Event* ev = new Event();
				ev->setEvent(gwinfo);
				D_MQTT("     GWINFO       >>>>    Client    %s\n", msgPrint(gwinfo));
				_res->getClientSendQue()->post(ev);
			}
		}
		
		/*------   Message form Clients      ---------*/
		else if(ev->getEventType() == EtClientRecv){

			ClientNode* clnode = ev->getClientNode();
			MQTTSnMessage* msg = clnode->getClientRecvMessage();
			
			if(msg->getType() == MQTTSN_TYPE_PUBLISH){
				handleSnPublish(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_SUBSCRIBE){
				handleSnSubscribe(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_UNSUBSCRIBE){
				handleSnUnsubscribe(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_PINGREQ){
				handleSnPingReq(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_PUBACK){
				handleSnPubAck(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_CONNECT){
				handleSnConnect(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_WILLTOPIC){
				handleSnWillTopic(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_WILLMSG){
				handleSnWillMsg(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_DISCONNECT){
				handleSnDisconnect(ev, clnode, msg);
			}else if(msg->getType() == MQTTSN_TYPE_REGISTER){
				handleSnRegister(ev, clnode, msg);
			}else{
				D_MQTT("%s   Irregular ClientRecvMessage\n", currentDateTime());
			}
		}
		/*------   Message form Broker      ---------*/
		else if(ev->getEventType() == EtBrokerRecv){

			ClientNode* clnode = ev->getClientNode();
			MQTTMessage* msg = clnode->getBrokerRecvMessage();
			
			if(msg->getType() == MQTT_TYPE_PUBACK){
				handlePuback(ev, clnode, msg);
			}else if(msg->getType() == MQTT_TYPE_PINGRESP){
				handlePingresp(ev, clnode, msg);
			}else if(msg->getType() == MQTT_TYPE_SUBACK){
				handleSuback(ev, clnode, msg);
			}else if(msg->getType() == MQTT_TYPE_UNSUBACK){
				handleUnsuback(ev, clnode, msg);
			}else if(msg->getType() == MQTT_TYPE_CONNACK){
				handleConnack(ev, clnode, msg);
			}else if(msg->getType() == MQTT_TYPE_PUBLISH){
				handlePublish(ev, clnode, msg);
			}else if(msg->getType() == MQTT_TYPE_DISCONNECT){
				handleDisconnect(ev, clnode, msg);
			}else{
				D_MQTT("%s   Irregular BrokerRecvMessage\n", currentDateTime());
			}
		}
		delete ev;
	}
}

/*=======================================================
                     Upstream
 ========================================================*/

/*-------------------------------------------------------
 *               Upstream MQTTSnPublish
 -------------------------------------------------------*/
void GatewayControlTask::handleSnPublish(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("\n     PUBLISH      <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnPublish* sPublish = new MQTTSnPublish();
	MQTTPublish* mqMsg = new MQTTPublish();
	sPublish->absorb(msg);

	Topic* tp = clnode->getTopics()->getTopic(sPublish->getTopicId());

	if(tp){
		mqMsg->setTopic(tp->getTopicName());

		if(sPublish->getMsgId()){
			MQTTSnPubAck* sPuback = new MQTTSnPubAck();
			sPuback->setMsgId(sPublish->getMsgId());
			sPuback->setTopicId(sPublish->getTopicId());
			clnode->setWaitedPubAck(sPuback);

			mqMsg->setMessageId(sPublish->getMsgId());
		}

		mqMsg->setQos(sPublish->getQos());

		if(sPublish->getFlags() && MQTTSN_FLAG_DUP){
			mqMsg->setDup();
		}
		if(sPublish->getFlags() && MQTTSN_FLAG_RETAIN){
			mqMsg->setRetain();
		}

		mqMsg->setPayload(sPublish->getData() , sPublish->getDataLength());

		clnode->setBrokerSendMessage(mqMsg);
		Event* ev1 = new Event();
		ev1->setBrokerSendEvent(clnode);
		_res->getBrokerSendQue()->post(ev1);

	}else{
		if(sPublish->getMsgId()){
			MQTTSnPubAck* sPuback = new MQTTSnPubAck();
			sPuback->setMsgId(sPublish->getMsgId());
			sPuback->setTopicId(sPublish->getTopicId());
			sPuback->setReturnCode(MQTTSN_RC_REJECTED_INVALID_TOPIC_ID);

			clnode->setClientSendMessage(sPuback);

			Event* ev1 = new Event();
			ev1->setClientSendEvent(clnode);
			D_MQTT("     PUBACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(sPuback));
			_res->getClientSendQue()->post(ev1);  // Send PubAck INVALID_TOPIC_ID
		}
	}
	delete sPublish;
}

/*-------------------------------------------------------
                Upstream MQTTSnSubscribe
 -------------------------------------------------------*/
void GatewayControlTask::handleSnSubscribe(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("\n     SUBSCRIBE    <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnSubscribe* sSubscribe = new MQTTSnSubscribe();
	MQTTSubscribe* subscribe = new MQTTSubscribe();
	sSubscribe->absorb(msg);

	uint8_t topicIdType = sSubscribe->getFlags() & 0x03;

	subscribe->setMessageId(sSubscribe->getMsgId());

	if(sSubscribe->getFlags() & MQTTSN_FLAG_DUP ){
		subscribe->setDup();
	}
	if(sSubscribe->getFlags() & MQTTSN_FLAG_RETAIN){
		subscribe->setRetain();
	}
	subscribe->setQos(sSubscribe->getQos());

	if(topicIdType != MQTTSN_FLAG_TOPICID_TYPE_RESV){
		/*----- TopicName ------*/
		if(topicIdType == MQTTSN_FLAG_TOPICID_TYPE_SHORT){
			subscribe->setTopic(sSubscribe->getTopicName(), sSubscribe->getQos());

			uint16_t tpId;
			Topic* tp = clnode->getTopics()->getTopic(sSubscribe->getTopicName());

			if (tp){
				tpId = tp->getTopicId();
			}else{
				tpId = clnode->getTopics()->createTopic(sSubscribe->getTopicName());
			}

			if(sSubscribe->getMsgId()){
				MQTTSnSubAck* sSuback = new MQTTSnSubAck();
				sSuback->setMsgId(sSubscribe->getMsgId());
				sSuback->setTopicId(tpId);
				clnode->setWaitedSubAck(sSuback);
			}

			clnode->setBrokerSendMessage(subscribe);
			Event* ev1 = new Event();
			ev1->setBrokerSendEvent(clnode);
			_res->getBrokerSendQue()->post(ev1);
		}
		/*----- TopicId ------*/
		else if(topicIdType == MQTTSN_FLAG_TOPICID_TYPE_NORMAL){

			Topic* tp = clnode->getTopics()->getTopic(sSubscribe->getTopicId());

			if(tp){
				subscribe->setTopic(tp->getTopicName(), sSubscribe->getQos());

				if(sSubscribe->getMsgId()){
					MQTTSnSubAck* sSuback = new MQTTSnSubAck();
					sSuback->setMsgId(sSubscribe->getMsgId());
					sSuback->setTopicId(tp->getTopicId());
					clnode->setWaitedSubAck(sSuback);
					clnode->setBrokerSendMessage(subscribe);
				}

				Event* ev1 = new Event();
				ev1->setBrokerSendEvent(clnode);
				_res->getBrokerSendQue()->post(ev1);       // Send MQTTSubscribe

			}else{
				MQTTSnSubAck* sSuback = new MQTTSnSubAck();
				sSuback->setMsgId(sSubscribe->getMsgId());
				sSuback->setReturnCode(MQTTSN_RC_REJECTED_INVALID_TOPIC_ID);
				clnode->setClientSendMessage(sSuback);

				Event* evregack = new Event();
				evregack->setClientSendEvent(clnode);
				D_MQTT("     SUBACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(sSuback));
				_res->getClientSendQue()->post(evregack);  // Send SubAck Response

				delete sSubscribe;
				delete subscribe;
				return;
			}

		}else{
			/*----- Predefined TopicId ------*/
			MQTTSnSubAck* sSuback = new MQTTSnSubAck();

			if(sSubscribe->getMsgId()){       // MessageID
				sSuback->setQos(sSubscribe->getQos());
				sSuback->setTopicId(sSubscribe->getTopicId());
				sSuback->setMsgId(sSubscribe->getMsgId());

				if(sSubscribe->getTopicId() == MQTTSN_TOPICID_PREDEFINED_TIME){
					sSuback->setReturnCode(MQTT_RC_ACCEPTED);
				}else{
					sSuback->setReturnCode(MQTT_RC_REFUSED_IDENTIFIER_REJECTED);
				}

				clnode->setClientSendMessage(sSuback);

				Event* evsuback = new Event();
				evsuback->setClientSendEvent(clnode);
				D_MQTT("     SUBACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(sSuback));
				_res->getClientSendQue()->post(evsuback);
			}

			if(sSubscribe->getTopicId() == MQTTSN_TOPICID_PREDEFINED_TIME){

				MQTTSnPublish* pub = new MQTTSnPublish();
				pub->setTopicIdType(1);  // pre-defined
				pub->setTopicId(MQTTSN_TOPICID_PREDEFINED_TIME);
				pub->setMsgId(clnode->getNextSnMsgId());
				long int tm = time(NULL);
				char payload[sizeof(long int)];
				memcpy(payload, &tm, sizeof(long int));
				pub->setData((unsigned char*)payload, sizeof(long int));
				D_MQTT("     PUBLISH      >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(pub));
				clnode->setClientSendMessage(pub);

				Event *evpub = new Event();
				evpub->setClientSendEvent(clnode);
				D_MQTT("     PUBACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(pub));
				_res->getClientSendQue()->post(evpub);
			}
			delete subscribe;
		}
		delete sSubscribe;
		return;

	}else{
		/*-- Irregular TopicIdType --*/
		if(sSubscribe->getMsgId()){
			MQTTSnSubAck* sSuback = new MQTTSnSubAck();
			sSuback->setMsgId(sSubscribe->getMsgId());
			sSuback->setTopicId(sSubscribe->getTopicId());
			sSuback->setReturnCode(MQTT_RC_REFUSED_IDENTIFIER_REJECTED);

			clnode->setClientSendMessage(sSuback);

			Event* evun = new Event();
			evun->setClientSendEvent(clnode);
			D_MQTT("     SUBACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(sSuback));
			_res->getClientSendQue()->post(evun);  // Send SUBACK to Client
		}
	}
	delete subscribe;
	delete sSubscribe;
}

/*-------------------------------------------------------
                Upstream MQTTSnUnsubscribe
 -------------------------------------------------------*/
void GatewayControlTask::handleSnUnsubscribe(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("\n     UNSUBSCRIBE  <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnUnsubscribe* sUnsubscribe = new MQTTSnUnsubscribe();
	MQTTUnsubscribe* unsubscribe = new MQTTUnsubscribe();
	sUnsubscribe->absorb(msg);

	uint8_t topicIdType = sUnsubscribe->getFlags() & 0x03;

	unsubscribe->setMessageId(sUnsubscribe->getMsgId());

	if(topicIdType != MQTTSN_FLAG_TOPICID_TYPE_RESV){

		if(topicIdType == MQTTSN_FLAG_TOPICID_TYPE_SHORT){
			unsubscribe->setTopicName(sUnsubscribe->getTopicName()); // TopicName

		}else if(clnode->getTopics()->getTopic(sUnsubscribe->getTopicId())){

			if(topicIdType == MQTTSN_FLAG_TOPICID_TYPE_PREDEFINED) goto uslbl1;

			unsubscribe->setTopicName(sUnsubscribe->getTopicName());
		}

		clnode->setBrokerSendMessage(unsubscribe);

		Event* ev1 = new Event();
		ev1->setBrokerSendEvent(clnode);
		_res->getBrokerSendQue()->post(ev1);    //  UNSUBSCRIBE to Broker
		delete sUnsubscribe;
		return;
	}

	/*-- Irregular TopicIdType or MQTTSN_FLAG_TOPICID_TYPE_PREDEFINED --*/
uslbl1:
	if(sUnsubscribe->getMsgId()){
		MQTTSnUnsubAck* sUnsuback = new MQTTSnUnsubAck();
		sUnsuback->setMsgId(sUnsubscribe->getMsgId());

		clnode->setClientSendMessage(sUnsuback);

		Event* evun = new Event();
		evun->setClientSendEvent(clnode);
		D_MQTT("     UNSUBACK     >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(sUnsuback));
		_res->getClientSendQue()->post(evun);  // Send UNSUBACK to Client
	}

	delete sUnsubscribe;
}

/*-------------------------------------------------------
                Upstream MQTTSnPingReq
 -------------------------------------------------------*/
void GatewayControlTask::handleSnPingReq(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("\n     PINGREQ      <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTPingReq* pingReq = new MQTTPingReq();

	clnode->setBrokerSendMessage(pingReq);

	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(clnode);
	_res->getBrokerSendQue()->post(ev1);
}

/*-------------------------------------------------------
                Upstream MQTTSnPubAck
 -------------------------------------------------------*/
void GatewayControlTask::handleSnPubAck(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("     PUBACK       <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnPubAck* sPubAck = new MQTTSnPubAck();
	MQTTPubAck* pubAck = new MQTTPubAck();
	sPubAck->absorb(msg);

	pubAck->setMessageId(sPubAck->getMsgId());

	clnode->setBrokerSendMessage(pubAck);
	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(clnode);
	_res->getBrokerSendQue()->post(ev1);
	delete sPubAck;
}

/*-------------------------------------------------------
                Upstream MQTTSnConnect
 -------------------------------------------------------*/
void GatewayControlTask::handleSnConnect(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("\n     CONNECT      <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	Topics* topics = clnode->getTopics();
	MQTTSnConnect* sConnect = new MQTTSnConnect();
	MQTTConnect* mqMsg = new MQTTConnect();
	sConnect->absorb(msg);

	mqMsg->setClientId(sConnect->getClientId());
	mqMsg->setKeepAliveTime(sConnect->getDuration());

	// ToDo: UserName & Password setting

	clnode->setConnectMessage(mqMsg);
	clnode->setNodeId(sConnect->getClientId());

	if(sConnect->isCleanSession()){
		if(topics){
			delete topics;
		}
		topics = new Topics();
		clnode->setTopics(topics);
		mqMsg->setCleanSessionFlg();
	}

	if(sConnect->isWillRequired()){
		MQTTSnWillTopicReq* reqTopic = new MQTTSnWillTopicReq();

		Event* evwr = new Event();

		clnode->setClientSendMessage(reqTopic);
		evwr->setClientSendEvent(clnode);
		D_MQTT("     WILLTOPICREQ >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(reqTopic));
		_res->getClientSendQue()->post(evwr);  // Send WILLTOPICREQ to Client
	}else{
		Event* ev1 = new Event();
		clnode->setBrokerSendMessage(clnode->getConnectMessage());
		ev1->setBrokerSendEvent(clnode);
		_res->getBrokerSendQue()->post(ev1);
	}
	delete sConnect;
}

/*-------------------------------------------------------
                Upstream MQTTSnWillTopic
 -------------------------------------------------------*/
void GatewayControlTask::handleSnWillTopic(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("     WILLTOPIC    <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnWillTopic* snMsg = new MQTTSnWillTopic();
	MQTTSnWillMsgReq* reqMsg = new MQTTSnWillMsgReq();
	snMsg->absorb(msg);

	if(clnode->getConnectMessage()){
		clnode->getConnectMessage()->setWillTopic(snMsg->getWillTopic());
		clnode->getConnectMessage()->setWillQos(snMsg->getQos());

		clnode->setClientSendMessage(reqMsg);

		Event* evt = new Event();
		evt->setClientSendEvent(clnode);
		D_MQTT("     WILLMSGREQ   >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(reqMsg));
		_res->getClientSendQue()->post(evt);  // Send WILLMSGREQ to Client
	}
	delete snMsg;
}

/*-------------------------------------------------------
                Upstream MQTTSnWillMsg
 -------------------------------------------------------*/
void GatewayControlTask::handleSnWillMsg(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("     WILLMSG      <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnWillMsg* snMsg = new MQTTSnWillMsg();
	snMsg->absorb(msg);

	if(clnode->getConnectMessage()){
		clnode->getConnectMessage()->setWillMessage(snMsg->getWillMsg());

		clnode->setBrokerSendMessage(clnode->getConnectMessage());
		clnode->setConnectMessage(NULL);

		Event* ev1 = new Event();
		ev1->setBrokerSendEvent(clnode);
		_res->getBrokerSendQue()->post(ev1);

	}else{
		MQTTSnConnack* connack = new MQTTSnConnack();
		connack->setReturnCode(MQTTSN_RC_REJECTED_CONGESTION);

		clnode->setClientSendMessage(connack);
		Event* ev1 = new Event();
		ev1->setClientSendEvent(clnode);
		D_MQTT("    *CONNACK      >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(connack));
		_res->getClientSendQue()->post(ev1);  // Send CONNACK REJECTED CONGESTION to Client

	}
	delete snMsg;
}

/*-------------------------------------------------------
                Upstream MQTTSnDisconnect
 -------------------------------------------------------*/
void GatewayControlTask::handleSnDisconnect(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("\n     DISCONNECT   <<<<    %s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnDisconnect* snMsg = new MQTTSnDisconnect();
	MQTTDisconnect* mqMsg = new MQTTDisconnect();
	snMsg->absorb(msg);

	clnode->setBrokerSendMessage(mqMsg);

	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(clnode);
	_res->getBrokerSendQue()->post(ev1);

	delete snMsg;
}


/*-------------------------------------------------------
                Upstream MQTTSnRegister
 -------------------------------------------------------*/
void GatewayControlTask::handleSnRegister(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	D_MQTT("\n     REGISTER     <<<<    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(msg));

	MQTTSnRegister* snMsg = new MQTTSnRegister();
	MQTTSnRegAck* respMsg = new MQTTSnRegAck();
	snMsg->absorb(msg);

	respMsg->setMsgId(snMsg->getMsgId());

	uint16_t tpId = clnode->getTopics()->createTopic(snMsg->getTopicName());

	respMsg->setTopicId(tpId);
	respMsg->setReturnCode(MQTTSN_RC_ACCEPTED);

	clnode->setClientSendMessage(respMsg);

	Event* evrg = new Event();
	evrg->setClientSendEvent(clnode);
	D_MQTT("     REGACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(respMsg));
	_res->getClientSendQue()->post(evrg);

	delete snMsg;
}
	

/*=======================================================
                     Downstream
 ========================================================*/

/*-------------------------------------------------------
                Downstream MQTTPubAck
 -------------------------------------------------------*/
void GatewayControlTask::handlePuback(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTPubAck* mqMsg = static_cast<MQTTPubAck*>(msg);
	MQTTSnPubAck* snMsg = clnode->getWaitedPubAck();

	if(snMsg){
		D_MQTT("     PUBACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(snMsg));
		if(snMsg->getMsgId() != mqMsg->getMessageId()){
			delete snMsg;
			return;
		}

		clnode->setClientSendMessage(snMsg);
		Event* ev1 = new Event();
		ev1->setClientSendEvent(clnode);
		_res->getClientSendQue()->post(ev1);
	}
}

/*-------------------------------------------------------
                Downstream MQTTPingResp
 -------------------------------------------------------*/
void GatewayControlTask::handlePingresp(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTSnPingResp* snMsg = new MQTTSnPingResp();
	D_MQTT("     PINGREQ     >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(snMsg));
	clnode->setClientSendMessage(snMsg);

	Event* ev1 = new Event();
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}

/*-------------------------------------------------------
                Downstream MQTTSubAck
 -------------------------------------------------------*/
void GatewayControlTask::handleSuback(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTSubAck* mqMsg = static_cast<MQTTSubAck*>(msg);
	MQTTSnSubAck* snMsg = clnode->getWaitedSubAck();
	if(snMsg){
		if(snMsg->getMsgId() != mqMsg->getMessageId()){
			delete snMsg;
			return;
		}else{
			snMsg->setReturnCode(MQTTSN_RC_ACCEPTED);
			snMsg->setQos(mqMsg->getGrantedQos());
		}
		D_MQTT("     SUBACK       >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(snMsg));
		clnode->setClientSendMessage(snMsg);

		Event* ev1 = new Event();
		ev1->setClientSendEvent(clnode);
		_res->getClientSendQue()->post(ev1);
	}
}

/*-------------------------------------------------------
                Downstream MQTTUnsubAck
 -------------------------------------------------------*/
void GatewayControlTask::handleUnsuback(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTUnsubAck* mqMsg = static_cast<MQTTUnsubAck*>(msg);
	MQTTSnUnsubAck* snMsg = new MQTTSnUnsubAck();

	snMsg->setMsgId(mqMsg->getMessageId());
	D_MQTT("     UNSUBACK     >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(snMsg));
	clnode->setClientSendMessage(snMsg);

	Event* ev1 = new Event();
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}


/*-------------------------------------------------------
                Downstream MQTTConnAck
 -------------------------------------------------------*/
void GatewayControlTask::handleConnack(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTConnAck* mqMsg = static_cast<MQTTConnAck*>(msg);
	MQTTSnConnack* snMsg = new MQTTSnConnack();

	if(mqMsg->getReturnCd()){
		snMsg->setReturnCode(MQTTSN_RC_REJECTED_NOT_SUPPORTED);
	}else{
		snMsg->setReturnCode(MQTTSN_RC_ACCEPTED);
	}
	D_MQTT("     CONNACK      >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(snMsg));
	clnode->setClientSendMessage(snMsg);

	Event* ev1 = new Event();
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}

/*-------------------------------------------------------
                Downstream MQTTConnAck
 -------------------------------------------------------*/
void GatewayControlTask::handleDisconnect(Event* ev, ClientNode* clnode, MQTTMessage* msg){
	MQTTSnDisconnect* snMsg = new MQTTSnDisconnect();
	clnode->setClientSendMessage(snMsg);
	D_MQTT("     DISCONNECT   >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(snMsg));
	Event* ev1 = new Event();
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}

/*-------------------------------------------------------
                Downstream MQTTPublish
 -------------------------------------------------------*/
void GatewayControlTask::handlePublish(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTPublish* mqMsg = static_cast<MQTTPublish*>(msg);
	MQTTSnPublish* snMsg = new MQTTSnPublish();

	UTFString* tp = mqMsg->getTopic();
	uint16_t tpId;

	tpId = clnode->getTopics()->getTopicId(tp);

	if(tpId == 0){
		/* ----- may be a publish message response of subscribed with '#' or '+' -----*/
		tpId = clnode->getTopics()->createTopic(tp);

		if(tpId > 0){
			MQTTSnRegister* regMsg = new MQTTSnRegister();
			regMsg->setTopicId(tpId);
			regMsg->setTopicName(tp);
			D_MQTT("     REGISTER     >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(regMsg));
			clnode->setClientSendMessage(regMsg);
			Event* evrg = new Event();
			evrg->setClientSendEvent(clnode);
			_res->getClientSendQue()->post(evrg);   // Send Register first.
		}else{
			D_MQTT("GatewayControlTask Can't create Topic   %s\n", tp->c_str());
			return;
		}
	}

	snMsg->setTopicId(tpId);
	snMsg->setMsgId(mqMsg->getMessageId());
	snMsg->setData(mqMsg->getPayload(),mqMsg->getPayloadLength());
	snMsg->setQos(mqMsg->getQos());

	if(mqMsg->isDup()){
		snMsg->setDup();
	}

	if(mqMsg->isRetain()){
		snMsg->setDup();
	}
	clnode->setClientSendMessage(snMsg);
	D_MQTT("     PUBLISH      >>>>    %-10s%s\n", clnode->getNodeId()->c_str(), msgPrint(snMsg));
	Event* ev1 = new Event();
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);

}

char*  GatewayControlTask::msgPrint(MQTTSnMessage* msg){

	char* buf = _printBuf;
	for(int i = 0; i < msg->getBodyLength(); i++){
		sprintf(buf," 0x%02X", *(msg->getBodyPtr() + i));
		buf += 5;
	}
	*buf = 0;
	return _printBuf;
}

char*  GatewayControlTask::msgPrint(MQTTMessage* msg){
	uint8_t sbuf[512];
	char* buf = _printBuf;
	msg->serialize(sbuf);

	for(int i = 0; i < msg->getRemainLength(); i++){
		sprintf(buf, " 0x%02X", *( sbuf + msg->getRemainLengthSize() + i));
		buf += 5;
	}
	*buf = 0;
	return _printBuf;
}
