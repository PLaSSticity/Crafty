include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)

cmake_minimum_required(VERSION 2.8)

project(minimal_nvm)

if ($ENV{NVM_LATENCY_RDTSCP} EQUAL 1)
    add_definitions(-DNVM_LATENCY_RDTSCP=$ENV{NVM_LATENCY_RDTSCP} -DNVM_WAIT_CYCLES=$ENV{NVM_WAIT_CYCLES})
endif()
add_definitions(-DNDEBUG=$ENV{NDEBUG} -DVERBOSE=$ENV{VERBOSE} -DBUDGET=$ENV{BUDGET} -DNVM_LATENCY=$ENV{NVM_LATENCY})

include(${PROJECT_SOURCE_DIR}/cmake_utils.txt)

file(GLOB_RECURSE minimal_nvm_SOURCES
  ABSOLUTE "${PROJECT_SOURCE_DIR}/src/"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${ARCH_INC_DIR}")

add_library(minimal_nvm ${minimal_nvm_SOURCES})

include(${PROJECT_SOURCE_DIR}/cmake_deb.txt)

if (USE_P8 STREQUAL "1")
  message(STATUS "Set \"USE_P8\" active")
  add_definitions(-DUSE_P8=1)
endif()

execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                OUTPUT_VARIABLE GCC_VERSION)
if (GCC_VERSION VERSION_LESS 5.0)
  add_definitions(-DOLD_ALLOC)
endif()

# check for arch dependecies
message(STATUS "Checking dependencies...")
if(NOT ARCH_INC_DIR)
	message(FATAL_ERROR "Please point ARCH_INC_DIR.")
endif()

