# SOCKET COMMUNICATION

Linux systems, Socket communication using libev library

## Building
		The libraries and packages required to build the project are 
		pthread and libev.
		Currently the address of the server is 192.168.1.15 (e.g. SERVER_IP=192.168.1.15).
		To build locally the project on your desktop, the first task is 
		to mount (Linux) the remote directory that contains the required libraries.
		
		Libraries
		=========
		This project includes libev-4.0 and pthread libraries
		
		Build
		=====
		Clone the project and create the build directory	
		Move into build directory
		
		-Linux
		======
			cmake ..
			make

## Running
		Move into build/src/ folder, where the binaries are.
		
		-Linux
		======

		Running the server:
			./socket_server
		Running the client
			./socket_client

		Note: You can ask the server to solve a problem, and then request your results. The solver is
		the a dummy example.
