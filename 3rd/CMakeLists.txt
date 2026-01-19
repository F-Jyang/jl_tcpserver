set(SPDLOG_BUILD_SHARED ON)
add_subdirectory("./spdlog")
# if(MSVC)
#     set_property(TARGET spdlog PROPERTY FOLDER "jl_tcpserver/3rd")
# endif(MSVC)

