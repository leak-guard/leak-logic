cmake_minimum_required(VERSION 3.29)
project(leak_logic)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(leak_logic INTERFACE)
target_include_directories(leak_logic INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(ENABLE_TESTS "Enable tests" ON)

if(ENABLE_TESTS)
    add_subdirectory(googletest)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    add_executable(leak_logic_test test/test_driver.cpp)

    target_link_libraries(leak_logic_test PRIVATE leak_logic gtest gtest_main)
endif()
