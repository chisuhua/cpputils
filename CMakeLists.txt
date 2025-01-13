cmake_minimum_required(VERSION 3.10)

# 项目名称
project(cpputils)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 doctest 头文件
find_path(DOCTEST_INCLUDE_DIR NAMES doctest/doctest.h
    PATHS ${CMAKE_SOURCE_DIR}/doctest
    NO_DEFAULT_PATH
)

if(NOT DOCTEST_INCLUDE_DIR)
    message(FATAL_ERROR "doctest include directory not found")
endif()

# 包含 doctest 头文件路径
include_directories(${DOCTEST_INCLUDE_DIR})

# 添加源文件
set(SOURCES
    main.cpp
    Queue.h
    String.h
)

# 创建可执行文件
add_executable(cpputils ${SOURCES})

# 运行测试
enable_testing()

# 添加测试
add_test(NAME cpputils_tests COMMAND cpputils --doctest)

