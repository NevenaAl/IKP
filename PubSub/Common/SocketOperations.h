

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "Queue.h"


#define SAFE_DELETE_HANDLE(h) {if(h)CloseHandle(h);}
///<summary>
/// Generates Message Struct with a header containing the lenght of a message.
///</summary>
///<param name ="message">Message.</param>
///<param name ="len">Length of a message.</param>
///<returns>Message Struct created.</returns>
MessageStruct* GenerateMessageStruct(char* message, int len) {

	MessageStruct* messageStruct = (MessageStruct *)(malloc(sizeof(MessageStruct)));

	messageStruct->header = len;
	memcpy(messageStruct->message, message, len);

	return messageStruct;

}
///<summary>
/// Initializes sockets.
///</summary>
///<returns>Error bool.</returns>
bool InitializeWindowsSockets()
{
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}