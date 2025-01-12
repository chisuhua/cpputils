cmake_minimum_required(VERSION 3.10)

project(SpscQueueTest)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")

    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -static-libc++")
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -static-libc++")
    #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -static-libc++")
endif()

if(APPLE)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++")
    #find_package(LibCXX REQUIRED)
    #include_directories(${LIBCXX_INCLUDE_DIRS})
    #link_directories(${LIBCXX_LIBRARY_DIRS})
    #link_libraries(c++)
endif()

# 添加源文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/doctest/doctest)

# 添加 doctest 库
add_subdirectory(external/doctest)

# 添加测试源文件
add_executable(test_spsc_queue test/test_spsc_queue.cpp)
target_link_libraries(test_spsc_queue doctest::doctest)

#add_library(spmc_queue spmc_queue.h)
add_executable(test_spmc_queue test/test_spmc_queue.cpp)
target_link_libraries(test_spmc_queue doctest::doctest)

# 添加可执行文件
#add_executable(test_string_intern include/String.h)
#target_link_libraries(test_string_intern doctest::doctest)
add_executable(test_string_intern test_string_intern.cpp)
target_link_libraries(test_string_intern doctest::doctest)

# 设置测试目标
enable_testing()
add_test(NAME test_spsc_queue COMMAND test_spsc_queue)
add_test(NAME test_spmc_queue COMMAND test_spmc_queue)
add_test(NAME run_tests COMMAND test_string_intern)
