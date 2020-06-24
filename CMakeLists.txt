project(wolkenbase)
cmake_minimum_required(VERSION 3.8.0)

include(TestBigEndian)
include(CheckTypeSize)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17) # appeared in CMake 3.8
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)
if (${FORCE_COLORED_OUTPUT})
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options (-fdiagnostics-color=always)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options (-fcolor-diagnostics)
  endif ()
endif ()

find_package(Boost COMPONENTS program_options)
find_package(Threads)

add_executable(wolkenbase angle.cpp binio.cpp boundrect.cpp cloud.cpp eisenstein.cpp
               flowsnake.cpp las.cpp ldecimal.cpp octree.cpp point.cpp ps.cpp quaternion.cpp
               random.cpp relprime.cpp shape.cpp testpattern.cpp threads.cpp wolkenbase.cpp)

add_executable(wolkentest angle.cpp binio.cpp boundrect.cpp cloud.cpp eisenstein.cpp
               flowsnake.cpp las.cpp ldecimal.cpp octree.cpp point.cpp ps.cpp quaternion.cpp
               random.cpp relprime.cpp shape.cpp testpattern.cpp threads.cpp wolkentest.cpp)

if (${Boost_FOUND})
target_link_libraries(wolkenbase ${Boost_LIBRARIES})
endif (${Boost_FOUND})
target_link_libraries(wolkentest ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(wolkenbase ${CMAKE_THREAD_LIBS_INIT})
set(CMAKE_CXX_FLAGS "-std=c++11")
install(TARGETS wolkenbase DESTINATION bin)

test_big_endian(BIGENDIAN)

set(WOLKENBASE_MAJOR_VERSION 0)
set(WOLKENBASE_MINOR_VERSION 1)
set(WOLKENBASE_PATCH_VERSION 0)
set(WOLKENBASE_VERSION ${WOLKENBASE_MAJOR_VERSION}.${WOLKENBASE_MINOR_VERSION}.${WOLKENBASE_PATCH_VERSION})

include_directories(${PROJECT_BINARY_DIR})
configure_file (config.h.in config.h)

set(CPACK_PACKAGE_VERSION_MAJOR ${WOLKENBASE_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${WOLKENBASE_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${WOLKENBASE_PATCH_VERSION})
set(CPACK_SOURCE_IGNORE_FILES /\\\\.git;.*~)
include(CPack)
