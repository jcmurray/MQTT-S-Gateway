/*
 * Topics.cpp
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
 *  Created on: 2013/11/07
 *      Author: Tomoaki YAMAGUCHI
 *     Version:
 *
 */

#include "Topics.h"
#include "Messages.h"
#include <string>

/*=====================================
        Class Topic
 ======================================*/
Topic::Topic(){
    _topicId = 0;
}


Topic::Topic(UTFString topic){
	_topicId = 0;
	_topicStr = topic;
}


Topic::~Topic(){

}

uint16_t Topic::getTopicId(){
    return _topicId;
}

UTFString* Topic::getTopicName(){
    return &_topicStr;
}

uint8_t Topic::getTopicLength(){
    return _topicStr.getByteLength();
}

void Topic::setTopicId(uint16_t id){
    _topicId = id;
}


void Topic::setTopicName(UTFString topic){
    _topicStr = topic;
}


uint8_t Topic::isWildCard(uint8_t* pos){
	unsigned int p = _topicStr.find("MQTTSN_TOPIC_SINGLE_WILDCARD", 0);
	if( p != string::npos){
		*pos = p;
		return MQTTSN_TOPIC_SINGLE_WILDCARD;
	}else{
		p = _topicStr.find("MQTTSN_TOPIC_MULTI_WILDCARD", 0);
		if( p != string::npos){
			*pos = p;
			return MQTTSN_TOPIC_MULTI_WILDCARD;
		}
	}
	*pos = 0;
    return 0;
}

bool Topic::isMatch(Topic* topic){
    uint8_t pos;
    uint8_t pos1;

    if (topic->isWildCard(&pos) == MQTTSN_TOPIC_SINGLE_WILDCARD &&
        (_topicStr.compare(0, (unsigned int)pos, (const string)*(topic->getTopicName()), 0, (unsigned int)pos) == 0 )){
    		pos1 = pos + 2;
    		pos = _topicStr.find("/", ++pos);

    	if(pos == string::npos ){
    		return true;
    	}else{
    		if(_topicStr.substr(pos) == topic->getTopicName()->substr(pos1)){
    			return true;
    		}else{
    			return false;
    		}
    	}

    }else if(topic->isWildCard(&pos) == MQTTSN_TOPIC_MULTI_WILDCARD &&
    		(_topicStr.compare(0, (unsigned int)pos, (const string)*(topic->getTopicName()), 0, (unsigned int)pos) == 0 )){
        return true;
    }
    return false;
}

/*=====================================
        Class Topics
 ======================================*/
Topics::Topics(){
	_topicId = MQTTSN_TOPICID_NORMAL;
	Topic* tp = new Topic(UTFString(MQTTSN_TOPIC_PREDEFINED_TIME));
	tp->setTopicId(MQTTSN_TOPICID_PREDEFINED_TIME);
	_topics.push_back(tp);
}

Topics::~Topics() {

}


uint16_t Topics::getTopicId(UTFString* topic){
    Topic *p = getTopic(topic);
    if ( p != NULL) {
        return p->getTopicId();
    }
    return 0;
}


Topic* Topics::getTopic(UTFString* topic) {
    for (uint8_t i = 0; i < _topics.size(); i++) {
        if( topic == (_topics[i]->getTopicName())){

            return _topics[i];
        }
    }
    return NULL;
}

Topic* Topics::getTopic(uint16_t id) {
    for (uint8_t i = 0; i < _topics.size(); i++) {
        if ( _topics[i]->getTopicId() == id) {
            return _topics[i];
        }
    }
      return NULL;
}


Topic* Topics::createTopic(UTFString* topic){
    if (!getTopic(topic)){
        if ( _topics.size() < MAX_TOPIC_COUNT){
        	Topic* tp = new Topic(*topic);
        	tp->setTopicId(getNextId());
        	_topics.push_back(tp);
        	return tp;
        }else{
        	return NULL;
        }
    }else{
    	return getTopic(topic);
    }
}

uint16_t Topics::getNextId(){
	if(++_topicId == MQTTSN_TOPICID_NORMAL){
		return ++_topicId;
	}else{
		return _topicId;
	}
}

Topic* Topics::match(UTFString* topic){
	uint8_t pos;

    for ( uint8_t i = 0; i< _topics.size(); i++){
        if (_topics[i]->isWildCard(&pos)){
            if (getTopic(topic)->isMatch(_topics[i])){
               return _topics[i];
            }
        }
    }
    return NULL;
}


