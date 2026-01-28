#include "web_utils.h"
#include <thread>

web_utils* web_utils::SingleInstance = nullptr;

bool web_utils::InitWebSock()
{
	server_ip = "127.0.0.1";
	server_port = static_cast<u_short>(atoi("8080"));

	// 2. 初始化WinSock库
	
	int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsa_result != 0) {
		std::cerr << "WSAStartup失败，错误码: " << wsa_result << std::endl;
		return false;
	}

	// 3. 创建TCP套接字
	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_socket == INVALID_SOCKET) {
		std::cerr << "创建套接字失败，错误码: " << WSAGetLastError() << std::endl;
		WSACleanup(); // 失败时清理WinSock资源
		return false;
	}

	// 4. 配置服务器地址结构
	server_address = {};
	server_address.sin_family = AF_INET;                // IPv4地址族
	server_address.sin_port = htons(server_port);       // 端口（转换为网络字节序）

	// 将字符串IP转换为二进制格式
	if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
		std::cerr << "无效的IP地址或地址不被支持" << std::endl;
		closesocket(client_socket);
		WSACleanup();
		return false;
	}
	std::cout << "WinSock初始化成功，准备连接到服务器 " << server_ip << ":" << server_port << std::endl;
	return true;
}

bool web_utils::ConnectToServer()
{
	// check socket validity
	if (client_socket == INVALID_SOCKET) {
		std::cerr << "套接字无效，无法连接到服务器" << std::endl;
		return false;
	}
	// connect to server
	if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == SOCKET_ERROR) {
		std::cerr << "连接服务器失败，错误码: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		WSACleanup();
		return false;
	}
	std::cout << "成功连接到服务器: " << server_ip << ":" << server_port << std::endl;
	return true;
}

void web_utils::CleanupWebSock()
{
	if (client_socket != INVALID_SOCKET)
	{
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}
	WSACleanup();               
}

void web_utils::SendMessage(const std::string& message)
{
	if (client_socket == INVALID_SOCKET) {
		std::cerr << "套接字无效，无法发送消息" << std::endl;
		return;
	}
	int send_result = send(client_socket, message.c_str(), static_cast<int>(message.size()), 0);
	if (send_result == SOCKET_ERROR) {
		std::cerr << "发送消息失败，错误码: " << WSAGetLastError() << std::endl;
	} else {
		std::cout << "成功发送消息: " << message << std::endl;
	}
}

void web_utils::SendBroadcastMessage(std::string message)
{
	if (client_socket == INVALID_SOCKET) {
		std::cerr << "套接字无效，无法发送消息" << std::endl;
		return;
	}
	message = std::string("!broadcast ") + message; // Prefix to indicate broadcast message
	
	int send_result = send(client_socket, message.c_str(), static_cast<int>(message.size()), 0);
	if (send_result == SOCKET_ERROR) {
		std::cerr << "发送消息失败，错误码: " << WSAGetLastError() << std::endl;
	} else {
		std::cout << "成功发送消息: " << message << std::endl;
	}
}

void web_utils::StartReceiveThread(std::vector<std::string>& messages)
{
	// Implementation of receiving thread can be added here
	std::thread receive_thread([this, &messages]() {
		char buffer[DEFAULT_BUFFER_SIZE];
		while (true) {
			int bytes_received = recv(client_socket, buffer, DEFAULT_BUFFER_SIZE - 1, 0);
			if (bytes_received > 0) {
				buffer[bytes_received] = '\0'; // Null-terminate the received data
				std::cout << "Received: " << buffer << std::endl;
				messages.push_back(std::string(buffer));
			} else if (bytes_received == 0) {
				std::cout << "Connection closed by server." << std::endl;
				break;
			} else {
				std::cerr << "Receive failed, error code: " << WSAGetLastError() << std::endl;
				break;
			}
		}
		});
	receive_thread.detach(); 
}
