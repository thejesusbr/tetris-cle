cmake_minimum_required(VERSION 3.0.0)
project(tetris-cle 
  VERSION 0.1.0
  DESCRIPTION "Play Tetris in your terminal!"
  LANGUAGES CXX)

include(CTest)
enable_testing()

add_executable(tetris-cle src/main.cpp)
find_library(LIBRARY_NCURSESW ncursesw)
if(LIBRARY_NCURSESW)
  target_link_libraries(tetris-cle ${LIBRARY_NCURSESW})
else()
  MESSAGE( FATAL "This program depends on ncursesw." )
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
