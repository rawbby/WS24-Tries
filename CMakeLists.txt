cmake_minimum_required(VERSION 3.10)

project(Trie
        VERSION 1.0
        LANGUAGES CXX)

# download and setup cmake utilities

file(DOWNLOAD "https://raw.githubusercontent.com/rawbby/cmake_utilities/refs/heads/main/bootstrap.cmake"
        "${PROJECT_SOURCE_DIR}/cmake/bootstrap.cmake")

include(cmake/bootstrap.cmake)
include(cmake/all.cmake)

# configure project directories

add_libraries_directory(core)
add_tests_directory(test)
add_executables_directory(tool)
