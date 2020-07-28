project(wolkenbase)
cmake_minimum_required(VERSION 3.8.0)

include(TestBigEndian)
include(CheckTypeSize)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17) # appeared in CMake 3.8
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
add_definitions(-DBOOST_ALL_NO_LIB)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
set(Boost_USE_STATIC_LIBS ON)
else ()
set(Boost_USE_STATIC_LIBS OFF)
endif ()

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
               flowsnake.cpp freeram.cpp las.cpp ldecimal.cpp
               octree.cpp point.cpp ps.cpp quaternion.cpp
               random.cpp relprime.cpp shape.cpp testpattern.cpp threads.cpp wolkenbase.cpp)

add_executable(wolkentest angle.cpp binio.cpp boundrect.cpp cloud.cpp eisenstein.cpp
               flowsnake.cpp freeram.cpp las.cpp ldecimal.cpp
               octree.cpp point.cpp ps.cpp quaternion.cpp
               random.cpp relprime.cpp shape.cpp testpattern.cpp threads.cpp wolkentest.cpp)

if (${Boost_FOUND})
target_link_libraries(wolkenbase ${Boost_LIBRARIES})
include_directories(${Boost_INCLUDE_DIR})
endif (${Boost_FOUND})
target_link_libraries(wolkentest ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(wolkenbase ${CMAKE_THREAD_LIBS_INIT})
target_compile_definitions(wolkenbase PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(wolkentest PUBLIC _USE_MATH_DEFINES)
install(TARGETS wolkenbase DESTINATION bin)

include(CheckIncludeFiles)
check_include_files(time.h HAVE_TIME_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(sys/sysinfo.h HAVE_SYS_SYSINFO_H)
check_include_files(windows.h HAVE_WINDOWS_H)
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
