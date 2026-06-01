cmake_minimum_required(VERSION 3.16)
project(KhazanahIlmu)
set(CMAKE_CXX_STANDARD 17)

# Homebrew paths (Apple Silicon = /opt/homebrew, Intel = /usr/local)
set(HOMEBREW_PATHS /opt/homebrew /usr/local)

find_library(MYSQLCPP_LIB
    NAMES mysqlcppconnx
    PATHS ${HOMEBREW_PATHS}
    PATH_SUFFIXES lib
    REQUIRED
)

find_path(MYSQLCPP_INCLUDE
    NAMES mysqlx/xdevapi.h
    PATHS ${HOMEBREW_PATHS}
    PATH_SUFFIXES include
    REQUIRED
)

message(STATUS "MySQL Connector lib:     ${MYSQLCPP_LIB}")
message(STATUS "MySQL Connector include: ${MYSQLCPP_INCLUDE}")

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(kilms ${SOURCES})
target_link_libraries(kilms ${MYSQLCPP_LIB})
target_include_directories(kilms PRIVATE ${MYSQLCPP_INCLUDE})
