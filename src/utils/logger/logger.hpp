#pragma once

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

#define SOCKET_CATEGORY "Socket"

namespace Logger
{
	void Error   (std::string category, std::string str);
	void Alert   (std::string category, std::string str);
	void Success (std::string category, std::string str);
}

#endif