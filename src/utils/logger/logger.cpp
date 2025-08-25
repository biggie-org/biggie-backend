#include <iostream>
#include <sstream>
#include <ctime>
#include <mutex>

namespace Logger
{
	std::mutex logger_mutex;
	
	std::tm GetNow()
	{
		const std::time_t time = std::time(nullptr);
		std::tm now{};

		localtime_s(&now, &time);

		return now;
	}

	std::string GetTimeFormatted(const std::tm* now, std::stringstream& stream)
	{
		stream.str("");
		stream.clear();

		const char* padding_h = (now->tm_hour < 10 ? "0" : "");
		const char* padding_m = (now->tm_min < 10 ? "0" : "");

		stream << padding_h << now->tm_hour << ":" << padding_m << (now->tm_min + 1);

		return stream.str();
	}

	std::string GetDateFormatted(const std::tm* now, std::stringstream& stream)
	{
		stream.str("");
		stream.clear();

		const char* padding_d = (now->tm_mday < 10 ? "0" : "");
		const char* padding_m = (now->tm_mon < 10 ? "0" : "");

		stream << padding_d << now->tm_mday << "/" << padding_m << (now->tm_mon + 1);

		return stream.str();
	}
	
	void Error(const std::string& category, const std::string& str)
	{
		std::lock_guard<std::mutex> lock(logger_mutex);
		
		std::stringstream stream;
		const std::tm now = GetNow();

		std::cout << "[" << GetDateFormatted(&now, stream) << "] " << "[" << GetTimeFormatted(&now, stream) << "] [" << category << "] [ERROR] " << str << "\n";
	}

	void Alert(const std::string& category, const std::string& str)
	{
		std::lock_guard<std::mutex> lock(logger_mutex);
		
		std::stringstream stream;
		std::tm now = GetNow();

		std::cout << "[" << GetDateFormatted(&now, stream) << "] " << "[" << GetTimeFormatted(&now, stream) << "] [" << category << "] [ALERT] " << str << "\n";
	}

	void Success(const std::string& category, const std::string& str)
	{
		std::lock_guard<std::mutex> lock(logger_mutex);
		
		std::stringstream stream;
		const std::tm now = GetNow();

		std::cout << "[" << GetDateFormatted(&now, stream) << "] " << "[" << GetTimeFormatted(&now, stream) << "] [" << category << "] [SUCCESS] " << str << "\n";
	}
}