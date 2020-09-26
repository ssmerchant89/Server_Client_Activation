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
#define DATAFILENAME "dataFile.txt"
#define IPADDRESS "127.0.0.1"
#define GOODMSG "good"
#define BADMSG "invalid"
#define DEFAULTPORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)
void cleanup(SOCKET socket);


int main(int argc, char* argv[])
{
	WSADATA		wsaData;			// structure to hold info about Windows sockets implementation
	SOCKET		listenSocket;		// socket for listening for incoming connections
	SOCKET		clientSocket;		// socket for communication with the client
	SOCKADDR_IN	serverAddr;			// structure to hold server address information
	string		ipAddress;			// server's IP address
	string		input;				// generic input from user
	char		buffer[BUFFERSIZE];	// buffer to hold message received from server
	int			port;				// server's port number
	int			iResult;			// resulting code from socket functions
	bool		done;				// variable to control communication loop
	bool		moreData;			// variable to control receive data loop
	string		machineID;
	int		    serialNumber;
	bool        lineCheck;
	bool		serialCheck;
	string		tempLine;
	ipAddress = IPADDRESS;
	ifstream	fileOne;
	ofstream	fileTwo;
	serialCheck = false;


	cout << "Activation server\n";
	// If user types in a port number on the command line use it,
	// otherwise, use the default port number
	if (argc > 1)
		port = atoi(argv[1]);
	else
		port = DEFAULTPORT;

	cout << "Running on port number " << port << "\n";
	
	// WSAStartup loads WS2_32.dll (Winsock version 2.2) used in network programming
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		cerr << "WSAStartup failed with error: " << iResult << endl;
		return 1;
	}


	// Create a new socket for communication with the client
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		cerr << "Socket failed with error: " << WSAGetLastError() << endl;
		cleanup(listenSocket);
		return 1;
	}

	// Setup a SOCKADDR_IN structure which will be used to hold address
	// and port information for the server. Notice that the port must be converted
	// from host byte order to network byte order.
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr);


	// Attempt to bind a the local network address to the socket
	iResult = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Bind failed with error: " << WSAGetLastError() << endl;
		cleanup(listenSocket);
		return 1;
	}


	// Start listening for incoming connections
	iResult = listen(listenSocket, 1);
	if (iResult == SOCKET_ERROR)
	{
		cerr << "Listen failed with error: " << WSAGetLastError() << endl;
		cleanup(listenSocket);
		return 1;
	}

	do
	{



		cout << "\nWaiting for connections...\n";

		// Accept an incoming connection; Program pauses here until a connection arrives
		clientSocket = accept(listenSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "Accept failed with error: " << WSAGetLastError() << endl;
			// Need to close listenSocket; clientSocket never opened
			cleanup(listenSocket);
			return 1;
		}

		cout << "Connected...\n\n";

		// Always make sure there's a '\0' at the end of the buffer
		
		
		// String to hold received message
		input = "";

		// Receive until client stops sending data
		moreData = false;
		lineCheck = false;
		done = false;
		fileOne.open("dataFile.txt");
		do
		{
			buffer[BUFFERSIZE - 1] = '\0';
			iResult = recv(clientSocket, buffer, BUFFERSIZE - 1, 0);

			if (iResult > 0)
			{
				// Received data; need to determine if there's more coming				
				if (buffer[iResult - 1] != '\0')
					moreData = true;
				else
					moreData = false;

				// Concatenate received data onto end of string we're building
				input = input + (string)buffer;
				
				// If no more data, get serialNumber and check if it's the correct format.
				if (moreData == false && lineCheck == false)
				{
					try
					{
						serialNumber = stoi(input);
						moreData = true;
						input = "";
						lineCheck = true;
						iResult = send(clientSocket, GOODMSG, 5, 0);
						cout << "Serial:      " << serialNumber  << "      valid \n";
					}
					catch (const std::invalid_argument&) 
					{
						cout << "Serial:      " << input << "      invalid \n";
						input = "Invalid Serial Number.";
						iResult = send(clientSocket, BADMSG, 4, 0);
						input = "";

						if (iResult == SOCKET_ERROR)
						{
							cerr << "Send failed with error: " << WSAGetLastError() << endl;
							return 1;
						}
					}
				}
				// If serial number has been checked and is valid, check the machineID against the serialnumber.
				// If there is no serialNumber associated with the machineID then it isn't in the text file.
				else if (moreData == false && lineCheck == true)
				{
					machineID = input;
					while (getline(fileOne, tempLine))
					{
						if (tempLine.compare(to_string(serialNumber)) == 0)
						{
							getline(fileOne, tempLine);

							if (tempLine.compare(machineID) == 0)
							{
								cout << "Machine Id: " << machineID << "      valid \n";
								iResult = send(clientSocket, GOODMSG, 5, 0);
								fileOne.close();
								fileTwo.close();
								serialCheck = true;

								break;
							}
							else
							{
								cout << "Machine Id: " << machineID << "      Invalid \n";
								iResult = send(clientSocket, BADMSG, 4, 0);
								lineCheck = false;
								fileOne.close();
								fileTwo.close();
								serialCheck = true;

								break;
							}
						}
					}
					// If the serial number and machineID are new, theyre valid and need to be stored in a text file.
					if (serialCheck == false && lineCheck == true)
					{
						fileOne.close();
						fileTwo.open("dataFile.txt", fstream::app);
						cout << "Machine Id: " << machineID << "      valid \n";
						fileTwo << serialNumber << endl << machineID << endl;
						iResult = send(clientSocket, GOODMSG, 5, 0);
						lineCheck = false;
						fileTwo.close();
						serialCheck = false;

						break;
					}
				}
			}
			else if (iResult == 0)
			{
				cout << "Connection closed\n";
				// Need to close clientSocket; listenSocket was already closed
				cleanup(clientSocket);
				return 0;
			}
			else
			{
				cerr << "Recv failed with error: " << WSAGetLastError() << endl;
				cleanup(clientSocket);
				return 1;
			}

		} while (moreData);

		// reset serialCheck for next connection
		serialCheck = false;

	} while (!done);

	return 0;
}


void cleanup(SOCKET socket)
{
	if (socket != INVALID_SOCKET)
		closesocket(socket);

	WSACleanup();
}