project(wolkenbase)
cmake_minimum_required(VERSION 3.1.0)

include(TestBigEndian)
include(CheckTypeSize)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(wolkenbase angle.cpp binio.cpp cloud.cpp
               las.cpp ldecimal.cpp octree.cpp point.cpp quaternion.cpp
               random.cpp testpattern.cpp wolkenbase.cpp)

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
