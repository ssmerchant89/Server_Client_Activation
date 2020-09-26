#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <string>
#include <fstream>
using namespace std;

// Make sure build environment links to Winsock library file
#pragma comment(lib, "Ws2_32.lib")


// Define default global constants
#define BUFFERSIZE 256
#define ACTIVATIONFILENAME "actFile.txt"
#define IPADDRESS "127.0.0.1"
#define GOODMSG "good"
#define BADMSG "invalid"
#define DEFAULTPORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)
void cleanup(SOCKET socket);


int main(int argc, char* argv[])
{
	WSADATA		wsaData;			// structure to hold info about Windows sockets implementation
	SOCKET		mySocket;			// socket for communication with the server
	SOCKADDR_IN	serverAddr;			// structure to hold server address information
	string		ipAddress;			// server's IP address
	string		machineID;			// user's machine ID
	string		serialNum;			// user's serial number
	string		fileLine;			// one line from the actFile.txt
	string		input;				// generic input from user
	char		buffer[BUFFERSIZE];	// buffer to hold message received from server
	int			port;				// server's port number
	int			iResult;			// resulting code from socket functions
	bool		done;				// variable to control communication loop
	bool		moreData;			// variable to control receive data loop

	// If user types in a port number on the command line use it,
	// otherwise, use the default port number
	if (argc > 1)
		port = atoi(argv[1]);
	else
		port = DEFAULTPORT; 

	ipAddress = IPADDRESS;
	cout << "Enter your machine id: ";
	cin >> machineID;

	ifstream inFile;
	inFile.open(ACTIVATIONFILENAME);
	if (!inFile) {
		cout << "\nYou must activate this program in order to continue using it.\n\n";
		cout << "Enter your serial number: ";
		cin >> serialNum;
	}
	else {
		while (inFile >> fileLine) {
			if (fileLine == machineID) {
				cout << "\nThis program has already been activated.";
				exit(1);
			}
		}
		cout << "\nActivation data is invalid.\n";
		cout << "You must activate this program in order to continue using it.\n\n";
		cout << "Enter your serial number: ";
		cin >> serialNum;
	}

	// WSAStartup loads WS2_32.dll (Winsock version 2.2) used in network programming
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		cerr << "WSAStartup failed with error: " << iResult << endl;
		return 1;
	}

	// Create a new socket for communication with the server
	mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (mySocket == INVALID_SOCKET)
	{
		cerr << "Socket failed with error: " << WSAGetLastError() << endl;
		cleanup(mySocket);
		return 1;
	}

	// Setup a SOCKADDR_IN structure which will be used to hold address
	// and port information for the server. Notice that the port must be converted
	// from host byte order to network byte order.
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);

	// Try to connect to server
	iResult = connect(mySocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Connect failed with error: " << WSAGetLastError() << endl;
		cleanup(mySocket);
		return 1;
	}

	// Always make sure there's a '\0' at the end of the buffer
	buffer[BUFFERSIZE - 1] = '\0';

	// Loop to communicate with server until response received from server.
	done = false;
	do
	{
		// Send serialNum to server

		iResult = send(mySocket, serialNum.c_str(), (int)serialNum.size() + 1, 0);
		if (iResult == SOCKET_ERROR)
		{
			cerr << "Send failed with error: " << WSAGetLastError() << endl;
			cleanup(mySocket);
			return 1;
		}


		if (!done)
		{
			// Wait to receive a reply message back from the server
			cout << "\nContacting activation server...\n\n";

			// String to hold received message
			input = "";

			// Receive until server stops sending data
			moreData = false;
			do
			{
				iResult = recv(mySocket, buffer, BUFFERSIZE - 1, 0);

				if (iResult > 0)
				{
					// Received data; need to determine if there's more coming				
					if (buffer[iResult - 1] != '\0')
						moreData = true;
					else
						moreData = false;

					// Concatenate received data onto end of string we're building
					input = input + (string)buffer;
				}
				else if (iResult == 0)
				{
					cout << "Connection closed\n";
					cleanup(mySocket);
					return 0;
				}
				else
				{
					cerr << "Recv failed with error: " << WSAGetLastError() << endl;
					cleanup(mySocket);
					return 1;
				}

			} while (moreData);

			if (input == GOODMSG) {
				input = "";
				// Send machineID to server

				iResult = send(mySocket, machineID.c_str(), (int)machineID.size() + 1, 0);
				if (iResult == SOCKET_ERROR)
				{
					cerr << "Send failed with error: " << WSAGetLastError() << endl;
					cleanup(mySocket);
					return 1;
				}

				moreData = false;
				do
				{
					iResult = recv(mySocket, buffer, BUFFERSIZE - 1, 0);

					if (iResult > 0)
					{
						// Received data; need to determine if there's more coming				
						if (buffer[iResult - 1] != '\0')
							moreData = true;
						else
							moreData = false;

						// Concatenate received data onto end of string we're building
						input = input + (string)buffer;
					}
					else if (iResult == 0)
					{
						cout << "Connection closed\n";
						cleanup(mySocket);
						return 0;
					}
					else
					{
						cerr << "Recv failed with error: " << WSAGetLastError() << endl;
						cleanup(mySocket);
						return 1;
					}

				} while (moreData);

				if (input == GOODMSG) {
					cout << "Activation was successful!";
					ofstream outfile;
					outfile.open("actFile.txt");
					outfile << machineID << endl;
					cleanup(mySocket);
					return 0;
				}
				else {
					cout << input << endl;
					cout << "Program is activated on another machine.\n";
					cout << "Activation was not successful.";
					cleanup(mySocket);
					exit(1);
				}

			}
			else {
				cout << "Invalid Serial Number.";
				cleanup(mySocket);
				exit(1);
			}
			done = true;
		}
	} while (!done);
}


void cleanup(SOCKET socket)
{
	if (socket != INVALID_SOCKET)
		closesocket(socket);

	WSACleanup();
}
