cmake_minimum_required (VERSION 2.8)

project ("websocket")

option(WEBSOCKET_BUILD_EXAMPLES "Build websocket examples" OFF)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wfatal-errors")

set(CMAKE_LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_INCLUDE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/include)

set(websocket_include_dirs ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(SYSTEM ${websocket_include_dirs})

macro (add_sources)
  file (RELATIVE_PATH _relPath "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
  foreach (_src ${ARGN})
    if (_relPath)
      list (APPEND SRCS "${_relPath}/${_src}")
    else()
      list (APPEND SRCS "${_src}")
    endif ()
  endforeach ()
  if (_relPath)
    # propagate SRCS to parent directory
    set (SRCS ${SRCS} PARENT_SCOPE)
  endif ()
endmacro ()

aux_source_directory(websocket src_MAIN)

add_library(${PROJECT_NAME}
  ${src_MAIN}
)
set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${websocket_include_dirs})

if (WEBSOCKET_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif (WEBSOCKET_BUILD_EXAMPLES)
