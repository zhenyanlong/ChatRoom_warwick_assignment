#pragma once
#define WIN32_LEAN_AND_MEAN
// 必要的头文件
#include <iostream>
#include <string>
#include <vector>
#include <thread>
// WinSock核心头文件

#include <winsock2.h>
#include <ws2tcpip.h>

// 链接WinSock库（必须）
#pragma comment(lib, "ws2_32.lib")

// 定义缓冲区大小
#define DEFAULT_BUFFER_SIZE 512

// message command definitions
#define BROADCAST_MSG "!broadcast"
#define PRIVATE_MSG "!private"
#define USER_LIST_MSG "!userlist"
#define ADD_USER_MSG "!adduser"
#define REMOVE_USER_MSG "!removeuser"
#define EXIT_MSG "!exit"
#define UNKNOWN_MSG "!unknown"




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

	void SendMessage(const std::string& message);

	void SendBroadcastMessage(std::string message);

	void ReceiveUserListMessage(std::vector<std::string>& user_list);

	void RemoveLineFeedFromTail(std::string& received_msg);

	void UpdateUserList(const std::string& user_list, std::vector<std::string>& user_list_vec);

	void StartReceiveThread(std::vector<std::string>& messages, std::vector<std::string>& user_list);

	void Command_RemoveUser(std::string& reminder, std::vector<std::string>& user_list);

	void Command_AddUser(std::string& reminder, std::vector<std::string>& user_list);

	// Unpack first command from message, return the command and the reminder message 
	static std::string UnpackFirstCommand(const std::string& message, std::string& OutReminder);
private:
	web_utils() = default;

	// server info
	const char* server_ip = "127.0.0.1";
	u_short server_port = 8080;
	std::string recv_cache; // 接收消息缓存，用于拆分粘包

	// WinSock variables
	WSADATA wsaData = {};
	SOCKET client_socket = INVALID_SOCKET;
	sockaddr_in server_address = {};
};

