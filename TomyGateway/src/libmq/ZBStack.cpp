/*
 * ZBeeStack.cpp
 *
 *               Copyright (c) 2009 Andrew Rapp.       All rights reserved.
 *               Copyright (c) 2013 Tomoaki YAMAGUCHI  All rights reserved.
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

#include "Messages.h"
#include "ZBStack.h"
#include "ProcessFramework.h"
#include "ErrorMessage.h"
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

using namespace std;

extern uint8_t* mqcalloc(uint8_t length);
extern uint32_t getLong(uint8_t* pos);
//extern void setLong(uint8_t* pos, uint32_t val);

/*=========================================
             Class XBeeAddress64
 =========================================*/
XBeeAddress64::XBeeAddress64(){
  _msb = _lsb = 0;
}

XBeeAddress64::XBeeAddress64(uint32_t msb, uint32_t lsb){
  _msb = msb;
  _lsb = lsb;
}

uint32_t XBeeAddress64::getMsb(){
  return _msb;
}

uint32_t XBeeAddress64::getLsb(){
  return _lsb;
}

void XBeeAddress64::setMsb(uint32_t msb){
  _msb = msb;
}

void XBeeAddress64::setLsb(uint32_t lsb){
  _lsb = lsb;
}

bool XBeeAddress64::operator==(XBeeAddress64& addr){
	if(_msb == addr.getMsb() && _lsb == addr.getLsb()){
		return true;
	}else{
		return false;
	}
}
/*=========================================
             Class XBResponse
 =========================================*/
XBResponse::XBResponse(){
	_frameDataPtr = 0;
	_msbLength = 0;
	_lsbLength = 0;
	_checksum = 0;
	_frameLength = 0;
	_errorCode = NO_ERROR;
	_complete = false;
	_apiId = 0;
}

XBResponse::~XBResponse(){
  if(_frameDataPtr){
	  free(_frameDataPtr);
  }
}

uint8_t XBResponse::getMsbLength(){
  return _msbLength;
}

uint8_t XBResponse::getLsbLength(){
  return _lsbLength;
}

uint8_t XBResponse::getChecksum(){
  return _checksum;
}

uint8_t XBResponse::getFrameDataLength(){
  return _frameLength;
}

uint8_t* XBResponse::getFrameData(){
  return _frameDataPtr;
}

uint16_t XBResponse::getPacketLength() {
        return ((_msbLength << 8) & 0xff) + (_lsbLength & 0xff);
}

void XBResponse::setMsbLength(uint8_t msbLength){
  _msbLength = msbLength;
}

void XBResponse::setLsbLength(uint8_t lsbLength){
  _lsbLength = lsbLength;
}

void XBResponse::setChecksum(uint8_t checksum){
  _checksum = checksum;
}

void XBResponse::setFrameDataLength(uint8_t frameLength){
  _frameLength = frameLength;
}
void XBResponse::setFrameData(uint8_t *frameDataPtr){
  _frameDataPtr = frameDataPtr;
}

bool XBResponse::isAvailable(){
  return _complete;
}

void XBResponse::setAvailable(bool complete){
  _complete = complete;
}

bool XBResponse::isError(){
  return _errorCode > 0;
}

uint8_t XBResponse::getErrorCode(){
  return _errorCode;
}

void XBResponse::setErrorCode(uint8_t errorCode){
  _errorCode = errorCode;
}

void XBResponse::setApiId(uint8_t apiId){
  _apiId = apiId;
}

void XBResponse::reset(){
	if(_frameDataPtr){
		  free(_frameDataPtr);
	}
	_msbLength = 0;
	_lsbLength = 0;
	_checksum = 0;
	_frameLength = 0;
	_errorCode = NO_ERROR;
_complete = false;
}

uint8_t XBResponse::getPayload(int index){
  return getFrameData()[ZB_PAYLOAD_OFFSET + index];
}

uint8_t* XBResponse::getPayloadPtr(){
  return getFrameData() + ZB_PAYLOAD_OFFSET;
}

uint8_t XBResponse::getPayloadLength(){
  return getFrameDataLength() - ZB_PAYLOAD_OFFSET;
}

uint16_t XBResponse::getRemoteAddress16(){
  return (getFrameData()[8] << 8) + getFrameData()[9];
}


uint32_t XBResponse::getRemoteAddressMsb(){
	return getLong(getFrameData());
}

uint32_t XBResponse::getRemoteAddressLsb(){
	return getLong(getFrameData() + 4);
}

XBeeAddress64* XBResponse::getRemoteAddress64(){
	return &_addr64;
}

uint8_t XBResponse::getOption(){
  return getFrameData()[10];
}

uint8_t XBResponse::getApiId(){
  return _apiId;
}

bool XBResponse::isBroadcast(){
  return ( getOption() && 0x02);
}

void XBResponse::absorb(XBResponse* resp){
	if(_frameDataPtr){
		free(_frameDataPtr);
	}
	_apiId = resp->getApiId();
	_msbLength = resp->getMsbLength();
	_lsbLength = resp->getLsbLength();
	_checksum = resp->getChecksum();
	_frameLength = resp->getFrameDataLength();
	_errorCode = resp->getErrorCode();
	_complete = resp->isAvailable();
	_addr64.setMsb(resp->getRemoteAddressMsb());
	_addr64.setLsb(resp->getRemoteAddressLsb());
	_frameDataPtr = mqcalloc(resp->getFrameDataLength());
	memcpy(_frameDataPtr, resp->getFrameData(), resp->getFrameDataLength());
}

/*=========================================
             Class XBRequest
 =========================================*/

XBRequest::XBRequest(){
	_apiId = 0x10;
	_addr16 = 0;
	_broadcastRadius = 0;
	_option = 0;
	_payloadPtr = 0;
	_payloadLength = 0;
}


XBRequest::~XBRequest(){

}

XBeeAddress64& XBRequest::getAddress64(){
    return _addr64;
}

uint16_t XBRequest::getAddress16(){
    return _addr16;
}

uint8_t XBRequest::getApiId(){
    return _apiId;
}

uint8_t XBRequest::getBroadcastRadius(){
    return _broadcastRadius;
}

uint8_t  XBRequest::getOption(){
    return _option;
}

uint8_t*  XBRequest::getPayloadPtr(){
    return _payloadPtr;
}

uint8_t  XBRequest::getPayloadLength(){
  return _payloadLength;
}

void XBRequest::setAddress64(XBeeAddress64* addr64){
	_addr64.setMsb(addr64->getMsb());
	_addr64.setLsb(addr64->getLsb());
}

void XBRequest::setAddress16(uint16_t addr16){
    _addr16 = addr16;
}

void XBRequest::setBroadcastRadius(uint8_t broadcastRadius){
    _broadcastRadius = broadcastRadius;
}

void XBRequest::setOption(uint8_t option){
    _option = option;
}

void XBRequest::setApiId(uint8_t apiId){
    _apiId = apiId;
}

void XBRequest::setPayload(uint8_t* payload){
    _payloadPtr = payload;
}

void XBRequest::setPayloadLength(uint8_t payloadLength){
    _payloadLength = payloadLength;
}


uint8_t XBRequest::getFrameData(uint8_t pos){
	if (pos == 0){
		return 0;    // Frame ID
	}else if (pos == 1){
		return _addr64.getMsb() & 0xff;
	}else if (pos == 2){
		return (_addr64.getMsb() >> 8) & 0xff;
	}else if (pos == 3){
		return (_addr64.getMsb() >> 16) & 0xff;
	}else if (pos == 4){
		return (_addr64.getMsb() >> 24) & 0xff;
	}else if (pos == 5){
		return _addr64.getLsb() & 0xff;
	}else if (pos == 6){
		return (_addr64.getLsb() >> 8) & 0xff;
	}else if (pos == 7){
		return (_addr64.getLsb() >> 16) & 0xff;
	}else if (pos == 8){
		return (_addr64.getLsb() >> 24) & 0xff;
	}else if (pos == 9){
		return (_addr16 >> 8) & 0xff;
	}else if (pos == 10){
		return _addr16 & 0xff;
	}else if (pos == 11){
		return _broadcastRadius;
	}else if (pos == 12){
		return _option;
	}
	return getPayloadPtr()[pos - ZB_TX_API_LENGTH -1 ];
}

uint8_t XBRequest::getFrameDataLength(){
    return ZB_TX_API_LENGTH + 1 + getPayloadLength();
}


/*=========================================
             Class XBee
 =========================================*/

XBee::XBee(){
	_pos = 0;
	_escape = false;
	_checksumTotal = 0;
	_response.setFrameData(mqcalloc(MAX_FRAME_DATA_SIZE));
	_serialPort = 0;
	_bd = 0;
}

XBee::~XBee(){

}

void XBee::readPacket(){

	while(read(&_bd)){
	  // Check Start Byte
	  if( _pos > 0 && _bd == START_BYTE){
		  _pos = 0;
	  }
	  // Check ESC
	  if(_pos > 0 && _bd == ESCAPE){
		  if(read(&_bd )){
			  _bd = 0x20^_bd;  // decode
		  }else{
			  _escape = true;
			  continue;
		  }
	  }

	  if(_escape){
		  _bd = 0x20 ^ _bd;
		  _escape = false;
	  }

	  if(_pos >= API_ID_INDEX){
		  _checksumTotal+= _bd;
	  }
	  switch(_pos){
		case 0:
		  if(_bd == START_BYTE){
			  _pos++;
		  }
		  break;
		case 1:
		  _response.setMsbLength(_bd);
		  _pos++;
		  break;
		case 2:
		  _response.setLsbLength(_bd);
		  _pos++;
		  D_ZBEESTACK("\r\n===> Recv start: ");
		  break;
		case 3:
		  _response.setApiId(_bd);
		  _pos++;
		  break;
		default:
		  if(_pos > MAX_FRAME_DATA_SIZE){
			  _response.setErrorCode(PACKET_EXCEEDS_BYTE_ARRAY_LENGTH);
			  return;
		  }else if(_pos == (_response.getPacketLength() + 3)){  // 3 = 2(packet len) + 1(checksum)
			  if((_checksumTotal & 0xff) == 0xff){
				  _response.setChecksum(_bd);
				  _response.setAvailable(true);
				  _response.setErrorCode(NO_ERROR);
			  }else{
				  _response.setErrorCode(CHECKSUM_FAILURE);
			  }
			  _response.setFrameDataLength(_pos - 4);    // 4 = 2(packet len) + 1(Api) + 1(checksum)
			  _pos = 0;
			  _checksumTotal = 0;
			  return;
		  }else{
			  uint8_t* buf = _response.getFrameData();
			  buf[_pos - 4] = _bd;
			  _pos++;
			  if (_response.getApiId() == XB_RX_RESPONSE && _pos == 15){
				  D_ZBEESTACK( "\r\n     Payload: ");
			  }
		  }
		  break;
	  }
	}
}

bool XBee::receiveResponse(XBResponse* response){

    while(true){
    	readPacket();

        if(_response.isAvailable()){
        	D_ZBEESTACK("\r\n<=== CheckSum OK\r\n\n");
			response->absorb(&_response);
            return true;

        }else if(_response.isError()){
        	D_ZBEESTACK("\r\n<=== Packet Error Code = %d\r\n\n",_response.getErrorCode());
			_response.reset();
			response->reset();
            return false;
        }
    }
    return false;
}

void XBee::setSerialPort(SerialPort *serialPort){
    _serialPort = serialPort;
}

void XBee::sendRequest(XBRequest &request){
	D_ZBEESTACK("\r\n===> Send start: ");

	sendByte(START_BYTE, false);

	uint8_t msbLen = ((request.getFrameDataLength() + 1) >> 8) & 0xff; // 1 = 1B(Api)  except Checksum
	uint8_t lsbLen = (request.getFrameDataLength() + 1) & 0xff;
	sendByte(msbLen, true);
	sendByte(lsbLen, true);

	sendByte(request.getApiId(), true);

	uint8_t checksum = 0;
	checksum+= request.getApiId();

	for( int i = 0; i < request.getFrameDataLength(); i++ ){
	  if (request.getApiId() == XB_TX_REQUEST && i == 13){
		  D_ZBEESTACK("\r\n     Payload:    ");
	  }
	  sendByte(request.getFrameData(i), true);
	  checksum+= request.getFrameData(i);
	}
	checksum = 0xff - checksum;
	sendByte(checksum, true);

	//flush();  // clear receive buffer

	D_ZBEESTACK("\r\n<=== Send completed\r\n\n" );
}

void XBee::sendByte(uint8_t b, bool escape){
	if(escape && (b == START_BYTE || b == ESCAPE || b == XON || b == XOFF)){
	  write(ESCAPE);
	  write(b ^ 0x20);
	}else{
	  write(b);
	}
}

void XBee::resetResponse(){
	_pos = 0;
	_escape = 0;
	_response.reset();
}

void XBee::flush(){
	_serialPort->flush();
}

bool XBee::write(uint8_t val){
	return (_serialPort->send(val) ? true : false );
}

bool XBee::read(uint8_t *buff){
	return  _serialPort->recv(buff);
}

/*===========================================
              Class  ZBeeStack
 ============================================*/
ZBeeStack::ZBeeStack(){

}

ZBeeStack::~ZBeeStack(){

}

void ZBeeStack::unicast(XBeeAddress64* addr64, uint16_t addr16,
		uint8_t* payload, uint8_t payloadLength, uint8_t option ){

	_txRequest.setAddress64(addr64);
	_txRequest.setAddress16(addr16);
	_txRequest.setOption(option);
	_txRequest.setPayload(payload);
	_txRequest.setPayloadLength(payloadLength);
	sendRequest(_txRequest);
}

void ZBeeStack::broadcast(uint8_t* payload, uint8_t payloadLength){
	XBeeAddress64 addr;
	addr.setMsb(0);
	addr.setLsb(XB_BROADCAST_ADDRESS32);
	unicast(&addr, XB_BROADCAST_ADDRESS16, payload, payloadLength, 0);
}

bool ZBeeStack::getResponse(XBResponse* response){
	return receiveResponse(response);
}


/*=========================================
       Class SerialPort
 =========================================*/
SerialPort::SerialPort(){
    _tio.c_iflag = IGNBRK | IGNPAR;
    _tio.c_cflag = CS8 | CLOCAL | CREAD | CRTSCTS;
    _tio.c_cc[VINTR] = 0;
    _tio.c_cc[VTIME] = 0;
    _tio.c_cc[VMIN] = 1;
    _fd = 0;
}

SerialPort::~SerialPort(){
	  if (_fd){
		  close(_fd);
	  }
}

int SerialPort::begin(const char* devName, unsigned int flg){
	return begin(devName, B9600, false, 1, flg);
}


int SerialPort::begin(const char* devName, unsigned int boaurate, unsigned int flg){
	return begin(devName, boaurate, false, 1, flg);
}

int SerialPort::begin(const char* devName, unsigned int boaurate, bool parity, unsigned int flg){
	return begin(devName, boaurate, parity, 1, flg);
}

int SerialPort::begin(const char* devName, unsigned int boaurate,  bool parity, unsigned int stopbit, unsigned int flg){
	_fd = open(devName, flg | O_NOCTTY);
	if(_fd < 0){
	  return _fd;
	}

	if (parity){
	  _tio.c_cflag = _tio.c_cflag | PARENB;
	}
	if (stopbit == 2){
	  _tio.c_cflag = _tio.c_cflag | CSTOPB ;
	}
	switch(boaurate){
	case B9600:
	case B19200:
	case B38400:
	case B57600:
	case B115200:
	  if( cfsetspeed(&_tio, boaurate)<0){
		return errno;
	  }
	  break;
	default:
	  return -1;
	}
	return tcsetattr(_fd, TCSANOW, &_tio);
}

bool SerialPort::send(unsigned char b){
	if (write(_fd, &b,1) != 1){
	    return false;
	}else{
		D_ZBEESTACK( " 0x%x", b);
	    return true;
	}
}

bool SerialPort::recv(unsigned char* buf){
	if(read(_fd, buf, 1) == 0){
	    return false;
	}else{
		D_ZBEESTACK( " 0x%x",buf[0] );
	    return true;
	}
}

void SerialPort::flush(void){
	tcsetattr(_fd, TCSAFLUSH, &_tio);
}


