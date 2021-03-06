// DamMinhTien_20156599_Tuan9.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "winsock2.h"

typedef struct CLIENT_INFO
{
	OVERLAPPED overlapped;
	SOCKET socket;
	char buf[256];
	WSABUF wsaBuf;
} CLIENT_INFO;

void CALLBACK CompletionRoutine(DWORD, DWORD, LPOVERLAPPED, DWORD);
DWORD WINAPI ClientThread(LPVOID);

int main()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	bind(listener, (SOCKADDR *)&addr, sizeof(addr));
	listen(listener, 5);

	while (1) {
		CLIENT_INFO client;
		client.socket = accept(listener, NULL, NULL);
		CreateThread(0, 0, ClientThread, &client, 0, 0);
	}

	return 0;
}

DWORD WINAPI ClientThread(LPVOID lpParam)
{
	CLIENT_INFO client = *(CLIENT_INFO *)lpParam;
	memset(&client.overlapped, 0, sizeof(client.overlapped));

	client.wsaBuf.buf = client.buf;
	client.wsaBuf.len = sizeof(client.buf);

	DWORD flags = 0;
	DWORD bytesReceived = 0;

	WSARecv(client.socket, &client.wsaBuf, 1, &bytesReceived, &flags, &client.overlapped, CompletionRoutine);

	while (1)
	{
		int ret = SleepEx(5000, TRUE);
		if (ret == 0)
			printf("Timed Out\n");
		else if (ret == WSA_WAIT_IO_COMPLETION)
			printf("IO Completion\n");
	}
}

void CALLBACK CompletionRoutine(DWORD dwError, DWORD dwBytesReceived, LPOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	CLIENT_INFO *pClient = (CLIENT_INFO *)lpOverlapped;

	if (dwError != 0 || dwBytesReceived == 0)
	{
		closesocket(pClient->socket);
		return;
	}

	pClient->buf[dwBytesReceived] = 0;
	printf("Received: %s\n", pClient->buf);

	memset(&pClient->overlapped, 0, sizeof(pClient->overlapped));
	DWORD flags = 0;
	DWORD bytesReceived = 0;

	WSARecv(pClient->socket, &pClient->wsaBuf, 1, &bytesReceived, &flags, lpOverlapped, CompletionRoutine);
}

