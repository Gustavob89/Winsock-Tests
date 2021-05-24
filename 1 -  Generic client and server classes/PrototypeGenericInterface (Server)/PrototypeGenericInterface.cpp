
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define PORTLEN 16

class PrototypeIntefaceClass {

private:

    SOCKET ListenSocket, ClientSocket;

    struct addrinfo* result;
    struct addrinfo hints;

    char receiveBuffer[DEFAULT_BUFLEN];
    int receiveBufferLength = DEFAULT_BUFLEN;

    WSADATA wsaData;


public: 

    char serverPort[PORTLEN];
    bool serverInitialized;


    PrototypeIntefaceClass() {

        serverInitialized = false;

        ListenSocket = INVALID_SOCKET;
        ClientSocket = INVALID_SOCKET;
        result = NULL;

        std::memcpy(serverPort, DEFAULT_PORT, sizeof(DEFAULT_PORT));
    }


    int InitializeWinsock() {
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (iResult != 0) {
            std::cout << "WSAStartup failed with error: " << iResult << std::endl;
            return 1;
        }

        return 0;
    }


    void SetServerPort(const char * pPort) {

        std::memcpy(serverPort, pPort, PORTLEN);
    }


    int InitializeServer() {

        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        if (iResult != 0) {
            std::cout << "WSAStartup failed with error: " << iResult << std::endl;
            return 1;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
            WSACleanup();
            return 1;
        }

        // Create a SOCKET for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET) {
            std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        // Setup the TCP listening socket
        iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }


        serverInitialized = true;
        return 0;
    }


    int Listen() {

        int iResult, iSendResult;

        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        else {
            std::cout << "Accepted connection" << std::endl;
        }

         //No longer need server socket
        closesocket(ListenSocket);

        // Receive until the peer shuts down the connection
        do {

            iResult = recv(ClientSocket, receiveBuffer, receiveBufferLength, 0);
            std::cout << receiveBuffer << std::endl;

            if (iResult > 0) {
                printf("Bytes received: %d\n", iResult);

                // Echo the buffer back to the sender
                iSendResult = send(ClientSocket, receiveBuffer, iResult, 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    WSACleanup();
                    return 1;
                }
                printf("Bytes sent: %d\n", iSendResult);
            }
            else if (iResult == 0)
                printf("Connection closing...\n");
            else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }

        } while (iResult > 0);

        
        // shutdown the connection since we're done
        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
        
        closesocket(ClientSocket);
        WSACleanup();

        return 0;
    }

};



int main() {

    PrototypeIntefaceClass testInterface;

    testInterface.InitializeWinsock();

    testInterface.SetServerPort("65000");

    
    while (true) {

        std::cout << std::endl;
        std::cout << " Server initialized, listening on port: " << testInterface.serverPort << std::endl;

        if (testInterface.InitializeServer() != 0) {
            std::cout << "Server failed to initialize" << std::endl;
            getchar();
            return 1;
        }

        if (testInterface.Listen() != 0) {
            getchar();
            return 1;
        }
    }
    
    std::cout << " Program end" << std::endl;
    getchar();
    return 0;
}

//Modified from: https://docs.microsoft.com/en-us/windows/win32/winsock/complete-server-code