cmake_minimum_required(VERSION 3.6)

add_executable( logreader main.cpp )
target_link_libraries( logreader PUBLIC reader )

source_group( \\ FILES main.cpp )

