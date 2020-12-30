project(wolkenbase)
cmake_minimum_required(VERSION 3.8.0)

include(TestBigEndian)
include(CheckTypeSize)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17) # appeared in CMake 3.8
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/perfecttin)
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

option(ENABLE_MITOBREVNO "Enable debugging with Mitobrevno if found." FALSE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(Qt5 COMPONENTS Core Widgets Gui LinguistTools REQUIRED)
find_package(Boost COMPONENTS program_options)
find_package(Mitobrevno)
find_package(Threads)

qt5_add_resources(lib_resources wolkenbase.qrc)
qt5_add_translation(qm_files wolkenbase_en.ts wolkenbase_es.ts)
# To update translations, run "lupdate *.cpp -ts *.ts" in the source directory.

# point.cpp must be before las.cpp for noPoint to be initialized to nanxyz.
add_executable(wolkenbase angle.cpp binio.cpp boundrect.cpp
	       brevno.cpp classify.cpp configdialog.cpp eisenstein.cpp
               fileio.cpp flowsnake.cpp freeram.cpp point.cpp las.cpp ldecimal.cpp
               leastsquares.cpp lissajous.cpp mainwindow.cpp manysum.cpp matrix.cpp
               octree.cpp peano.cpp ps.cpp quaternion.cpp
               random.cpp relprime.cpp scan.cpp shape.cpp testpattern.cpp
               threads.cpp tile.cpp unitbutton.cpp wkt.cpp wolkenbase.cpp wolkencanvas.cpp
               ${lib_resources} ${qm_files})

add_executable(wolkencli angle.cpp binio.cpp boundrect.cpp
	       brevno.cpp classify.cpp eisenstein.cpp
               flowsnake.cpp freeram.cpp point.cpp las.cpp ldecimal.cpp
               leastsquares.cpp manysum.cpp matrix.cpp octree.cpp ps.cpp quaternion.cpp
               random.cpp relprime.cpp scan.cpp shape.cpp testpattern.cpp
               threads.cpp tile.cpp wkt.cpp wolkencli.cpp)

add_executable(wolkentest angle.cpp binio.cpp boundrect.cpp
	       brevno.cpp classify.cpp eisenstein.cpp
               flowsnake.cpp freeram.cpp point.cpp las.cpp ldecimal.cpp leastsquares.cpp
               manysum.cpp matrix.cpp octree.cpp peano.cpp ps.cpp quaternion.cpp
               random.cpp relprime.cpp scan.cpp shape.cpp testpattern.cpp
               threads.cpp tile.cpp wkt.cpp wolkentest.cpp)

if (${Boost_FOUND})
target_link_libraries(wolkencli ${Boost_LIBRARIES})
include_directories(${Boost_INCLUDE_DIR})
endif (${Boost_FOUND})
if (${Mitobrevno_FOUND})
target_link_libraries(wolkencli ${Mitobrevno_LIBRARY})
target_link_libraries(wolkenbase ${Mitobrevno_LIBRARY})
target_link_libraries(wolkentest ${Mitobrevno_LIBRARY})
endif (${Mitobrevno_FOUND})
target_link_libraries(wolkentest ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(wolkencli ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(wolkenbase ${CMAKE_THREAD_LIBS_INIT} Qt5::Widgets Qt5::Core)
target_compile_definitions(wolkencli PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(wolkenbase PUBLIC _USE_MATH_DEFINES)
target_compile_definitions(wolkentest PUBLIC _USE_MATH_DEFINES)
install(TARGETS wolkenbase DESTINATION bin)

include(CheckIncludeFiles)
check_include_files(time.h HAVE_TIME_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(sys/sysinfo.h HAVE_SYS_SYSINFO_H)
check_include_files(sys/sysctl.h HAVE_SYS_SYSCTL_H)
check_include_files(windows.h HAVE_WINDOWS_H)
test_big_endian(BIGENDIAN)
if (EXISTS "/proc/meminfo")
set(HAVE_PROC_MEMINFO 1)
endif ()

set(WOLKENBASE_MAJOR_VERSION 0)
set(WOLKENBASE_MINOR_VERSION 1)
set(WOLKENBASE_PATCH_VERSION 0)
set(WOLKENBASE_VERSION ${WOLKENBASE_MAJOR_VERSION}.${WOLKENBASE_MINOR_VERSION}.${WOLKENBASE_PATCH_VERSION})
set(COPY_YEAR 2020)

include_directories(${PROJECT_BINARY_DIR})
if (${Mitobrevno_FOUND})
include_directories(${Mitobrevno_INCLUDE_DIR})
endif (${Mitobrevno_FOUND})
configure_file (config.h.in config.h)

set(CPACK_PACKAGE_VERSION_MAJOR ${WOLKENBASE_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${WOLKENBASE_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${WOLKENBASE_PATCH_VERSION})
set(CPACK_SOURCE_IGNORE_FILES /\\\\.git;.*~)
include(CPack)
