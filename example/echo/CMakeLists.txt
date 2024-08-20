add_executable(echo echo.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example/echo)

target_link_libraries(echo my_muduo)