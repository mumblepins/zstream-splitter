
cmake_minimum_required(VERSION 3.0)

project(zstream-splitter
        VERSION 0.1.0
        LANGUAGES C)


set(BUILD_SHARED_LIBS OFF)

set(CMAKE_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CMAKE_PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CMAKE_PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})


set(CMAKE_C_STANDARD 11)

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
        /lib
        /usr/lib
        /usr/local/lib)

set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}_source)

set(CPACK_GENERATOR TGZ DEB)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

set(CPACK_PACKAGE_CONTACT "Daniel Sullivan <mumblepins@gmail.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/mumblepins/zstream-splitter")
set(CPACK_DEBIAN_PACKAGE_NAME "zstream-splitter")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_COMPRESSION_TYPE xz)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
set(CPACK_DEBIAN_PACKAGE_SECTION utils)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
        "A program to break up zfs send streams and display appropriate resume token")
INCLUDE(CPack)


find_library(LIBNVPAIR NAMES libnvpair libnvpair.so libnvpair.so.2)
find_library(LIBZFS NAMES libzfs libzfs.so libzfs.so.2)
find_library(LIBZFS_CORE NAMES libzfs_core libzfs_core.so libzfs_core.so.1)
find_library(LIBZPOOL NAMES libzpool libzpool.so libzpool.so.2)
message("libnvpair path ${LIBNVPAIR}")
message("libzfs path ${LIBZFS}")
message("libzfs_core path ${LIBZFS_CORE}")
message("libzpool path ${LIBZPOOL}")

SET(SYS_NAME_ARCH "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

configure_file(config.h.cmake config.h @ONLY)
include_directories(/usr/include/libspl)
include_directories(/usr/include/libzfs)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_executable(zstream-splitter main.c main.h define.h config.h)
TARGET_LINK_LIBRARIES(zstream-splitter
        ${LIBNVPAIR}
        ${LIBZFS}
        ${LIBZFS_CORE}
        ${LIBZPOOL})

install(TARGETS zstream-splitter
        RUNTIME DESTINATION bin)

install(PROGRAMS zstream-split-gdrive DESTINATION bin )