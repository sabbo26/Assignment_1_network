#include <iostream>
#include <WS2tcpip.h>
#include <thread>
#include <fstream>
#include <filesystem>
#include <mutex>
#pragma comment (lib,"ws2_32.lib")

using namespace std;

mutex m;

int num_of_connected_clients = 0;


string construct_response(string request) {

	string response;

	int pos = request.find(" ");

	string request_type = request.substr(0, pos);

	request.erase(0, pos + 1);

	pos = request.find(" ");

	string file_path = request.substr(0, pos);

	request.erase(0, pos + 1);


	if (strcmp(request_type.c_str(), "POST") == 0) {

		int pos = request.find("\r\n\r\n");

		while (pos != std::string::npos)
		{
			request.erase(0, pos + 4);
			pos = request.find("\r\n\r\n");
		}
		ofstream my_file(file_path);
		if (my_file.fail()) {
			response = "HTTP/1.1 404 Not Found\r\n\r\n";
		}
		else {
			my_file << request;
			my_file.close();
			response = "HTTP/1.1 200 OK\r\n\r\n";
		}
	}
	else {
		ifstream my_file(file_path);
		if (my_file.fail()) {
			response = "HTTP/1.1 404 Not Found\r\n\r\n";
		}
		else {
			stringstream buff;
			buff << my_file.rdbuf();
			response = "HTTP/1.1 200 OK\r\n\r\n" + buff.str();
			my_file.close();
		}
	}
	return response;

}

void serve_client(SOCKET s ) {

	// buffer to receive client request

	char buffer[4096];

	// fd_set to perform select and time out on client

	fd_set my_set ;

	FD_ZERO(&my_set);

	FD_SET(s, &my_set);
	
	int num_of_received_bytes ;

	timeval time_out;

	// values to set time out

	time_out.tv_usec = 0;

	while ( true ){

		m.lock();
		time_out.tv_sec = 10 - num_of_connected_clients;
		m.unlock();

		int score = select( s + 1 , &my_set, nullptr, nullptr, &time_out );
		
		if (score > 0) {

			// client sends a request before timeout period ends

			ZeroMemory(buffer, 4096);

			num_of_received_bytes = recv(s, buffer, 4096, 0);

			if (num_of_received_bytes <= 0) {
				// client closed connection or an error happened
				m.lock();
				num_of_connected_clients--;
				m.unlock();
				closesocket(s);
				return;
			}
			else {
				// server reveived request successfully

				// print request

				string request = string(buffer, 0, num_of_received_bytes + 1);

				cout << "received request :\n\n" + request + "\n\n" << endl ;

				// parse request and creating response
				
				string response = construct_response( request) ;


				// send response

				int state = send(s, response.c_str() , response.size() + 1  , 0);
				
				if (state == SOCKET_ERROR) {
					// client closed connection 
					m.lock();
					num_of_connected_clients;
					m.unlock();
					closesocket(s);
					return;
				}
			}

		}
		else {
			// time out
			m.lock();
			num_of_connected_clients--;
			m.unlock();
			closesocket(s);
			return;
		}
	}
}


void main() {

	// initialize winsock to use it

	WSADATA data;

	WORD version = MAKEWORD(2, 2);			
	
	int state = WSAStartup(version, &data );

	if (state != 0) {

		cerr << "Error while initializing winsock !! " << endl ;
		return;
	}

	// create a welcoming socket

	SOCKET welcome_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if (welcome_socket == INVALID_SOCKET) {

		cerr << "Error while creating socket !! " << endl;
		WSACleanup();
		return;

	}

	// bind IP address and port number to the welcoming socket

	string my_IP = "192.168.1.3";

	sockaddr_in binding ;

	ZeroMemory(&binding, sizeof(binding));

	binding.sin_family = AF_INET ;

	binding.sin_port = htons( 80 ) ;   

	inet_pton(AF_INET, my_IP.c_str() , &binding.sin_addr);

	state = bind(welcome_socket, ( SOCKADDR *) &binding, sizeof(binding));

	if (state != 0) {

		cerr << "Error binding to IP and port number !!" << endl;
		closesocket(welcome_socket);
		WSACleanup();
		return;
	}

	// put the welcoming socket in listening mode

	state = listen(welcome_socket, SOMAXCONN);

	if (state != 0) {

		cerr << "Error while putting the welcoming socket in listening mode !!" << endl ;
		closesocket(welcome_socket);
		WSACleanup();
		return;
	}

	// accept new connections and create threads to serve clients
		

	while (true) {

		SOCKET new_socket = accept(welcome_socket, nullptr, nullptr);

		if (new_socket == INVALID_SOCKET) {
			cerr << "Error accepting client request to connect !!" << endl;
		}
		else {
			m.lock();
			num_of_connected_clients++;
			m.unlock();
			thread t(serve_client, new_socket);

			t.detach();
		}

	}

	// clean up winsock

	WSACleanup();


}