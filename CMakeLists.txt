cmake_minimum_required(VERSION 3.10)

project(Trie
        VERSION 1.0
        LANGUAGES CXX)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/bootstrap.cmake")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/rawbby/cmake_utilities/refs/tags/v2.0.1/bootstrap.cmake"
            "${CMAKE_BINARY_DIR}/bootstrap.cmake")
endif ()

set(CMAKE_UTILITIES_VERSION "v2.0.1")
include("${CMAKE_BINARY_DIR}/bootstrap.cmake")

include(.cmake_utilities/all.cmake)
