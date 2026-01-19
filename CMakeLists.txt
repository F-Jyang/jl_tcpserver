cmake_minimum_required(VERSION 3.10.0)
project(jl_tcpserver VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "build test" ON)
option(BUILD_JL_TCPSERVER_AS_SHARED "build shared library" ON)
option(ENABLE_OPENSSL "enable ssl connction" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

file(GLOB SRC_FILES main.cpp src/*.cpp src/*.hpp src/*.h)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rd")

if(ENABLE_OPENSSL)
    add_compile_definitions(ENABLE_OPENSSL)
    message(STATUS "Enable Openssl")
    find_package(OpenSSL REQUIRED)
endif()

if(MSVC)
    add_compile_options("/utf-8")
    add_compile_definitions(-D _WIN32_WINDOWS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) # 编译dll库需要导出所有符号
    # target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus) # msvc不会自动设置cpp版本宏
    # target_link_options(easy_tools PRIVATE $<IF:$<CONFIG:Debug>,/SUBSYSTEM:CONSOLE,/SUBSYSTEM:WINDOWS> /ENTRY:mainCRTStartup) # 设置debug、release下不同的入口
endif()

set(LibraryName ${PROJECT_NAME})

# 是否创建动态库
if(BUILD_JL_TCPSERVER_AS_SHARED)
    set(LibraryName ${PROJECT_NAME}_shared)
    message(STATUS "Build jl_tcpserver as shared library.")
    add_library(${LibraryName} SHARED ${SRC_FILES})
else()
    message(STATUS "Build jl_tcpserver as static library.")
    set(LibraryName ${PROJECT_NAME}_static)
    add_library(${LibraryName} STATIC ${SRC_FILES})
endif()

target_include_directories(${LibraryName} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/asio/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${LibraryName} PUBLIC
    spdlog::spdlog
)
if(ENABLE_OPENSSL)
    target_include_directories(${LibraryName} PUBLIC
        ${OPENSSL_INCLUDE_DIR}
    )
    target_link_libraries(${LibraryName} PUBLIC
        spdlog::spdlog
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

add_library(${PROJECT_NAME}::${LibraryName} ALIAS ${LibraryName}) # 创建别名

if(BUILD_TESTS)
    add_subdirectory("./test")
endif()

