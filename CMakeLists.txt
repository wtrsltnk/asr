cmake_minimum_required(VERSION 3.0)

include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#7.1.3")
CPMAddPackage("gh:nlohmann/json@3.10.5")

include(cmake/htdocs.cmake)

project(asr VERSION 1.0.0)

configure_file(src/config.h.in config.h)

add_htdocs_file("htdocs/css/styles.css" HTDOCS_STYLES)
add_htdocs_file("htdocs/js/scripts.js" HTDOCS_SCRIPTS)
add_htdocs_file("htdocs/aggregate-root.html" HTDOCS_AGGREGATEROOTHTML)
add_htdocs_file("htdocs/postmodal.html" HTDOCS_POSTMODALHTML)

configure_file(src/htdocs.h.in htdocs.h)

add_library(system.net
    thirdparty/system.net/src/http/httplistener.cpp
    thirdparty/system.net/include/http/httplistener.h
    thirdparty/system.net/src/http/httplistenercontext.cpp
    thirdparty/system.net/include/http/httplistenercontext.h
    thirdparty/system.net/src/http/httplistenerresponse.cpp
    thirdparty/system.net/include/http/httplistenerresponse.h
    thirdparty/system.net/src/http/httplistenerrequest.cpp
    thirdparty/system.net/include/http/httplistenerrequest.h
    thirdparty/system.net/src/http/httplistenerexception.cpp
    thirdparty/system.net/include/http/httplistenerexception.h
)

target_compile_features(system.net
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    PRIVATE cxx_thread_local
)

target_include_directories(system.net
    PUBLIC
        "thirdparty/system.net/include"
)

add_executable(asr
    htdocs/css/styles.css
    htdocs/js/scripts.js
    htdocs/aggregate-root.html
    htdocs/postmodal.html
    src/program.cpp
    src/common/templateutils.cpp
    src/common/templateutils.h
    src/common/instrumentationtimer.cpp
    src/common/instrumentationtimer.h
    thirdparty/sqlite3/sqlite3.c
    README.md
    "${PROJECT_BINARY_DIR}/htdocs.h"
)

target_compile_features(asr
    PUBLIC cxx_std_17
)

target_link_libraries(asr
    system.net
    fmt
    nlohmann_json::nlohmann_json
    ws2_32
)

target_include_directories(asr
    PUBLIC
        "thirdparty/"
        "${PROJECT_BINARY_DIR}"
        )

CPMAddPackage("gh:catchorg/Catch2@2.13.8")

add_executable(asr_tests
    tests/tests-bootstrap.cpp
    tests/templateutils_tests.cpp
    src/common/templateutils.cpp
    src/common/templateutils.h
)

target_link_libraries(asr_tests
    Catch2
    fmt
)

target_compile_features(asr_tests
    PUBLIC cxx_std_14
)
