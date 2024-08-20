add_executable(testAsyncLogging testAsyncLogging.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/logger/test)

target_link_libraries(testAsyncLogging my_muduo)

