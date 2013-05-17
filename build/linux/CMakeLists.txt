cmake_minimum_required(VERSION 2.6)

project(ise CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CXX_FLAGS
 -g
 -Wall
 -Woverloaded-virtual
 -march=native
 -rdynamic
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_SOURCE_DIR})
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(ISE_ROOT_PATH ${CMAKE_SOURCE_DIR}/../..)

find_package(Boost REQUIRED)
message(STATUS "Boost_INCLUDE_DIRS = " ${Boost_INCLUDE_DIRS})

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ISE_ROOT_PATH})

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

aux_source_directory(${ISE_ROOT_PATH}/ise/main ISE_MAIN_SRCS)
add_library(ise ${ISE_MAIN_SRCS})
target_link_libraries(ise pthread)

add_subdirectory(${ISE_ROOT_PATH}/ise/ext/dbi/mysql ${PROJECT_BINARY_DIR}/ext/dbi/mysql)
add_subdirectory(${ISE_ROOT_PATH}/examples ${PROJECT_BINARY_DIR}/examples)

install(TARGETS ise DESTINATION lib)

file(GLOB ISE_MAIN_HEADERS ${ISE_ROOT_PATH}/ise/main/*.h)
install(FILES ${ISE_MAIN_HEADERS} DESTINATION include/ise/main)
