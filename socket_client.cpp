/*! 
 *  \brief    Socket client using libev
 *  \author    Katsileros Petros
 *  \date      06/11/2015
 *  \copyright 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libev/ev++.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h> 

class sockClient {
private:
	int     s;
	ev::io cl_watcher;
	ev::io idle_cl_watcher;
	struct sockaddr_in server_addr;
		
public:
	
	sockClient(int port) 
	{
		server_addr.sin_family = AF_INET;
		server_addr.sin_port     = htons(8192);
		server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		
		char* some_addr = inet_ntoa(server_addr.sin_addr);
		std::cout << "Client speaking to " << some_addr << ", on port " << port << std::endl;
		
		// Create the socket
		s = socket(PF_INET, SOCK_STREAM, 0);
		if(s == -1) {
			std::cout << "Error - socket" << std::endl;
			exit(EXIT_FAILURE);
		}
		
		if( connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
			printf("Error : Connect Failed \n");
			exit(1);
		}
			
		// Execute the event loops func
		this->evEventLoops();
	}
	
	// effictivly a close and a destroy
	virtual ~sockClient() {
		// Stop and free watcher if client socket is closing
		cl_watcher.stop();
		idle_cl_watcher.stop();

		close(s);
	}
		
	// Socket is writable
	void callback(ev::io &watcher, int revents) 
	{
		if (EV_ERROR & revents) {
			std::cout << "got invalid event" << std::endl;
			return;
		}
		
		cl_watcher.stop();
		
		if(revents & EV_WRITE) {
			//~ std::cout << "write" << std::endl;
			idle_cl_watcher.set<sockClient, &sockClient::send_cb>(this);
			idle_cl_watcher.start(s,ev::WRITE);
		}
		else if(revents & EV_READ) {
			//~ std::cout << "read" << std::endl;
			idle_cl_watcher.set<sockClient, &sockClient::recv_cb>(this);
			idle_cl_watcher.start(s,ev::READ);
		}
	}
	
	void send_cb(ev::io &watcher, int revents) 
	{
		char sendBuff[1025] = {"\0"};
		std::cout << "Write your request to the server" << std::endl;
		std::cin.getline(sendBuff,sizeof(sendBuff));
		send(s, sendBuff, strlen(sendBuff),ev::WRITE); 
		
		idle_cl_watcher.stop();
		cl_watcher.start(s, ev::READ);
	}
	
	void recv_cb(ev::io &watcher, int revents)
	{
		char recvBuff[1025] = {"\0"};
		read(s, recvBuff, sizeof(recvBuff));
		std::cout << "Server said: " << recvBuff << std::endl;
		
		idle_cl_watcher.stop();
		cl_watcher.start(s, ev::WRITE);
	}

	
	void evEventLoops()
	{
		ev::default_loop loop(EVFLAG_AUTO);
		cl_watcher.set<sockClient, &sockClient::callback>(this);
		cl_watcher.start(s, ev::WRITE);
		loop.run(0);
	}
};

int main(int argc, char **argv)
{
	int port = 8192;
	sockClient	cl(port);
	
	return 0;
}

