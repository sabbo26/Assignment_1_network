#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <fstream>
#include <iphlpapi.h>

#pragma comment (lib , "ws2_32.lib")

using namespace std;

string parse_request( string request , ifstream* post_data ) {

	string server_request = "";

	int pos = request.find(" ");

	string request_type = request.substr(0, pos);

	request.erase(0, pos + 1);

	pos = request.find(" ");

	string file_path = request.substr(0, pos);

	string version = request.substr(pos + 1, request.size());

	if (strcmp(request_type.c_str(), "GET") == 0) {
		server_request = server_request + "GET " + file_path + " " + version + "\r\n";
		server_request = server_request + "\r\n";
	}
	else {
		server_request = server_request + "POST " + file_path + " " + version + "\r\n";
		server_request = server_request + "\r\n";
		string post_data_line;
		getline(*post_data, post_data_line);
		server_request = server_request + post_data_line.substr(0, post_data_line.size());
	}
	return server_request;
}


void main() {

	// setting IP address and port number of server

	string server_IP = "192.168.1.3";
	
	int server_port_num = 80 ;

	// initialize winsock

	WSADATA data;

	WORD version = MAKEWORD(2, 2);

	int state = WSAStartup(version, &data );

	if (state != 0) {
		// error initializing winsock
		cerr << "Error while initializing WSA !! " << endl ;
		return;

	}
	
	// creating socket to connect to server

	SOCKET my_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (my_socket == INVALID_SOCKET) {
		// error while creating socket
		cerr << "Error while creating client socket !!" << endl;
		WSACleanup();
		return;

	}

	// setting addr struct for server and connect to the server

	sockaddr_in addr;

	ZeroMemory(&addr, sizeof(addr));

	addr.sin_family = AF_INET;

	addr.sin_port = htons(80);

	inet_pton(AF_INET, server_IP.c_str(), &addr.sin_addr);

	state = connect(my_socket, (sockaddr*)&addr, sizeof(addr));

	if (state != 0) {
		// error while connecting to server
		cerr << "Error while connecting to server !! " << endl;
		closesocket(my_socket);
		WSACleanup();
		return ;
	}


	// exchanging messages with server
	// buffer to receive server responses
	char buffer[4096];

	string request ;

	// requests and data of post requests files

	ifstream requests("requests.txt");

	ifstream post_data("post_data.txt");

	// to control sending the same request or going on

	bool change_request = true ;

	string server_request;

	while ( true ) {

		if (change_request) {
			//checking for requests end
			if ( requests.eof() )
				break;

			// parsing request

			getline(requests, request);

			server_request =  parse_request( request , &post_data ) ;

		}

		// send request to server

		int num_of_bytes = send(my_socket, server_request.c_str(), server_request.size() + 1, 0);

		if (num_of_bytes != SOCKET_ERROR) {

			// request sent successfully

			ZeroMemory(buffer, 4096);

			int num = recv(my_socket, buffer, 4096, 0);

			if (num > 0) {

				// client received response succssfully

				cout << "received response :\n\n" + string(buffer, 0, num) + "\n" << endl;

				change_request = true ;

			}
			else {
				// server closed connection or error occured
				// connect and send the same request again

				do {
					state = connect(my_socket, (sockaddr*)&addr, sizeof(addr));
				} while (state != 0);
				change_request = false;

			}
			
		}
		else {
			// if connection closed due to time out, connect again and send the same request
			do {
				state = connect(my_socket, (sockaddr*)&addr, sizeof(addr));
			} while (state != 0);
			change_request = false;
		}

	}

	requests.close();

	post_data.close();

	closesocket(my_socket);
	
	WSACleanup();

}
