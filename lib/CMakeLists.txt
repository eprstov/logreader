cmake_minimum_required(VERSION 3.6)

#set( CMAKE_CXX_STANDARD 20 )
#set( CMAKE_CXX_STANDARD_REQUIRED ON )
#set( CMAKE_CXX_EXTENSIONS OFF )

file( GLOB SOURCES LIST_DIRECTORIES false *.h *.cpp )

add_library( reader ${SOURCES} )
target_include_directories( reader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
source_group( \\ FILES ${SOURCES} )

