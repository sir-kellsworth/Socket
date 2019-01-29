/**********************************************************************
 *
 * Filename:    Socket.cpp
 * 
 * Description: Implementations of Socket class
 *
 * Notes:
 *
 * 
 * Copyright (c) 2018 by Kelly Klein.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include "Socket.h"

Socket::Socket(int port){
	crcInit();
	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock == -1){
		std::cout << "Could not create socket" << std::endl;
		exit(1);
	}

	server.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	int optval = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		std::cout << "Connection error" << std::endl;
		exit(1);
	}
	messageThread = std::thread([this](){
		while(running){
			checkForMessage();
			//std::this_thread::sleep_for(std::chrono::milliseconds(30));
		}
	});
}

int Socket::available(){
	return pendingMessages.size() > 0 ? true : false;
}

bool Socket::getMessage(Message *message,std::string &sender,int &port){
	if(!available()){
		return false;
	}
	readingMutex.lock();
	StoredPacket packet = pendingMessages.front();
	memcpy(message,&packet.message,sizeof(Message));
	sender = packet.sender;
	port = packet.port;
	pendingMessages.pop_front();

	readingMutex.unlock();
	return checkCRC(message);
}

void Socket::checkForMessage(){
	struct sockaddr_in client;
	int length = sizeof(client);
	uint8_t *buffer = new uint8_t[MAX_SIZE];

	//this method is blocking
	int bytes_read = recvfrom(sock,buffer,MAX_SIZE,0,(struct sockaddr *)&client, (socklen_t *)&length);
	if(bytes_read > 0){
		Message *message = new Message;
		std::string sender;
		int port;

		sender = inet_ntoa(client.sin_addr);
		port = ntohs(client.sin_port);
		StoredPacket packet;
		message->type = buffer[0];
		message->length = (buffer[1] << 8) + buffer[2];
		/*std::cout << "got message: ";
		for(int i = 0; i < bytes_read; i++){
			std::cout << (int)buffer[i] << " ";
		}
		std::cout << std::endl;
		std::cout << (int)buffer[0] << " " << (int)buffer[1] << " " << (int)buffer[2] << std::endl;
		std::cout << "got length: " << message->length << std::endl;
		std::cout << "read num Bytes: " << bytes_read << std::endl;*/
		buffer += 3;
		message->data = (uint8_t *)malloc(message->length);
		memcpy(message->data,buffer,message->length);
		buffer += message->length;
		message->crc = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
		packet.sender = sender;
		packet.port = port;
		packet.message = *message;
		packet.timestamp = std::chrono::high_resolution_clock::now();
		if(message->type == LAG){
			if(message->data[0] == PING){
				int lagPort = (message->data[1] << 8) + message->data[2];
				message->data[0] = PONG;
				write(message,sender,lagPort);
			}else{
				lagTestingMessages.push(packet);
			}
		}else{
			pendingMessages.emplace_back(packet);
		}
	}else if(bytes_read == 0){
		//std::cout << "read 0 bytes" << std::endl;
		//socket closed...
	}else if(bytes_read == -1){
		std::cout << "got an error? " << std::endl;
		//some error...probably need to ask for a resend
	}
}

void Socket::write(Message *message,std::string address,int port){
	writingMutex.lock();

	struct sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr(address.c_str());
	dest.sin_port = htons(port);

	generateCRC(message);

	int total = 0;
	int bytesLeft = message->length+7;//sizeof(message);
	int n;
	uint8_t *buffer = (uint8_t *)malloc(message->length+7);
	buffer[0] = message->type;
	buffer[1] = message->length >> 8;
	buffer[2] = message->length;
	buffer += 3;
	memcpy(buffer,message->data,message->length);
	buffer -= 3;
	buffer[message->length+3] = message->crc >> 24;
	buffer[message->length+4] = message->crc >> 16;
	buffer[message->length+5] = message->crc >> 8;
	buffer[message->length+6] = message->crc;
	/*std::cout << "sending message: ";
	for(int i = 0; i < sizeof(buffer); i++){
		std::cout << (int)buffer[i] << " ";
	}
	std::cout << std::endl;*/

	while(total < bytesLeft){
		n = sendto(sock,buffer,message->length+7,0,(struct sockaddr *)&dest,sizeof(dest));
		if(n == -1){break;}
		total += n;
		bytesLeft -= n;
	}
	//std::cout << "sent: " << total << " bytes out of " << message->length+7 << " total" << std::endl;

	writingMutex.unlock();
	buffer = NULL;
}

bool Socket::checkCRC(Message *message){
	int size = message->length+3;//sizeof(Message) - sizeof(uint32_t);
	unsigned char *buffer = (unsigned char*)malloc(size);
	buffer[0] = message->type;
	buffer[1] = message->length >> 8;
	buffer[2] = message->length;
	for(int i = 0; i < message->length; i++){
		buffer[3+i] = message->data[i];
	}
	std::cout << std::endl;
	uint32_t crc = crcFast(buffer,size);//crcFast(reinterpret_cast<const unsigned char*>(message), size);
	if(crc == message->crc){
		return true;
	}else{
		return false;
	}

	buffer = NULL;
}

void Socket::generateCRC(Message *message){
	int size = message->length+3;//sizeof(Message) - sizeof(uint32_t);
	unsigned char *buffer = (unsigned char*)malloc(size);
	buffer[0] = message->type;
	buffer[1] = message->length >> 8;
	buffer[2] = message->length;
	for(int i = 0; i < message->length; i++){
		buffer[3+i] = message->data[i];
	}
	uint32_t crc = crcFast(buffer,size);//crcFast(reinterpret_cast<const unsigned char*>(message), size);
	message->crc = crc;

	buffer = NULL;
}

double Socket::checkLag(int sampleSize,std::string address,int port,uint16_t receivePort){
	double sum = 0;
	Message *test = new Message;
	test->length = 0x03;
	test->data = (uint8_t *)malloc(test->length);
	test->type = LAG;
	test->data[0] = PING;
	test->data[1] = receivePort >> 8;
	test->data[2] = receivePort;
	for(int i = 0; i < sampleSize; i++){
		/*std::cout << "sending data: " << (int)test->type << " " << (int)(test->length >> 8) << " " << (int)test->length << std::endl;
		std::cout << "to " << address << " at port: " << port << std::endl;
		std::cout << "data: " << (int)test->data[0] << " " << (int)test->data[1] << " " << (int)test->data[2] << std::endl;*/
		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		write(test,address,port);
		while(lagTestingMessages.empty()){
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
		StoredPacket message = lagTestingMessages.front();
		lagTestingMessages.pop();

		std::chrono::duration<double, std::milli> span = message.timestamp - t1;
		//lag time is just time it takes to get from the sender to the reciever
		//about 1/2 the time...
		sum += (span.count() / 2);
	}
	delete(test);

	return sum / sampleSize;
}

Socket::~Socket(){
	shutdown(sock,SHUT_RDWR);
	close(sock);
	running = false;
	messageThread.join();
}
