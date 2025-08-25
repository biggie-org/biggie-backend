cmake_minimum_required(VERSION 3.15)
project(biggie-backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(VCPKG_ROOT "C:/dev/vcpkg")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /GL /DNDEBUG /arch:AVX2 /fp:fast /GS- /favor:INTEL64 /MT")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

include_directories(${CMAKE_SOURCE_DIR}/libs/flatbuffers/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/asio/include/)


# libuv
find_library(LIBUV_LIB NAMES uv PATHS "${VCPKG_ROOT}/installed/x64-windows/lib")
find_path(LIBUV_INCLUDE_DIR uv.h PATHS "${VCPKG_ROOT}/installed/x64-windows/include")
include_directories(${LIBUV_INCLUDE_DIR})

# hiredis
find_package(hiredis CONFIG REQUIRED)

# boost (asio é header-only, precisa só do system pra funcionar)
find_package(Boost REQUIRED COMPONENTS asio system)

add_executable(biggie-backend
    src/main.cpp
    src/socket/server.cpp
    src/utils/logger/logger.cpp
    src/packets/handler/handler.cpp
)

target_link_libraries(biggie-backend
    PRIVATE Boost::system
            hiredis::hiredis
            ${LIBUV_LIB}
            ws2_32
)
