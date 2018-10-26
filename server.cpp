#include <iostream>
#include <Socket.h>
#include <string>

int main(int argc,char *argv[]){
	Socket socket(5555);

	while(!socket.available()){
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	Message message;
	std::string client;
	int port;
	if(socket.getMessage(&message,client,port)){
		std::cout << "got message: " << message.type << std::endl;
		std::cout << "data: ";
		for(int i = 0; i < message.length; i++){
			std::cout << message.data[i];
		}
		std::cout << std::endl;
	}else{
		std::cout << "crc failed" << std::endl;
	}

	std::string data = "hello world!";
	Message responce;
	responce.type = DATA;
	responce.length = data.length();
	memcpy(&responce.data,data.c_str(),data.length());
	socket.write(&responce,client,5556);

	return 0;
}
