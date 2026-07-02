cmake_minimum_required(VERSION 3.20)

project(microconf_find_package_consumer LANGUAGES C)

find_package(microconf CONFIG REQUIRED)

add_executable(microconf_find_package_consumer main.c)
target_link_libraries(microconf_find_package_consumer PRIVATE microconf::microconf)
