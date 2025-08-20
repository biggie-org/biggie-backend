#pragma once

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

namespace Logger
{
	void Error   (std::string category, std::string str);
	void Alert   (std::string category, std::string str);
	void Success (std::string category, std::string str);
}

#endif