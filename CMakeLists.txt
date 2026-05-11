cmake_minimum_required(VERSION 3.16)
project(media_watcher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra")

find_package(Threads REQUIRED)

set(LIB_SOURCES
        src/scanner.cpp
        src/json_builder.cpp
        src/http_server.cpp
)

add_library(media_watcher_lib STATIC ${LIB_SOURCES})
target_include_directories(media_watcher_lib PUBLIC src)
target_link_libraries(media_watcher_lib PUBLIC Threads::Threads)

add_executable(media_watcher src/main.cpp)
target_link_libraries(media_watcher PRIVATE media_watcher_lib)

option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
    )

    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(media_watcher_tests
            tests/test_main.cpp
            tests/test_scanner.cpp
            tests/test_json_builder.cpp
            tests/test_http_server.cpp
    )

    target_link_libraries(media_watcher_tests
            PRIVATE
            media_watcher_lib
            GTest::gtest
    )

    include(GoogleTest)
    gtest_discover_tests(media_watcher_tests)
endif()

install(TARGETS media_watcher DESTINATION bin)
