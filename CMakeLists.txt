# 指定使用 CMake 的最低版本号
cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(my_muduo C CXX)

# 源文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src/base SRC_BASE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/logger SRC_LOG)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/event SRC_EVENT)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/event/poller SRC_POLLER)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/timer SRC_TIMER)

# 设置头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    )

# 设置编译选项
set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
    )

# 生成动态库 
add_library(my_muduo SHARED 
        ${SRC_BASE}
        ${SRC_LOG}
        ${SRC_NET}
        ${SRC_EVENT}
        ${SRC_TIMER}
        ${SRC_POLLER}
        )

# 目标动态库所需连接的库（这里需要连接libpthread.so）
target_link_libraries(my_muduo pthread)

# 设置生成动态库的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置安装的默认路径
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

# 将目录安装到 include/tiny_network 下（只安装头文件）
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ DESTINATION include/my_muduo
    FILES_MATCHING PATTERN "*.h")

# 加载test
add_subdirectory(src/base/test)
add_subdirectory(src/logger/test)
add_subdirectory(src/timer/test)

# 加载example
add_subdirectory(example/echo)
