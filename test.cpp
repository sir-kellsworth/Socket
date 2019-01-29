/**********************************************************************
 *
 * Filename:    test.cpp
 * 
 * Description: Example of using the Socket class
 *
 * Notes:
 *
 * 
 * Copyright (c) 2018 by Kelly Klein.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#include <iostream>
#include <Socket.h>
#include <string>

int main(int argc,char *argv[]){
	Socket s1(5555);
	Socket s2(5556);
	std::cout << "made sockets" << std::endl;

	std::string data = "big bad voodoo daddy, super duper";

	Message message;
	message.type = ACK;
	message.length = (unsigned)data.length();
	message.data = (uint8_t *)malloc(message.length);
	memcpy(message.data, data.c_str(), data.length());
	s1.write(&message,"127.0.0.1",5556);

	while(!s2.available()){
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	Message received;
	std::string sender;
	int port;
	bool crc = s2.getMessage(&received,sender,port);
	std::cout << "crc match: " << crc << std::endl;
	std::cout << "type: " << (int)received.type << std::endl;
	std::cout << "size: " << received.length << std::endl;
	std::string d = std::string((char *)received.data,received.length);
	std::cout << "got message " << d << std::endl;

	std::string str = "got dat message brah";
	message.type = ACK;
	message.length = str.length();
	memcpy(message.data,str.c_str(),str.length());
	s2.write(&message,"127.0.0.1",5555);

	while(!s1.available()){
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	crc = s1.getMessage(&received,sender,port);
	std::cout << "crc match: " << crc << std::endl;
	std::cout << "type: " << (int)received.type << std::endl;
	std::cout << "size: " << received.length << std::endl;
	d = std::string((char *)received.data,received.length);
	std::cout << "got message " << d << std::endl;

	double lag = s1.checkLag(10,"127.0.0.1",5556,5555);
	std::cout << "lag time: " << lag << std::endl;

	return 0;
}
