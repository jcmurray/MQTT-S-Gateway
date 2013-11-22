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
 *     Version:
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
				adv->setGwId(GATEWAY_ID);
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

			if(msg->getType() == MQTTSN_TYPE_SEARCHGW){
				MQTTSnGwInfo* gwinfo = new MQTTSnGwInfo();
				gwinfo->setGwId(GATEWAY_ID);
				Event* ev = new Event();
				ev->setEvent(gwinfo);
				_res->getClientSendQue()->post(ev);
			}
		}
		
		/*------   Message form Clients      ---------*/
		else if(ev->getEventType() == EtClientRecv){

			ClientNode* clnode = ev->getClientNode();
			MQTTSnMessage* msg = clnode->getClientRecvMessage();
			
			if(msg->getType() == MQTTSN_TYPE_PUBLISH){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_PUBLISH" << endl;
				handleSnPublish(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_SUBSCRIBE){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_SUBSCRIBE" << endl;
				handleSnSubscribe(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_UNSUBSCRIBE){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_UNSUBSCRIBE" << endl;
				handleSnUnsubscribe(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_PINGREQ){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_PINGREQ" << endl;
				handleSnPingReq(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_PUBACK){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_PUBACK" << endl;
				handleSnPubAck(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_CONNECT){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_CONNECT" << endl;
				handleSnConnect(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_WILLTOPIC){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_WILLTOPIC" << endl;
				handleSnWillTopic(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_WILLMSG){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_WILLMSG" << endl;
				handleSnWillMsg(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_DISCONNECT){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_DISCONNECT" << endl;
				handleSnDisconnect(ev, clnode, msg);

			}else if(msg->getType() == MQTTSN_TYPE_REGISTER){
				cout << "GatewayControlTask acquire MQTTSN_TYPE_REGISTER" << endl;
				handleSnRegister(ev, clnode, msg);

			}else{
				fprintf(stdout, "%s   %s\n", currentDateTime(), "Irregular ClientRecvMessage");
			}
		}
		/*------   Message form Broker      ---------*/
		else if(ev->getEventType() == EtBrokerRecv){

			ClientNode* clnode = ev->getClientNode();
			MQTTMessage* msg = clnode->getBrokerRecvMessage();
			
			if(msg->getType() == MQTT_TYPE_PUBACK){
				cout << "GatewayControlTask acquire MQTT_TYPE_PUBACK" << endl;
				handlePuback(ev, clnode, msg);

			}else if(msg->getType() == MQTT_TYPE_PINGRESP){
				cout << "GatewayControlTask acquire MQTT_TYPE_PINGRESP" << endl;
				handlePingresp(ev, clnode, msg);

			}else if(msg->getType() == MQTT_TYPE_SUBACK){
				cout << "GatewayControlTask acquire MQTT_TYPE_SUBACK" << endl;
				handleSuback(ev, clnode, msg);

			}else if(msg->getType() == MQTT_TYPE_UNSUBACK){
				cout << "GatewayControlTask acquire MQTT_TYPE_UNSUBACK" << endl;
				handleUnsuback(ev, clnode, msg);

			}else if(msg->getType() == MQTT_TYPE_CONNACK){
				cout << "GatewayControlTask acquire MQTT_TYPE_CONNACK" << endl;
				handleConnack(ev, clnode, msg);

			}else if(msg->getType() == MQTT_TYPE_PUBLISH){
				handlePublish(ev, clnode, msg);
			}else{
				fprintf(stdout, "%s   %s\n", currentDateTime(), "Irregular BrokerRecvMessage");
			}
		}

		delete ev;
	}
}


/*-------------------------------------------------------
 *               Upstream MQTTSnPublish
 -------------------------------------------------------*/
void GatewayControlTask::handleSnPublish(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	Topics* topics = clnode->getTopics();
	MQTTSnPublish* publish = new MQTTSnPublish();
	MQTTPublish* mqMsg = new MQTTPublish();
	publish->absorb(msg);

	mqMsg->setTopic(topics->getTopic(publish->getTopicId())->getTopicName());
	mqMsg->setMessageId(publish->getMsgId());

	if(publish->getFlags() && MQTTSN_FLAG_DUP){
		mqMsg->setDup();
	}
	if(publish->getFlags() && MQTTSN_FLAG_RETAIN){
		mqMsg->setRetain();
	}
	mqMsg->setQos(publish->getQos());
	mqMsg->setPayload(publish->getBodyPtr() , publish->getBodyLength());

	clnode->setBrokerSendMessage(mqMsg);

	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(clnode);
	_res->getBrokerSendQue()->post(ev1);

	delete publish;
}

/*-------------------------------------------------------
                Upstream MQTTSnSubscribe
 -------------------------------------------------------*/
void GatewayControlTask::handleSnSubscribe(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){
	Topics* topics = clnode->getTopics();
	MQTTSnSubscribe* sSubscribe = new MQTTSnSubscribe();
	MQTTSubscribe* subscribe = new MQTTSubscribe();
	sSubscribe->absorb(msg);

	subscribe->setMessageId(sSubscribe->getMsgId());

	if(sSubscribe->getFlags() && MQTTSN_FLAG_DUP){
		subscribe->setDup();
	}
	if(sSubscribe->getFlags() && MQTTSN_FLAG_RETAIN){
		subscribe->setRetain();
	}
	subscribe->setQos(subscribe->getQos());

	if(!((sSubscribe->getFlags() & 0x03 ) == MQTTSN_FLAG_TOPICID_TYPE_RESV)){     // Correct TopicIdType
		if((sSubscribe->getFlags() & 0x03) == MQTTSN_FLAG_TOPICID_TYPE_NORMAL){
			subscribe->setTopic(topics->getTopic(sSubscribe->getTopicId())->getTopicName(), sSubscribe->getQos());

			clnode->setBrokerSendMessage(subscribe);

			Event* ev1 = new Event();
			ev1->setBrokerSendEvent(clnode);
			_res->getBrokerSendQue()->post(ev1);

		}else if((sSubscribe->getFlags() & 0x03) == MQTTSN_FLAG_TOPICID_TYPE_SHORT){  //  Topic
			subscribe->setTopic(sSubscribe->getTopicName(), sSubscribe->getQos());

			if(!clnode->getTopics()->getTopic(sSubscribe->getTopicName())){   // New Topic
				Topic* tp = clnode->getTopics()->createTopic(sSubscribe->getTopicName());

				if(tp->getTopicId()){
					MQTTSnRegAck* sRegack = new MQTTSnRegAck();
					sRegack->setMsgId(sSubscribe->getMsgId());
					sRegack->setTopicId(tp->getTopicId());
					Event* evregack = new Event();
					clnode->setClientSendMessage(sRegack);
					evregack->setClientSendEvent(clnode);
					_res->getClientSendQue()->post(evregack);  // Send RegAck Response

				}else{
					MQTTSnSubAck* sSuback = new MQTTSnSubAck();
					sSuback->setMsgId(sSubscribe->getMsgId());
					sSuback->setReturnCode(MQTTSN_RC_REJECTED_INVALID_TOPIC_ID);
					Event* evregack = new Event();
					clnode->setClientSendMessage(sSuback);
					evregack->setClientSendEvent(clnode);
					_res->getClientSendQue()->post(evregack);  // Send Suback Response

					fprintf(stdout, "%s   %s\n", currentDateTime(), "can't create a Topic");
					delete sSubscribe;
					delete subscribe;
					return;
				}
			}

			clnode->setBrokerSendMessage(subscribe);

			Event* ev1 = new Event();
			ev1->setBrokerSendEvent(clnode);
			_res->getBrokerSendQue()->post(ev1);       // Send MQTTSubscribe

		}else{     //  Predefined TopicID
			cout << "GatewayControlTask  acquire snSubscribe predefinedId" << endl;
			MQTTSnSubAck* sSuback = new MQTTSnSubAck();
			if(sSubscribe->getQos()){       // QoS <> 0
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

				cout << "GatewayControlTask  post suback" << endl;
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
				clnode->setClientSendMessage(pub);

				Event *evpub = new Event();
				evpub->setClientSendEvent(clnode);

				cout << "GatewayControlTask  post publish unix time" << "0x" << pub << endl;
				_res->getClientSendQue()->post(evpub);
			}
		}

	}else{         // Irregular TopicIdType
		delete subscribe;
		fprintf(stdout, "%s   %s\n", currentDateTime(), "Irregular TopicType");
	}
	delete sSubscribe;
}

/*-------------------------------------------------------
                Upstream MQTTSnUnsubscribe
 -------------------------------------------------------*/
void GatewayControlTask::handleSnUnsubscribe(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){
	
MQTTSnUnsubscribe* sUnsubscribe = new MQTTSnUnsubscribe();
	MQTTUnsubscribe* unsubscribe = new MQTTUnsubscribe();
	sUnsubscribe->absorb(msg);

	unsubscribe->setMessageId(sUnsubscribe->getMsgId());

	if(!(sUnsubscribe->getFlags() && MQTTSN_FLAG_TOPICID_TYPE_RESV)){     // Correct TopicId TopicIdType

		if(sUnsubscribe->getFlags() && MQTTSN_FLAG_TOPICID_TYPE_SHORT){  //  TopicName
			unsubscribe->setTopicName(sUnsubscribe->getTopicName());              // prepare UNSUBSCRIBE

		}else{     // Normal Topic
			if(clnode->getTopics()->getTopic(sUnsubscribe->getTopicName())){
				unsubscribe->setTopicName(sUnsubscribe->getTopicName());          // prepare UNSUBSCRIBE

			}else{  // undefined TopicId
				fprintf(stdout, "%s   %s\n", currentDateTime(), "can't create a Topic");
				goto irrUnsub;
			}
		}

		clnode->setBrokerSendMessage(unsubscribe);

		Event* ev1 = new Event();
		ev1->setBrokerSendEvent(clnode);
		_res->getBrokerSendQue()->post(ev1);    // Pass a UNSUBSCRIBE to Broker


	}else{                 // Irregular TopicIdType
		fprintf(stdout, "%s   %s\n", currentDateTime(), "Irregular TopicType");
irrUnsub:	MQTTSnUnsubAck* sUnsuback = new MQTTSnUnsubAck();
		sUnsuback->setMsgId(sUnsubscribe->getMsgId());

		clnode->setClientSendMessage(sUnsuback);

		Event* evun = new Event();
		evun->setClientSendEvent(clnode);
		_res->getClientSendQue()->post(evun);  // Send UNSUBACK to Client instead of UNSUBSCRIBE
		delete sUnsuback;
	}
	delete sUnsubscribe;
}

/*-------------------------------------------------------
                Upstream MQTTSnPingReq
 -------------------------------------------------------*/
void GatewayControlTask::handleSnPingReq(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

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

	Topics* topics = clnode->getTopics();
	MQTTSnConnect* sConnect = new MQTTSnConnect();
	MQTTConnect* mqMsg = new MQTTConnect();
	sConnect->absorb(msg);

	mqMsg->setClientId(sConnect->getClientId());
	mqMsg->setKeepAliveTime(sConnect->getDuration());

	// ToDo: UserName & Password setting

	clnode->setConnectMessage(mqMsg);

	if(sConnect->isCleanSession()){
		if(topics){
			delete topics;
		}
		mqMsg->setCleanSessionFlg();
	}

	if(sConnect->isWillRequired()){
		MQTTSnWillTopicReq* reqTopic = new MQTTSnWillTopicReq();

		Event* evwr = new Event();

		clnode->setClientSendMessage(reqTopic);
		evwr->setClientSendEvent(clnode);
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

	MQTTSnWillTopic* snMsg = new MQTTSnWillTopic();
	MQTTSnWillMsgReq* reqMsg = new MQTTSnWillMsgReq();
	snMsg->absorb(msg);

	clnode->getConnectMessage()->setWillTopic(snMsg->getWillTopic());
	clnode->getConnectMessage()->setWillQos(snMsg->getQos());

	clnode->setClientSendMessage(reqMsg);

	Event* evt = new Event();
	evt->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(evt);  // Send WILLMSGREQ to Client
	delete snMsg;
}

/*-------------------------------------------------------
                Upstream MQTTSnWillMsg
 -------------------------------------------------------*/
void GatewayControlTask::handleSnWillMsg(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

	MQTTSnWillMsg* snMsg = new MQTTSnWillMsg();
	snMsg->absorb(msg);

	clnode->getConnectMessage()->setWillMessage(snMsg->getWillMsg());

	clnode->setBrokerSendMessage(clnode->getConnectMessage());

	Event* ev1 = new Event();
	ev1->setBrokerSendEvent(clnode);
	_res->getBrokerSendQue()->post(ev1);
	delete snMsg;
}

/*-------------------------------------------------------
                Upstream MQTTSnDisconnect
 -------------------------------------------------------*/
void GatewayControlTask::handleSnDisconnect(Event* ev, ClientNode* clnode, MQTTSnMessage* msg){

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

	MQTTSnRegister* snMsg = new MQTTSnRegister();
	MQTTSnRegAck* respMsg = new MQTTSnRegAck();
	snMsg->absorb(msg);

	respMsg->setMsgId(snMsg->getMsgId());

	Topic* tp = clnode->getTopics()->createTopic(snMsg->getTopicName());

	respMsg->setTopicId(tp->getTopicId());
	respMsg->setReturnCode(MQTTSN_RC_ACCEPTED);

	clnode->setClientSendMessage(respMsg);

	Event* evrg = new Event();
	evrg->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(evrg);

	delete snMsg;
}
	
/*-------------------------------------------------------
                Upstream MQTTPubAck
 -------------------------------------------------------*/
void GatewayControlTask::handlePuback(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTPubAck* mqMsg = static_cast<MQTTPubAck*>(msg);
	MQTTSnPubAck* snMsg = new MQTTSnPubAck();

	snMsg->setMsgId(mqMsg->getMessageId());
	//snMsg->setTopicId(topics->getTopic);

	// ToDo: how to get the TopicId

	Event* ev1 = new Event();
	clnode->setClientSendMessage(snMsg);
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}

/*-------------------------------------------------------
                Upstream MQTTPingResp
 -------------------------------------------------------*/
void GatewayControlTask::handlePingresp(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTSnPingResp* snMsg = new MQTTSnPingResp();
	Event* ev1 = new Event();
	clnode->setClientSendMessage(snMsg);
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}

/*-------------------------------------------------------
                Upstream MQTTSubAck
 -------------------------------------------------------*/
void GatewayControlTask::handleSuback(Event* ev, ClientNode* clnode, MQTTMessage* msg){


}

/*-------------------------------------------------------
                Upstream MQTTUnsubAck
 -------------------------------------------------------*/
void GatewayControlTask::handleUnsuback(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTUnsubAck* mqMsg = static_cast<MQTTUnsubAck*>(msg);
	MQTTSnUnsubAck* snMsg = new MQTTSnUnsubAck();

	snMsg->setMsgId(mqMsg->getMessageId());
	Event* ev1 = new Event();
	clnode->setClientSendMessage(snMsg);
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}


/*-------------------------------------------------------
                Upstream MQTTConnAck
 -------------------------------------------------------*/
void GatewayControlTask::handleConnack(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTConnAck* mqMsg = static_cast<MQTTConnAck*>(msg);
	MQTTSnConnack* snMsg = new MQTTSnConnack();

	if(mqMsg->getReturnCd()){
		snMsg->setReturnCode(MQTTSN_RC_REJECTED_NOT_SUPPORTED);
	}else{
		snMsg->setReturnCode(MQTTSN_RC_ACCEPTED);
	}
	Event* ev1 = new Event();
	clnode->setClientSendMessage(snMsg);
	ev1->setClientSendEvent(clnode);
	_res->getClientSendQue()->post(ev1);
}

/*-------------------------------------------------------
                Upstream MQTTPublish
 -------------------------------------------------------*/
void GatewayControlTask::handlePublish(Event* ev, ClientNode* clnode, MQTTMessage* msg){

	MQTTPublish* mqMsg = static_cast<MQTTPublish*>(msg);
	MQTTSnPublish* snMsg = new MQTTSnPublish();


}
