/*
 * GatewayDefines.h
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
 *  Created on: 2013/11/08
 *      Author: Tomoaki YAMAGUCHI
 *     Version: 1.0.0
 *
 */

#ifndef GATEWAYDEFINES_H_
#define GATEWAYDEFINES_H_


#define DEBUG_MQTT

/*============================================
   Debug Print Definition
 ============================================*/
#ifdef DEBUG_MQTT
  #define D_MQTT(...)  printf(__VA_ARGS__)
#else
  #define D_MQTT(...)
#endif

/*===========================================
 *   Gateway Control Constants
 ===========================================*/

#define BROKER_HOST_NAME  "localhost"
#define BROKER_PORT       1883

#define KEEP_ALIVE_TIME   900000    // 900000 msec = 15 min

#define TIMEOUT_PERIOD     10000    //  10000 msec = 10 sec

#define SEND_UNIXTIME_PERIOD  360000   // 360000 msec = 1 hour



#define MAX_CLIENT_NODES  500



#endif /* GATEWAYDEFINES_H_ */
