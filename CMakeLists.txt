cmake_minimum_required(VERSION 3.0.0)
project(tetris-cle VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(tetris-cle src/main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
