#pragma once
#define WIN32_LEAN_AND_MEAN
// 必要的头文件
#include <iostream>
#include <string>
// WinSock核心头文件

#include <winsock2.h>
#include <ws2tcpip.h>

// 链接WinSock库（必须）
#pragma comment(lib, "ws2_32.lib")

// 定义缓冲区大小
#define DEFAULT_BUFFER_SIZE 512



class web_utils
{
	
	//single instance
	static web_utils* SingleInstance;
public:
	// delete copy
	web_utils(const web_utils&) = delete;
	web_utils& operator=(const web_utils&) = delete;

	//Get single instance pointer
	static web_utils* Get() {
		if (SingleInstance == nullptr) {
			SingleInstance = new web_utils();
		}
		return SingleInstance;
	}

	//Initialize WinSock
	bool InitWebSock();
	// Apply to connect to server
	bool ConnectToServer();
	//Cleanup WinSock
	void CleanupWebSock();
private:
	web_utils() = default;

	// server info
	const char* server_ip = "127.0.0.1";
	u_short server_port = 8080;

	// WinSock variables
	WSADATA wsaData = {};
	SOCKET client_socket = INVALID_SOCKET;
	sockaddr_in server_address = {};
};

