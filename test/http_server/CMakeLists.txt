cmake_minimum_required(VERSION 3.16)

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

message(STATUS "build executable http_server.")

add_executable(http_server ${SRCS})

target_include_directories(http_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(BUILD_JL_TCPSERVER_AS_SHARED)
    target_link_libraries(http_server PRIVATE
        jl_tcpserver::jl_tcpserver_shared
    )
else()
    target_link_libraries(http_server PRIVATE
        jl_tcpserver::jl_tcpserver_static
    )
endif()
