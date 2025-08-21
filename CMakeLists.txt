cmake_minimum_required(VERSION 3.15)
project(biggie-backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR}/libs/flatbuffers/include)

add_executable(biggie-backend
        src/main.cpp
        src/socket/server.cpp
	src/utils/logger/logger.cpp
	src/packets/handler/handler.cpp
)

target_link_libraries(biggie-backend ws2_32)