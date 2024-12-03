cmake_minimum_required(VERSION 3.29)
project(leak_logic)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(leak_logic INTERFACE)
target_include_directories(leak_logic INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (NOT TARGET static_collections)
    add_subdirectory(external/static-collections)
endif()

include_directories(external/static-collections/include)

option(ENABLE_TESTS "Enable tests" ON)

if(ENABLE_TESTS)
    add_subdirectory(external/googletest)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test)
    add_executable(leak_logic_test test/test_driver.cpp)

    target_link_libraries(leak_logic_test PRIVATE leak_logic gtest gtest_main)
endif()
