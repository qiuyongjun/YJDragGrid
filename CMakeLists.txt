cmake_minimum_required(VERSION 3.16)

project(QtDragGrid VERSION 0.3.0 LANGUAGES CXX)

set(QTDRAGGRID_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(QTDRAGGRID_IS_TOP_LEVEL ON)
endif()

set(QTDRAGGRID_DEFAULT_BUILD_EXAMPLES ${QTDRAGGRID_IS_TOP_LEVEL})
if(DEFINED ENV{CI})
    set(QTDRAGGRID_DEFAULT_BUILD_EXAMPLES OFF)
endif()

set(QTDRAGGRID_BUILD_EXAMPLES_WAS_DEFINED OFF)
if(DEFINED QTDRAGGRID_BUILD_EXAMPLES)
    set(QTDRAGGRID_BUILD_EXAMPLES_WAS_DEFINED ON)
endif()
set(QTDRAGGRID_BUILD_DEMO_WAS_DEFINED OFF)
if(DEFINED QTDRAGGRID_BUILD_DEMO)
    set(QTDRAGGRID_BUILD_DEMO_WAS_DEFINED ON)
endif()

option(QTDRAGGRID_BUILD_SHARED "Build QtDragGrid as a shared library" OFF)
option(QTDRAGGRID_BUILD_EXAMPLES "Build QtDragGrid example applications" ${QTDRAGGRID_DEFAULT_BUILD_EXAMPLES})
option(QTDRAGGRID_BUILD_DEMO "Compatibility alias for QTDRAGGRID_BUILD_EXAMPLES" ${QTDRAGGRID_BUILD_EXAMPLES})
option(QTDRAGGRID_BUILD_TESTS "Build QtDragGrid tests" ${QTDRAGGRID_IS_TOP_LEVEL})
option(QTDRAGGRID_INSTALL "Install QtDragGrid targets and headers" ON)
option(QTDRAGGRID_INSTALL_EXAMPLES "Install QtDragGrid example applications" OFF)

# Prefer the new option when both switches exist so stale demo cache cannot reopen examples.
set(QTDRAGGRID_ENABLE_EXAMPLES ${QTDRAGGRID_BUILD_EXAMPLES})
if(QTDRAGGRID_BUILD_EXAMPLES_WAS_DEFINED)
    set(QTDRAGGRID_ENABLE_EXAMPLES ${QTDRAGGRID_BUILD_EXAMPLES})
elseif(QTDRAGGRID_BUILD_DEMO_WAS_DEFINED)
    set(QTDRAGGRID_ENABLE_EXAMPLES ${QTDRAGGRID_BUILD_DEMO})
endif()
set(QTDRAGGRID_BUILD_EXAMPLES ${QTDRAGGRID_ENABLE_EXAMPLES} CACHE BOOL "Build QtDragGrid example applications" FORCE)
set(QTDRAGGRID_BUILD_DEMO ${QTDRAGGRID_ENABLE_EXAMPLES} CACHE BOOL "Compatibility alias for QTDRAGGRID_BUILD_EXAMPLES" FORCE)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

if(QTDRAGGRID_BUILD_TESTS)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
endif()

set(QTDRAGGRID_LIBRARY_TYPE STATIC)
if(QTDRAGGRID_BUILD_SHARED)
    set(QTDRAGGRID_LIBRARY_TYPE SHARED)
endif()

set(QTDRAGGRID_PUBLIC_HEADERS
    include/QtDragGrid/QtDragGridGlobal.h
    include/QtDragGrid/DragGridLayout.h
    include/QtDragGrid/DragGridWidget.h
)

set(QTDRAGGRID_COMPAT_HEADERS
    include/DragGridLayout.h
    include/DragGridWidget.h
)

set(QTDRAGGRID_SOURCES
    src/DragGridLayout.cpp
    src/DragGridWidget.cpp
)

add_library(QtDragGrid ${QTDRAGGRID_LIBRARY_TYPE}
    ${QTDRAGGRID_PUBLIC_HEADERS}
    ${QTDRAGGRID_COMPAT_HEADERS}
    ${QTDRAGGRID_SOURCES}
)

add_library(QtDragGrid::QtDragGrid ALIAS QtDragGrid)
add_library(DragGrid ALIAS QtDragGrid)

target_compile_features(QtDragGrid PUBLIC cxx_std_17)
target_include_directories(QtDragGrid
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(QtDragGrid PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)
target_compile_definitions(QtDragGrid PRIVATE QTDRAGGRID_LIBRARY)
if(NOT QTDRAGGRID_BUILD_SHARED)
    target_compile_definitions(QtDragGrid PUBLIC QTDRAGGRID_STATIC)
endif()

set_target_properties(QtDragGrid PROPERTIES
    OUTPUT_NAME QtDragGrid
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME QtDragGrid
)

if(QTDRAGGRID_ENABLE_EXAMPLES)
    add_subdirectory(examples/basic)
endif()

if(QTDRAGGRID_BUILD_TESTS)
    enable_testing()

    set(TEST_FILES
        tests/TestDragGridLayout.cpp
        tests/TestDragGridWidget.cpp
    )

    foreach(test_file ${TEST_FILES})
        get_filename_component(test_name ${test_file} NAME_WE)
        add_executable(${test_name} ${test_file})
        target_link_libraries(${test_name} PRIVATE QtDragGrid::QtDragGrid Qt${QT_VERSION_MAJOR}::Test)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

if(QTDRAGGRID_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS QtDragGrid
        EXPORT QtDragGridTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/QtDragGrid
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )

    install(FILES ${QTDRAGGRID_COMPAT_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT QtDragGridTargets
        FILE QtDragGridTargets.cmake
        NAMESPACE QtDragGrid::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtDragGrid
    )

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtDragGridConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/QtDragGridConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtDragGrid
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/QtDragGridConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/QtDragGridConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/QtDragGridConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtDragGrid
    )
endif()
