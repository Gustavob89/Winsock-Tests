#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define DEFAULT_ADDR "127.0.0.1"
#define IPADDRLEN 128
#define PORTLEN 16


class WinsockTestClientClass {

private:
    WSADATA wsaData;
    SOCKET connectionSocket = INVALID_SOCKET;
    struct addrinfo* result, * ptr, hints;
    const char * sendBuffer; 
    char receiveBuffer[DEFAULT_BUFLEN],
         serverAddress[IPADDRLEN],
         serverPort[PORTLEN];

public:

    bool clientInitialized;


    WinsockTestClientClass() {
        clientInitialized = false;
        result = NULL;
        ptr = NULL;
        
        std::memcpy(serverAddress, DEFAULT_ADDR, sizeof(DEFAULT_ADDR));
        std::memcpy(serverPort, DEFAULT_PORT, sizeof(DEFAULT_PORT));
    }


    int InitializeWinsock() {
        int iResult  = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (iResult != 0) {
            std::cout << "WSAStartup failed with error: " << iResult << std::endl;
            return 1;
        }

        return 0;
    }


    int ConnectToServer() {

        int iResult;   

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // Resolve the server address and port (default):
        iResult = getaddrinfo(serverAddress, serverPort, &hints, &result);
        if ( iResult != 0 ) {
            std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
            WSACleanup();
            return 1;
        }

        // Attempt to connect to an address until one succeeds:
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

            // Create a SOCKET for connecting to server
            connectionSocket = socket(ptr->ai_family, ptr->ai_socktype,
                ptr->ai_protocol);
            if (connectionSocket == INVALID_SOCKET) {
                std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
                WSACleanup();
                return 1;
            }

            // Connect to server
            iResult = connect(connectionSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR) {
                closesocket(connectionSocket);
                connectionSocket = INVALID_SOCKET;
                continue;
            }
            break;
        }

        freeaddrinfo(result);

        if (connectionSocket == INVALID_SOCKET) {
            std::cout << "Unable to connect to server!" << std::endl;
            WSACleanup();
            return 1;
        }


        clientInitialized = true;
        return 0;
    }
   

    void SetServerAddressPort(const char * pAddress, const char *  pPort) {
        
        std::memcpy(serverAddress, pAddress, IPADDRLEN);
        std::memcpy(serverPort, pPort, PORTLEN);
   }


    int SendFromFile(const char * filePath) {

        std::ifstream inputTxtFile;
        char currentChar, sendBuffer[DEFAULT_BUFLEN];
        int counter1, iResult;
        

        if (clientInitialized == false) {
            std::cout << "Send failed. Socket not initialized" << std::endl;
            return 1;
        }

        inputTxtFile.open(filePath);

        if ( !inputTxtFile.is_open() ) {
            std::cout << "Failed to open file" << std::endl;
            return 2;
        }


        while (inputTxtFile.good()) {
           
            for (counter1 = 0; counter1 < DEFAULT_BUFLEN-1; counter1++) { 
               
                    currentChar = inputTxtFile.get();

                    if (currentChar == EOF) {
                        sendBuffer[counter1] = '\0';
                        break;
                    }
                    else {
                        sendBuffer[counter1] = currentChar;
                    }

            }

            sendBuffer[DEFAULT_BUFLEN - 1] = '\0';
           
            if ( send(connectionSocket, sendBuffer, (int)strlen(sendBuffer), 0)
                                                               == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(connectionSocket);
                WSACleanup();
                return 1;
            }

            //std::cout << sendBuffer;
        }

        return 0;
    }


    int ReceiveFromServer() {

        int iResult;

        if (shutdown(connectionSocket, SD_SEND) == SOCKET_ERROR) {
            std::cout << "Shutdown failed with error:" << WSAGetLastError() << std::endl;
            closesocket(connectionSocket);
            WSACleanup();
        }


        do {
            iResult = recv(connectionSocket, receiveBuffer, DEFAULT_BUFLEN, 0);
            if (iResult > 0)
                printf("Bytes received: %d\n", iResult);
            else if (iResult == 0)
                printf("Connection closed\n");
            else
                printf("recv failed with error: %d\n", WSAGetLastError());

        } while (iResult > 0);

        closesocket(connectionSocket);
        WSACleanup();


        return 0;
    }
  
};


int main() {

    WinsockTestClientClass testClient;

    if (testClient.InitializeWinsock() != 0) {
        getchar();
        return 1;
    }

    // Connect to the server:
    std::cout << "Connecting to address " << DEFAULT_ADDR;
    std::cout << " on Port " << DEFAULT_PORT << std::endl;
    
    testClient.SetServerAddressPort("127.0.0.1", "27015");
    

    std::cout << "Sending SendTest1" << std::endl;
    
    if (testClient.ConnectToServer()!=0) {
        getchar(); 
        return 1;
    }


    if (testClient.SendFromFile("./SendTests/SendTest1.txt") != 0) {
        getchar();
        return 1;
    }
   
    testClient.ReceiveFromServer();
    getchar();


    if (testClient.InitializeWinsock() != 0) {
        getchar();
        return 1;
    }


    std::cout << "Sending SendTest2" << std::endl;

    if (testClient.ConnectToServer() != 0) {
        getchar();
        return 1;
    }

    if (testClient.SendFromFile("./SendTests/SendTest2.txt") != 0) {
        getchar();
        return 1;
    }

    testClient.ReceiveFromServer();
    
    std::cout << std::endl;
    getchar();

    
    std::cout << "Program End" << std::endl;
    getchar();
    return 0;
}


/*  Modified from:
    https://docs.microsoft.com/en-us/windows/win32/winsock/complete-client-code
*/