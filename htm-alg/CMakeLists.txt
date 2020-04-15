include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)

cmake_minimum_required(VERSION 2.8)

project(htm_sgl)

if ($ENV{NVM_LATENCY_RDTSCP} EQUAL 1)
	add_definitions(-DNVM_LATENCY_RDTSCP=$ENV{NVM_LATENCY_RDTSCP} -DNVM_WAIT_CYCLES=$ENV{NVM_WAIT_CYCLES})
endif()
add_definitions(-DNDEBUG=$ENV{NDEBUG} -DVERBOSE=$ENV{VERBOSE} -DBUDGET=$ENV{BUDGET} -DNVM_LATENCY=$ENV{NVM_LATENCY})

include(${PROJECT_SOURCE_DIR}/cmake_utils.txt)

file(GLOB_RECURSE htm_sgl_SOURCES
  ABSOLUTE "${PROJECT_SOURCE_DIR}/src/"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}/include")

add_library(htm_sgl ${htm_sgl_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake_deb.txt)

message(STATUS "Checking arch_dep...")
if(NOT ARCH_INC_DIR)
	message(FATAL_ERROR "Please point ARCH_INC_DIR to the architecture dependencies.")
endif()
include_directories("${ARCH_INC_DIR}")

