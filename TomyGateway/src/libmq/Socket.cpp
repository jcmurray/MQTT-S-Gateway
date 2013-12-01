/*
 * Socket.cpp
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
 *     Version: 0.1.0
 *
 */

#include "Defines.h"
#include "Socket.h"
#include "string.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>

using namespace std;

/*========================================
       Class Socket
 =======================================*/
Socket::Socket() :  _sock ( -1 ){
    memset ( &_addr, 0, sizeof ( _addr ) );
}

Socket::~Socket(){
    disconnect();
}

void Socket::disconnect(){
    if ( isValid() ){
    	::close ( _sock );
    	_sock = -1;
    }
}

bool Socket::create(){
	_sock = socket ( AF_INET, SOCK_STREAM, 0 );
	if ( ! isValid() ){
		return false;
    }
	// TIME_WAIT - argh
	int on = 1;
	if ( setsockopt ( _sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 ){
		return false;
	}
  return true;
}

bool Socket::bind ( const int port ){
	if ( ! isValid() ){
		return false;
	}
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port = htons ( port );

	int bind_return = ::bind ( _sock, ( struct sockaddr * ) &_addr, sizeof ( _addr ) );
	if ( bind_return == -1 ){
	    return false;
	}
	return true;
}

bool Socket::listen() const{
	if ( ! isValid() )	{
	    return false;
	}
	int listen_return = ::listen ( _sock, SOCKET_MAXCONNECTIONS );
	if ( listen_return == -1 ){
	    return false;
	}
	return true;
}


bool Socket::accept ( Socket& new_socket ) const{
	int addr_length = sizeof ( _addr );
	new_socket._sock = ::accept ( _sock, ( sockaddr * ) &_addr, ( socklen_t * ) &addr_length );
	if ( new_socket._sock <= 0 ){
		return false;
	}else{
		return true;
	}
}

int Socket::send (const uint8_t* buf, uint16_t length  ){
	int status = ::send ( _sock, buf, length, MSG_NOSIGNAL );
	if( status == -1){
		printf("       errno == %d in Socket::send\n", errno);
	}
	return status;
}

int Socket::recv ( uint8_t* buf, uint16_t len ){
	int status = ::recv ( _sock, buf, len, 0 );

	if ( status == -1 )	{
		printf("       errno == %d in Socket::recv\n", errno);
	    return -1;
	}else if ( status == 0 ){
	    return 0;
	}else{
	    return status;
	}
}


bool Socket::connect ( const string host, const int port ){
	if ( ! isValid() ){
		return false;
	}
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons ( port );

	int status = inet_pton ( AF_INET, host.c_str(), &_addr.sin_addr );

	if ( errno == EAFNOSUPPORT ){
		return false;
	}
	status = ::connect ( _sock, ( sockaddr * ) &_addr, sizeof ( _addr ) );

	if ( status == 0 ){
		return true;
	}else{
		return false;
	}
}

void Socket::setNonBlocking ( const bool b ){
	int opts;

	opts = fcntl ( _sock, F_GETFL );

	if ( opts < 0 ){
	    return;
	}

	if ( b ){
	    opts = ( opts | O_NONBLOCK );
	}else{
	    opts = ( opts & ~O_NONBLOCK );
	}
	fcntl ( _sock,  F_SETFL,opts );
}

