cmake_minimum_required(VERSION 3.2)

option(HTTP_EXAMPLE "Build example programs" OFF)

project(system.net)

add_library(system.net
    src/http/httplistener.cpp
    include/http/httplistener.h
    src/http/httplistenercontext.cpp
    include/http/httplistenercontext.h
    src/http/httplistenerresponse.cpp
    include/http/httplistenerresponse.h
    src/http/httplistenerrequest.cpp
    include/http/httplistenerrequest.h
    src/http/httplistenerexception.cpp
    include/http/httplistenerexception.h
    )

target_compile_features(system.net
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    PRIVATE cxx_thread_local
    )

target_include_directories(system.net
    PUBLIC include
    )

if (HTTP_EXAMPLE)
    add_executable(system.net.example1
        examples/example1.cpp
        )

    target_compile_features(system.net.example1
        PRIVATE cxx_auto_type
        PRIVATE cxx_nullptr
        PRIVATE cxx_range_for
        PRIVATE cxx_thread_local
        )

    target_include_directories(system.net.example1
        PRIVATE include
        )

    target_link_libraries(system.net.example1
        system.net ws2_32
        )
endif()
