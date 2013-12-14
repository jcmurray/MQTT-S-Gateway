/*
 * ClientSendTask.cpp
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
 *     Version: 1.0.0
 *
 */
#include "ClientSendTask.h"
#include "GatewayResourcesProvider.h"
#include "libmq/ProcessFramework.h"
#include "libmq/Messages.h"
#include "libmq/ErrorMessage.h"

#include <unistd.h>
#include <iostream>
#include <fcntl.h>


ClientSendTask::ClientSendTask(GatewayResourcesProvider* res){
	_res = res;
	_res->attach(this);
}

ClientSendTask::~ClientSendTask(){

}

void ClientSendTask::run(){

	MQTTSnMessage* msg = new MQTTSnMessage();

	if(_sp.begin(_res->getArgv()[ARGV_DEVICE_NAME], B57600, O_WRONLY) == -1){
		THROW_EXCEPTION(ExFatal, ERRNO_SYS_02, "can't open device.");  // ABORT
	}

	_zb.setSerialPort(&_sp);

	while(true){

		Event* ev = _res->getClientSendQue()->wait();

		if(ev->getEventType() == EtClientSend){
			ClientNode* clnode = ev->getClientNode();
			msg->absorb( clnode->getClientSendMessage() );

			_zb.unicast(clnode->getAddress64Ptr(), clnode->getAddress16(),
					msg->getMessagePtr(), msg->getMessageLength());

		}else if(ev->getEventType() == EtBroadcast){
			msg->absorb( ev->getMqttSnMessage() );
			_zb.broadcast(msg->getMessagePtr(), msg->getMessageLength());
		}
		delete ev;
	}
}
