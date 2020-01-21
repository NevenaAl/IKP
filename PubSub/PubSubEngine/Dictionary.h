#pragma once

#include <winsock2.h>
#include <Windows.h>

struct node {
	SOCKET sendTo;
	HANDLE hSemaphore;
};
