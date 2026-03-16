# Set the minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

project(Notepad VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Notepad
    main.cpp 
    editor.cpp
)