cmake_minimum_required(VERSION 3.16)

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(src ${SRCS})
    get_filename_component(name ${src} NAME_WE)
    message(STATUS "build executable ${name}.")

    add_executable(${name} ${src})

    # target_include_directories(jl_tcpserver_)
    if(BUILD_JL_TCPSERVER_AS_SHARED)
        target_link_libraries(${name} PRIVATE
            jl_tcpserver::jl_tcpserver_shared
        )
    else()
        target_link_libraries(${name} PRIVATE
            jl_tcpserver::jl_tcpserver_static
        )
    endif()
endforeach()
