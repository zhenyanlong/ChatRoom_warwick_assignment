#pragma once
#include <string>
#include <vector>

//private chat data structure
struct PrivateChatData {
	bool is_show = false;          // window visibility
	std::vector<std::string> msgs; // private message list, message type: "username: message"
	char input_buf[256] = { 0 };   // input buffer
};