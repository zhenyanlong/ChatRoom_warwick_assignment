#include "web_utils.h"

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
