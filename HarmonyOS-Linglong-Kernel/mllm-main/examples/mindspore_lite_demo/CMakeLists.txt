cmake_minimum_required(VERSION 3.16)
project(mindspore_lite_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED MSLITE_ROOT)
  message(FATAL_ERROR "Set MSLITE_ROOT to MindSpore Lite install prefix")
endif()

set(MSLITE_INCLUDE "${MSLITE_ROOT}/include")
set(MSLITE_LIB_DIR "${MSLITE_ROOT}/lib")

include_directories(${MSLITE_INCLUDE})
link_directories(${MSLITE_LIB_DIR})

add_executable(mindspore_lite_demo mindspore_lite_demo.cpp)
target_link_libraries(mindspore_lite_demo mindspore-lite)

