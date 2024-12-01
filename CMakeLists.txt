
# 设置最低 CMake 版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和语言
project(MyCProject C)

# 指定源文件
set(SOURCES
    src/main.c
    src/math/add.c
    src/math/subtract.c
    src/math/multiply.c  # 添加新的源文件
    src/utils/utils.c
    src/vector/vector.c  # 添加新的源文件
    src/list/list.c  # 添加新的源文件
    src/list/skiplist.c  # 添加新的源文件
    src/list/Stack.c  # 添加新的源文件
    src/test_leetecode/test_leetecode.c
    src/threadpool/threadpool.c
    src/mempool/mempool.c
    src/cjson/cJSON.c
    src/mapset/mapset.c
    src/mapset/hashmap.c
    src/mapset/multimap.c
    src/ValueRange/ValueRange.c
    src/common.c  # 添加新的源文件
)

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