// ChatServerUsingWSAAS.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ChatServerUsingWSAAS.h"
#include "stdio.h"
#include "winsock2.h"	

#define MAX_LOADSTRING 100
#define WM_SOCKET WM_USER + 1


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

												// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

SOCKET listener;
SOCKET client;
SOCKET clients[1024];
SOCKET loggedinClients[1024];
int numClients = 0;
int numLoggedinClients = 0;
char *addr[1024];
char * ids[1024];
SOCKADDR_IN clientAddr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	bind(listener, (SOCKADDR *)&addr, sizeof(addr));
	listen(listener, 5);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CHATSERVERUSINGWSAAS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHATSERVERUSINGWSAAS));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHATSERVERUSINGWSAAS));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CHATSERVERUSINGWSAAS);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 650, 500, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	WSAAsyncSelect(listener, hWnd, WM_SOCKET, FD_ACCEPT);

	CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL,
		10, 10, 160, 350, hWnd, (HMENU)IDC_LIST_CLIENT, GetModuleHandle(NULL), NULL);

	CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL,
		180, 10, 400, 350, hWnd, (HMENU)IDC_LIST_MESSAGE, GetModuleHandle(NULL), NULL);

	CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP, 10, 360, 400, 40,
		hWnd, (HMENU)IDC_EDIT_TEXT, GetModuleHandle(NULL), NULL);

	CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("BUTTON"), TEXT("OK"),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
		420, 360, 150, 40, hWnd, (HMENU)IDC_BUTTON_SEND, GetModuleHandle(NULL), NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam))
		{
			closesocket((SOCKET)wParam);
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT)
		{
			int clientAddrLen = sizeof(clientAddr);
			SOCKET client = accept((SOCKET)wParam, (SOCKADDR *)&clientAddr, &clientAddrLen);

			char buf[256];
			strcpy(buf, inet_ntoa(clientAddr.sin_addr));
			strcat(buf, ":");

			char bport[16];
			_itoa(clientAddr.sin_port, bport, 10);
			strcat(buf, bport);

			addr[numClients] = (char *)malloc(64);
			memcpy(addr[numClients], buf, strlen(buf) + 1);

			SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_ADDSTRING, 0, (LPARAM)buf);

			WSAAsyncSelect(client, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);

			clients[numClients] = client;
			numClients++;
		}
		else if (WSAGETSELECTEVENT(lParam) == FD_READ)
		{
			char buf[256];
			char cmd[16], id[64], tmp[64];
			char targetId[64];
			char sendbuf[256];
			char msg[] = "Syntax error when log in\n";
			client = (SOCKET)wParam;
			int ret = recv(client, buf, sizeof(buf), 0);
			buf[ret] = 0;

			int j = 0;
			for (; j < numClients; j++)
				if (client == loggedinClients[j])
					break;

			if (j == numClients)
			{

				ret = sscanf(buf, "%s %s %s", cmd, id, tmp);
				if (ret == 2)
				{
					if (strcmp(cmd, "client_id:") == 0)
					{
						send(client, "Login successful!", 18, 0);

						ids[numClients] = (char *)malloc(64);
						memcpy(ids[numClients], id, strlen(id) + 1);
						char buf[256];
						strcpy(buf, inet_ntoa(clientAddr.sin_addr));
						strcat(buf, ":");

						char bport[16];
						_itoa(clientAddr.sin_port, bport, 10);
						strcat(buf, bport);
						strcat(buf, ":");
						strcat(buf, id);

						SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_ADDSTRING, 0, (LPARAM)buf);
						loggedinClients[numClients] = client;
						numClients++;
					}
					else
						send(client, msg, strlen(msg), 0);
				}
				else
					send(client, msg, strlen(msg), 0);
			}
			else
			{

				sscanf(buf, "%s", targetId);

				sprintf(sendbuf, "%s: %s", ids[j], buf + strlen(targetId) + 1);

				if (strcmp(targetId, "@all") == 0)
				{
					for (j = 0; j < numClients; j++) {
						if (loggedinClients[j] != client)
							send(loggedinClients[j], sendbuf, strlen(sendbuf), 0);
					}
					SendDlgItemMessageA(hWnd, IDC_LIST_MESSAGE, LB_ADDSTRING, 0, (LPARAM)sendbuf);

				}
				else
				{
					for (j = 0; j < numClients; j++) {
						if (strcmp(targetId + 1, ids[j]) == 0) {
							send(loggedinClients[j], sendbuf, strlen(sendbuf), 0);
						}
					}
					SendDlgItemMessageA(hWnd, IDC_LIST_MESSAGE, LB_ADDSTRING, 0, (LPARAM)sendbuf);

				}
			}
		}
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}