cmake_minimum_required(VERSION 3.18)

project(shared_mutex VERSION 1.0.0 LANGUAGES C)

if(NOT CMAKE_C_FLAGS)
	set(CMAKE_C_FLAGS "-O3")
endif()
if(NOT CMAKE_C_FLAGS_DEBUG)
	set(CMAKE_C_FLAGS_DEBUG "-g -ggdb")
endif()

add_library(shared_mutex)
#target_include_directories(shared_mutex PUBLIC .)
target_include_directories(shared_mutex PUBLIC "${PROJECT_SOURCE_DIR}")
set_target_properties(shared_mutex PROPERTIES PUBLIC_HEADER "shared_mutex.h;shared_mutex.hpp")
target_link_libraries(shared_mutex PUBLIC rt)

target_sources(shared_mutex
	PRIVATE shared_mutex.c
	PUBLIC FILE_SET HEADERS
	BASE_DIRS ${PROJECT_SOURCE_DIR}
	FILES shared_mutex.h shared_mutex.hpp)

install(TARGETS shared_mutex FILE_SET HEADERS)
