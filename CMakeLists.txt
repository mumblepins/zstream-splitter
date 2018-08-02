
cmake_minimum_required(VERSION 3.0)

project(zstream_splitter
        VERSION 0.1
        LANGUAGES C)

set(CMAKE_C_STANDARD 11)

set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}_source)

set(CPACK_GENERATOR TGZ)
set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

INCLUDE(CPack)

add_library(libnvpair STATIC IMPORTED)

set_target_properties(libnvpair PROPERTIES IMPORTED_LOCATION "/usr/lib/libnvpair.a")

add_library(libzfs SHARED IMPORTED)
set_target_properties(libzfs PROPERTIES IMPORTED_LOCATION "/lib/libzfs.so.2")

add_library(libzfs_core SHARED IMPORTED)
set_target_properties(libzfs_core PROPERTIES IMPORTED_LOCATION "/lib/libzfs_core.so.1")

add_library(libzpool SHARED IMPORTED)
set_target_properties(libzpool PROPERTIES IMPORTED_LOCATION "/lib/libzpool.so.2")


SET(SYS_NAME_ARCH "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

configure_file(config.h.cmake config.h @ONLY)
include_directories(/usr/include/libspl)
include_directories(/usr/include/libzfs)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_executable(zstream_splitter main.c main.h define.h config.h)
TARGET_LINK_LIBRARIES(zstream_splitter libnvpair libzfs libzfs_core libzpool)

install(TARGETS zstream_splitter
        RUNTIME DESTINATION bin)