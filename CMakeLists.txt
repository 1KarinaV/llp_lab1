cmake_minimum_required(VERSION 3.23)
project(llp_lab1 C)

set(CMAKE_C_STANDARD 99)

add_executable(llp_lab1 main.c store/graph_struct.h datafile/datafile.h store/cells.h datafile/node.h datafile/node.c datafile/relation.h datafile/relation.c)
