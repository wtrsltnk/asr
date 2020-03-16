cmake_minimum_required(VERSION 3.0)

project(asr)

add_subdirectory(fmt)

add_library(system.net
    system.net/src/http/httplistener.cpp
    system.net/include/http/httplistener.h
    system.net/src/http/httplistenercontext.cpp
    system.net/include/http/httplistenercontext.h
    system.net/src/http/httplistenerresponse.cpp
    system.net/include/http/httplistenerresponse.h
    system.net/src/http/httplistenerrequest.cpp
    system.net/include/http/httplistenerrequest.h
    system.net/src/http/httplistenerexception.cpp
    system.net/include/http/httplistenerexception.h
    )

target_compile_features(system.net
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    PRIVATE cxx_thread_local
    )

target_include_directories(system.net
    PUBLIC system.net/include
    )

add_executable(asr
    program.cpp
    sqlite3/sqlite3.c
    README.md
    )

target_compile_features(asr
    PUBLIC cxx_std_17)

target_link_libraries(asr
    system.net
    fmt
    ws2_32
    )

target_include_directories(asr
    PUBLIC system.net/include
    )
