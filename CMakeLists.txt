cmake_minimum_required(VERSION 3.16)
project(KhazanahIlmu)
set(CMAKE_CXX_STANDARD 17)

# Find MySQL Connector/C++
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQLCPP REQUIRED mysqlcppconn)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(kilms ${SOURCES})
target_link_libraries(kilms ${MYSQLCPP_LIBRARIES})
target_include_directories(kilms PRIVATE ${MYSQLCPP_INCLUDE_DIRS})
target_compile_options(kilms PRIVATE ${MYSQLCPP_CFLAGS_OTHER})
