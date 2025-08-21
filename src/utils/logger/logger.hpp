#pragma once

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

#define SOCKET_CATEGORY "Socket"

namespace Logger
{
	void Error   (const std::string& category, const std::string& str);
	void Alert   (const std::string& category, const std::string& str);
	void Success (const std::string& category, const std::string& str);
}

#endif