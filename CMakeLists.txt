
# 设置最低 CMake 版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和语言
project(MyCProject C)

# 自动递归查找所有 C 源文件（支持动态更新）
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c)

# 指定头文件目录
include_directories(include)

# 添加可执行文件
add_executable(my_program ${SOURCES})

# 查找线程库
find_package(Threads REQUIRED)

# 链接线程库
target_link_libraries(my_program PRIVATE Threads::Threads)

set(CMAKE_BUILD_TYPE Debug)

# 如果需要，可以设置编译选项
target_compile_options(my_program PRIVATE -Wall -g)
