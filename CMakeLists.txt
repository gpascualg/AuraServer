cmake_minimum_required(VERSION 2.8)

# -[ DBOSS Project
project(DBOSS)

# -[ Macros
include(cmake/Macros.cmake)

# -[ C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# -[ Subdirectories
add_subdirectory(src)
