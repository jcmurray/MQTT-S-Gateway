/*
 * ProcessFramework.h
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
 *  Created on: 2013/10/25
 *      Author: Tomoaki YAMAGUCHI
 *     Version: 1.0.0
 *
 */

#ifndef PROCESSFRAMEWORK_H_
#define PROCESSFRAMEWORK_H_

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <list>
#include <queue>
#include <exception>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>


using namespace std;

/*=================================
 *    Data Type
 ==================================*/
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

/*=====================================
         Class Mutex
  ====================================*/
class Mutex{
public:
	Mutex();
	~Mutex();
	void lock(void);
	void unlock(void);

private:
	pthread_mutex_t _mutex;
};

/*=====================================
         Class Semaphore
  ====================================*/
class Semaphore{
public:
	Semaphore();
	Semaphore(unsigned int);
	~Semaphore();
	void post(void);
	void wait(void);
	void timedwait(uint16_t millsec);

private:
	sem_t _sem;
};

/*=====================================
         Class EventQue
  ====================================*/
template <class T>
class EventQue{
public:
	EventQue();
	~EventQue();
	T*  wait(void);
	T*  timedwait(uint16_t millsec);
    int post(T*);
    int size();

private:
    queue<T*> _que;
    Mutex         _mutex;
    Semaphore     _sem;
};


/*=====================================
         Class Thread
  ====================================*/
#define MAGIC_WORD_FOR_TASK \
	public: void EXECRUN(){try{run();}catch(Exception& ex){ex.writeMessage();\
    if(ex.isFatal()){stopProcess();}}catch(...){throw;}}

class Runnable{
public:
	Runnable(){}
	virtual ~Runnable(){}
	virtual void initialized(int, char**){}
	virtual void EXECRUN(){}
};

class Thread : virtual public Runnable{
public:
	Thread();
    ~Thread();
	int start(void);
	int join(void);
	int cancel(void);
	static pthread_t getID();
	static bool equals(pthread_t*, pthread_t*);
	void initialize(int, char**);
	void stopProcess(void);
	int getArgc();
	char** getArgv();
private:
	pthread_t _threadID;
	Semaphore* _stopProcessEvent;
	int  _argc;
	char** _argv;

	static void* _run(void*);

};

/*=====================================
         Class Process
  ====================================*/
class Process{
public:
	Process(){}
	virtual ~Process(){}
	virtual void initialize(int argc, char** argv){}
	virtual void run(){}

};

extern Process* theProcess;

/*=====================================
         Class MultiTaskProcess
  ====================================*/
class MultiTaskProcess: public Process{
	friend int main(int, char**);
	friend class Thread;
public:
	MultiTaskProcess();
	virtual ~MultiTaskProcess();
	void attach(Thread*);
	int getArgc();
	char** getArgv();
protected:
	void initialize(int argc, char** argv);
	void run();
	Semaphore* getStopProcessEvent();

private:
	int _argc;
	char** _argv;
	list<Thread*> _threadList;
	Semaphore _stopProcessEvent;
};

extern MultiTaskProcess* theMultiTask;


/*=====================================
        Class Exception
 =====================================*/
enum ExceptionType {
	ExInfo = 0,
	ExWarn,
	ExError,
	ExFatal,
	ExDebug
};

#define THROW_EXCEPTION(type, exceptionNo, message) \
	throw Exception(type, exceptionNo, message, __FILE__, __func__, __LINE__)


class Exception : public exception {
public:
    Exception(const ExceptionType type, const int exNo, const string& message);
    Exception(const ExceptionType type, const int exNo, const string& message,
    		   const char*, const char*, const int);
    virtual ~Exception() throw();
    const char* getFileName();
    const char* getFunctionName();
    const int getLineNo();
    const int getExceptionNo();
    virtual const char* what() const throw();
    void writeMessage();
    bool isFatal();

private:
    const char* strType();
    ExceptionType _type;
    int _exNo;
    string _message;
    const char* _fileName;
    const char* _functionName;
    int _line;
};


/*============================================
                Timer
 ============================================*/
class Timer {
public:
    Timer();
    void start(uint32_t msec = 0);
    bool isTimeup(uint32_t msec);
    bool isTimeup(void);
    void stop();
private:
    struct timeval _startTime;
    uint32_t _millis;
};

/*=====================================
        Class EventQue
 =====================================*/
template<class T> EventQue<T>::EventQue(){

}

template<class T> EventQue<T>::~EventQue(){
	_mutex.lock();
	while(!_que.empty()){
		delete _que.front();
		_que.pop();
	}
	_mutex.unlock();
}

template<class T> T* EventQue<T>::wait(void){
	T* ev;
	_sem.wait();
	_mutex.lock();
	ev = _que.front();
	_que.pop();
	_mutex.unlock();
	return ev;
}

template<class T> T* EventQue<T>::timedwait(uint16_t millsec){
	T* ev;
	_sem.timedwait(millsec);
	_mutex.lock();

	if(_que.empty()){
		ev = new T();
		ev->setTimeout();
	}else{
		ev = _que.front();
		_que.pop();
	}
	_mutex.unlock();
	return ev;
}

template<class T> int EventQue<T>::post(T* ev){
	_mutex.lock();
	_que.push(ev);
	_mutex.unlock();
	_sem.post();
	return 0;
}

template<class T> int EventQue<T>::size(){
	_mutex.lock();
	int sz = _que.size();
	_mutex.unlock();
	return sz;
}


#endif /* PROCESSFRAMEWORK_H_ */
