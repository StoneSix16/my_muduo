add_executable(testThreadPool testThreadPool.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/base/test)

target_link_libraries(testThreadPool my_muduo)