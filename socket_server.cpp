/*! 
 *  \brief     Multithread socket communication server, using libev
 *  \author    Katsileros Petros
 *  \date      06/11/2015
 *  \copyright 
 */

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <libev/ev++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <resolv.h>
#include <errno.h>
#include <list>

#include <pthread.h>      
#include "solver.h"

#include <sys/syscall.h>

pthread_mutex_t mutex;
pthread_cond_t cond;
int flag_var = 0;
int solving_var = 0;
  
//
//   Buffer class - allow for output buffering such that it can be written out 
//                                 into async pieces
//
struct Buffer {
	char *data;
	ssize_t len;
	ssize_t pos;

	Buffer(const char *bytes, ssize_t nbytes) {
		pos = 0;
		len = nbytes;
		data = new char[nbytes];
		memcpy(data, bytes, nbytes);
	}

	virtual ~Buffer() {
		delete [] data;
	}

	char *dpos() {
		return data + pos;
	}

	ssize_t nbytes() {
		return len - pos;
	}
};
  
//
//   A single instance of a non-blocking Echo handler
//
class EchoInstance {
private:
	ev::io           io;
	static int total_clients;
	int              sfd;
	  
	// Buffers that are pending write
	std::list<Buffer*>     write_queue;
 
	// Generic callback
	void callback(ev::io &watcher, int revents) {
		if (EV_ERROR & revents) {
				perror("got invalid event");
				return;
		}

		if (revents & EV_READ) 
				read_cb(watcher);

		if (revents & EV_WRITE) 
				write_cb(watcher);

		if (write_queue.empty()) {
				io.set(ev::READ);
		} 
		else {
				io.set(ev::READ|ev::WRITE);
		}
	}

	// Socket is writable
	void write_cb(ev::io &watcher) {
		if (write_queue.empty()) {
			io.set(ev::READ);
			return;
		}

		Buffer* buffer = write_queue.front();
				  
		ssize_t written = write(watcher.fd, buffer->dpos(), buffer->nbytes());
		if (written < 0) {
			perror("read error");
			return;
		}

		buffer->pos += written;
		if (buffer->nbytes() == 0) {
			write_queue.pop_front();
			delete buffer;
		}
	}

	// Receive message from client socket
	void read_cb(ev::io &watcher) {
		char buffer[1024] = {"\0"};

		ssize_t nread = recv(watcher.fd, buffer, sizeof(buffer), 0);

		if (nread < 0) {
			perror("read error");
			return;
		}

		if (nread == 0) {
				// Gack - we're deleting ourself inside of ourself!
			delete this;
		} 
		else {
			
			// Send message back to the client
			std::cout << "Client said: " << buffer << std::endl;
						
			if(!strcmp(buffer,"solve"))
			{
				if(solving_var == 99) // 99 Means the solver is still solving
				{
					strcpy(buffer,"Solver is still executing");
					write_queue.push_back(new Buffer(buffer, sizeof(buffer)));
				}
				else // You can ask solver to solve
				{
					
					pthread_mutex_lock(&mutex);
					flag_var = 2; // ex. 2 means solve
					pthread_cond_signal(&cond);
					pthread_mutex_unlock(&mutex);
					 
					strcpy(buffer,"Solver start solving");
					write_queue.push_back(new Buffer(buffer, sizeof(buffer)));
				}
			}
			else if(!strcmp(buffer,"results"))
			{	
				if(solving_var == 100) // 100 means the solver has finished his job
				{	
					pthread_mutex_lock(&mutex);
					flag_var = 3; // ex. 3 means resutls
					pthread_cond_signal(&cond);
					pthread_mutex_unlock(&mutex);

					// wait signal from solver that the resutls are ready to read
					
					pthread_mutex_lock(&mutex);
					pthread_cond_wait(&cond,&mutex);
					pthread_mutex_unlock(&mutex);
					
					std::cout << "Server: Received results from solver" << std::endl;
					std::cout << "Server: Sending to client" << std::endl;
					
					// Send results to client using libev socket communication
					strcpy(buffer,"Hey client, take your results");
					write_queue.push_back(new Buffer(buffer, sizeof(buffer)));
				}
				else if(solving_var == 99) // 99 Means the solver is still solving
				{
					strcpy(buffer,"Solver is still executing");
					write_queue.push_back(new Buffer(buffer, sizeof(buffer)));
				}
				else if(solving_var == 98) // 98 Means the solver has never been executed
				{
					strcpy(buffer,"Solver did not executed at all");
					write_queue.push_back(new Buffer(buffer, sizeof(buffer)));
				}
			}
			else
			{
				//~ std::cout << "Client said: " << buffer << std::endl;
				strcpy(buffer,"Unknown command");
				write_queue.push_back(new Buffer(buffer, sizeof(buffer)));
			}
		}
	}

	// effictivly a close and a destroy
	virtual ~EchoInstance() {
		// Stop and free watcher if client socket is closing
		io.stop();

		close(sfd);
		
		flag_var = 1;
		printf("%d client(s) connected.\n", --total_clients);
	}
  
public:
	EchoInstance(int s) : sfd(s) {
		fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); 
		
		printf("Got connection\n");
		total_clients++;

		io.set<EchoInstance, &EchoInstance::callback>(this);

		io.start(s, ev::READ);
	}
};
  
class EchoServer {
	
private:
	ev::io	 io;
	ev::sig sio;
	int       s;
  
public:
	EchoServer(int port) 
	{
		printf("Listening on port %d\n", port);

		struct sockaddr_in addr;

		s = socket(PF_INET, SOCK_STREAM, 0);

		addr.sin_family = AF_INET;
		addr.sin_port     = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
				perror("bind");
		}

		fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); 
		
		listen(s, 5);
		
		io.set<EchoServer, &EchoServer::io_accept>(this);
		io.start(s, ev::READ);
	}
          
	virtual ~EchoServer() {
		flag_var = 0;
		shutdown(s, SHUT_RDWR);
		close(s);
	}

	void io_accept(ev::io &watcher, int revents) 
	{
		if (EV_ERROR & revents) {
				perror("got invalid event");
				return;
		}
	
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
	
		int client_sd = accept(watcher.fd, (struct sockaddr *)&client_addr, &client_len);
	
		if (client_sd < 0) {
				perror("accept error");
				return;
		}
		
		flag_var = 1;
		EchoInstance *client = new EchoInstance(client_sd);
	}
};
 
void *create_solver(void *threadid)
{
	// Create the solver object
	solver* solver_obj = new solver();
	//~ pid_t tid = (pid_t) syscall (SYS_gettid);
	//~ std::cout << "solver thread: " << tid << std::endl;
	solving_var = 98;
	
	while(flag_var){
		//~ std::cout << "flag_var: " << flag_var << std::endl;
		
		if(flag_var == 1) // Wait signal from server
		{
			pthread_mutex_lock(&mutex);
			pthread_cond_wait(&cond,&mutex);
			std::cout << "Solver: Received signal from server" << std::endl;
			pthread_mutex_unlock(&mutex);
		}
		if(flag_var == 2) // solve
		{
			long tid;
			//~ tid = (long)threadid;
			//~ std::cout << "Running solver with thread-" << tid << std::endl;
			solving_var = 99;
			flag_var = 1;
			solver_obj->run(32);
			flag_var = 1;
			solving_var = 100;
			
			std::cout << "Finished" << std::endl;
		}
		else if(flag_var == 3) // get results
		{
			if(solver_obj->flag == 1) // solver flag = 1 means the solver finished the job
			{
				std::cout << "Solver sending the results to server" << std::endl;
				solving_var = 100;
				flag_var = 1;
				
				pthread_mutex_lock(&mutex);
				pthread_cond_signal(&cond);
				pthread_mutex_unlock(&mutex);
			}
			else if(solver_obj->flag == 0 & solver_obj->executed == 1) // flag = 0 AND executed = 1 means solver is still solving
			{
				std::cout << "Try again later. The solver still solving." << std::endl;

			}
			else if(solver_obj->flag == 0 & solver_obj->executed == 0) // flag = executed = 0,  the solver did not executed
			{
				std::cout << "Solver did not executed" << std::endl;
				solving_var = 98;
				flag_var = 1;
			}
		
			
		}
	}	
}


int EchoInstance::total_clients = 1;
  
int main(int argc, char **argv) 
{
	int port = 8192;
	if (argc > 1)
			port = atoi(argv[1]);

	// pthread condition & lock
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	/* Initialize mutex and condition variable objects */
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init (&cond, NULL);

	// Create solver thread
	flag_var = 1;
	pthread_t solver_thread[1];
	int rc = pthread_create(&solver_thread[0],NULL,create_solver,(void*)1);
	
	sleep(1);	
	
	//~ pid_t tid = (pid_t) syscall (SYS_gettid);
	//~ std::cout << "main thread: " << tid << std::endl;
		
	ev::default_loop loop;
	EchoServer	echo(port);
	loop.run(0);
	
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	pthread_exit(NULL);
	return 0;
}


