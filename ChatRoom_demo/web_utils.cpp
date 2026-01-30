#include "web_utils.h"
#include <thread>
#include "PrivateChatStructure.h"
#include <map>

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
	message = std::string(BROADCAST_MSG)+ " " + message; // Prefix to indicate broadcast message
	
	int send_result = send(client_socket, message.c_str(), static_cast<int>(message.size()), 0);
	if (send_result == SOCKET_ERROR) {
		std::cerr << "发送消息失败，错误码: " << WSAGetLastError() << std::endl;
	} else {
		std::cout << "成功发送消息: " << message << std::endl;
	}
}

void web_utils::ReceiveUserListMessage(std::vector<std::string>& user_list)
{
	char buffer[DEFAULT_BUFFER_SIZE];
	int bytes_received = recv(client_socket, buffer, DEFAULT_BUFFER_SIZE - 1, 0);
	if (bytes_received > 0) {
		buffer[bytes_received] = '\0'; // Null-terminate the received data
		std::string received_msg = std::string(buffer);

		RemoveLineFeedFromTail(received_msg);

		std::string reminder;
		std::string command = UnpackFirstCommand(received_msg, reminder);
		if (command == USER_LIST_MSG)
		{
			UpdateUserList(reminder, user_list);
		}
		else
		{
			std::cerr << "Received unexpected message when expecting user list: " << buffer << std::endl;
		}
	} else if (bytes_received == 0) {
		std::cout << "Connection closed by server." << std::endl;
	} else {
		std::cerr << "Receive failed, error code: " << WSAGetLastError() << std::endl;
	}
	recv_cache.clear();
}

void web_utils::CombinePrivateMessage(const std::string& to_user, const std::string& message, std::string& out_message)
{
	out_message = std::string(PRIVATE_MSG) + " " + to_user + " " + message;
	out_message += "\n"; // Append line feed for message termination
}



void web_utils::UpdateUserList(const std::string& user_list, std::vector<std::string>& user_list_vec)
{
	user_list_vec.clear();
	size_t start = 0;
	size_t end = user_list.find(',');
	while (end != std::string::npos) {
		std::string user = user_list.substr(start, end - start);
		// 只添加非空的用户名称
		if (!user.empty()) {
			user_list_vec.push_back(user);
		}
		start = end + 1;
		end = user_list.find(',', start);
	}
	// Add the last user (or the only user if no commas)
	if (start < user_list.size()) {
		std::string user = user_list.substr(start);
		if (!user.empty()) {
			user_list_vec.push_back(user);
		}
	}
}

void web_utils::StartReceiveThread(std::vector<std::string>& messages, std::vector<std::string>& user_list, std::map<std::string, PrivateChatData>& private_chat_map)
{
	// Implementation of receiving thread can be added here
	std::thread receive_thread([this, &messages, &user_list, &private_chat_map]() {
		char buffer[DEFAULT_BUFFER_SIZE];
		while (true) {
			int bytes_received = recv(client_socket, buffer, DEFAULT_BUFFER_SIZE - 1, 0);
			recv_cache.clear();
			buffer[bytes_received] = '\0'; // Null-terminate the received data
			if (bytes_received > 0) {
				// 1. 将接收的数据追加到缓存
				recv_cache += std::string(buffer);

				// 2. 按\n拆分完整消息，处理粘包
				size_t newline_pos;
				while ((newline_pos = recv_cache.find('\n')) != std::string::npos) {
					// 提取完整消息（去掉\n）
					std::string complete_msg = recv_cache.substr(0, newline_pos);
					RemoveLineFeedFromTail(complete_msg);
					// 移除已处理的消息，更新缓存
					recv_cache = recv_cache.substr(newline_pos + 1);

					// 跳过空的完整消息
					if (complete_msg.empty()) {
						continue;
					}

					// 3. 解析完整消息（原有逻辑）
					std::cout << "Received complete message: " << complete_msg<<"// " << std::endl;
					std::string reminder;
					std::string command = UnpackFirstCommand(complete_msg, reminder);

					// -- deal with different commands -- //

					if (command == BROADCAST_MSG) {
						messages.push_back(reminder);
					}
					else if (command == ADD_USER_MSG) {
						Command_AddUser(reminder, user_list);

					}
					else if (command == REMOVE_USER_MSG)
					{
						Command_RemoveUser(reminder, user_list);

					}
					else if (command == PRIVATE_MSG)
					{
						Command_AddPrivateMessage(reminder, private_chat_map);
					}
					else if (command == UNKNOWN_MSG) {
						continue;
					}
				}
			} else if (bytes_received == 0) {
				std::cout << "Connection closed by server." << std::endl;
				break;
			} else {
				std::cerr << "Receive failed, error code: " << WSAGetLastError() << std::endl;
				break;
			}
			recv_cache.clear();
		}
		});
	receive_thread.detach(); 
}

void web_utils::Command_RemoveUser(std::string& reminder, std::vector<std::string>& user_list)
{
	// 过滤空的用户名称
	if (!reminder.empty()) {
		std::cout << "User left: " << reminder << std::endl;
		auto it = std::find(user_list.begin(), user_list.end(), reminder);
		if (it != user_list.end()) {
			user_list.erase(it);
		}
	}
}

void web_utils::Command_AddUser(std::string& reminder, std::vector<std::string>& user_list)
{
	// 过滤空的用户名称
	if (!reminder.empty()) {
		std::cout << "New user joined: " << reminder << std::endl;
		user_list.push_back(reminder);
	}
}

void web_utils::Command_AddPrivateMessage(std::string& reminder, std::map<std::string, PrivateChatData>& private_chat_map)
{
	// Here we can process private messages if needed
	std::cout << "Received private message: " << reminder << std::endl;
	RemoveLineFeedFromTail(reminder);
	std::string from_user;
	std::string private_msg;
	SplitStringAtFirstSpace(reminder, from_user, private_msg);
	std::cout << "From: " << from_user << ", Message: " << private_msg << std::endl;
	private_chat_map[from_user].msgs.push_back(from_user + ": " + private_msg);
}

void web_utils::RemoveLineFeedFromTail(std::string& received_msg)
{
	if (!received_msg.empty() && received_msg.back() == '\n') {
		received_msg = received_msg.substr(0, received_msg.size() - 1);
	}
}

void web_utils::AddLineFeedToTail(std::string& message)
{
	if (message.empty() || message.back() != '\n') {
		message += '\n';
	}
}

std::string web_utils::UnpackFirstCommand(const std::string& message, std::string& OutReminder)
{
	// 检查字符串开头是不是命令标志'!'
	if (message.empty() || message[0] != '!') {
		OutReminder = message;
		return UNKNOWN_MSG;
	}
	size_t space_pos = message.find(' ');
	if (space_pos == std::string::npos) {
		OutReminder = "";
		return message; // 整个消息就是命令
	} else {
		OutReminder = message.substr(space_pos + 1);
		return message.substr(0, space_pos);
	}
}

void web_utils::SplitStringAtFirstSpace(const std::string& input_str, std::string& before_space, std::string& after_space)
{
	size_t first_space_pos = input_str.find(' ');
	if (first_space_pos == std::string::npos) {
		// No space found
		before_space = input_str;
		after_space = "";
	}
	else {
		before_space = input_str.substr(0, first_space_pos);
		after_space = input_str.substr(first_space_pos + 1); // +1 to skip the space
	}
}

std::string web_utils::GetStrBeforeFirstSymbol(const std::string& input_str, char symbol)
{
	// 1. 查找第一个空格的位置（find返回size_t类型，未找到返回string::npos）
	size_t first_space_pos = input_str.find(symbol);

	// 2. 处理不同情况
	if (first_space_pos == std::string::npos) {
		// 情况1：字符串中无空格，返回原字符串
		return input_str;
	}
	else if (first_space_pos == 0) {
		// 情况2：开头就是空格，返回空字符串（可根据需求调整，比如返回原串或报错）
		return "";
	}
	else {
		// 情况3：正常有空格，截取0到第一个空格位置的子串
		return input_str.substr(0, first_space_pos);
	}
}
