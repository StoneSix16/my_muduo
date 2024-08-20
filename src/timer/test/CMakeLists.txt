add_executable(testTimerQueue testTimerQueue.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/timer/test)

target_link_libraries(testTimerQueue my_muduo)

