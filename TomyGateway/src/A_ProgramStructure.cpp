/*
 * A_ProgramStructure.cpp
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
 *  Created on: 2013/10/21
 *      Author: Tomoaki YAMAGUCHI
 *     Version: 1.0.0
 *
 */

#include "GatewayResourcesProvider.h"
#include "ClientRecvTask.h"
#include "ClientSendTask.h"
#include "BrokerRecvTask.h"
#include "BrokerSendTask.h"
#include "GatewayControlTask.h"
#include "libmq/ProcessFramework.h"
#include <iostream>


/**************************************
 *       Gateway Application
 **************************************/

GatewayResourcesProvider gwR = GatewayResourcesProvider();

GatewayControlTask th0 = GatewayControlTask(&gwR);
ClientRecvTask th1 = ClientRecvTask(&gwR);
ClientSendTask th2 = ClientSendTask(&gwR);
BrokerRecvTask th3 = BrokerRecvTask(&gwR);
BrokerSendTask th4 = BrokerSendTask(&gwR);

