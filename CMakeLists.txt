cmake_minimum_required(VERSION 3.15)
project(biggie-backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(biggie-backend
        src/main.cpp
        src/socket/server.cpp
	src/utils/logger/logger.cpp
)

target_link_libraries(biggie-backend ws2_32)