project(appmenu-qt)
cmake_minimum_required(VERSION 2.8.0)

# Versions
set(appmenu_qt_VERSION_MAJOR 0)
set(appmenu_qt_VERSION_MINOR 1)
set(appmenu_qt_VERSION_PATCH 2)
set(appmenu_qt_VERSION ${appmenu_qt_VERSION_MAJOR}.${appmenu_qt_VERSION_MINOR}.${appmenu_qt_VERSION_PATCH})

# Packaging
set(ARCHIVE_NAME appmenu-qt-${appmenu_qt_VERSION})
add_custom_target(dist
    COMMAND bzr export --root=${ARCHIVE_NAME} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_custom_target(distcheck
    COMMAND cd ${CMAKE_BINARY_DIR}
    && rm -rf ${ARCHIVE_NAME}
    && tar xf ${ARCHIVE_NAME}.tar.bz2
    && mkdir ${ARCHIVE_NAME}/build
    && cd ${ARCHIVE_NAME}/build
    && cmake -DCMAKE_INSTALL_PREFIX=../install ..
    && make
    && make install
    )
add_dependencies(distcheck dist)

# Dependencies
find_package(PkgConfig REQUIRED)
find_package(Qt4 REQUIRED)
pkg_check_modules(DBUSMENUQT REQUIRED dbusmenu-qt)

# Source
add_subdirectory(src)
