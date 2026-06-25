cmake_minimum_required(VERSION 3.16)

project(QtDragGrid VERSION 0.2.0 LANGUAGES CXX)

set(QTDRAGGRID_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(QTDRAGGRID_IS_TOP_LEVEL ON)
endif()

set(QTDRAGGRID_DEFAULT_BUILD_DEMO ${QTDRAGGRID_IS_TOP_LEVEL})
if(DEFINED ENV{CI})
    set(QTDRAGGRID_DEFAULT_BUILD_DEMO OFF)
endif()

option(QTDRAGGRID_BUILD_DEMO "Build the GridLayout demo application" ${QTDRAGGRID_DEFAULT_BUILD_DEMO})
option(QTDRAGGRID_BUILD_TESTS "Build GridLayout tests" ${QTDRAGGRID_IS_TOP_LEVEL})

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

if(QTDRAGGRID_BUILD_TESTS)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
endif()

# ---------------------------------------------------------------------------
# Library: DragGrid
# ---------------------------------------------------------------------------
set(DRAGGRID_SOURCES
    DragGridLayout.h
    DragGridLayout.cpp
    DragGridWidget.h
    DragGridWidget.cpp
)

add_library(DragGrid STATIC ${DRAGGRID_SOURCES})

# Make headers available to consumers both during build and after install.
target_include_directories(DragGrid
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(DragGrid PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

# ---------------------------------------------------------------------------
# Demo application
# ---------------------------------------------------------------------------
if(QTDRAGGRID_BUILD_DEMO)
    set(DEMO_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        MainWindow.ui
        CardWidget.cpp
        CardWidget.h
        CardWidget.ui
    )

    if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        qt_add_executable(QtDragGridDemo
            MANUAL_FINALIZATION
            ${DEMO_SOURCES}
        )
    else()
        if(ANDROID)
            add_library(QtDragGridDemo SHARED ${DEMO_SOURCES})
        else()
            add_executable(QtDragGridDemo ${DEMO_SOURCES})
        endif()
    endif()

    target_link_libraries(QtDragGridDemo PRIVATE DragGrid Qt${QT_VERSION_MAJOR}::Widgets)

    # Copy the stylesheet next to the executable so the demo can load it at runtime.
    set(QSS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/style.qss")
    add_custom_command(TARGET QtDragGridDemo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QSS_FILE}" "$<TARGET_FILE_DIR:QtDragGridDemo>/style.qss"
    )

    if(${QT_VERSION} VERSION_LESS 6.1.0)
        set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.QtDragGridDemo)
    endif()
    set_target_properties(QtDragGridDemo PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
    )

    if(QT_VERSION_MAJOR EQUAL 6)
        qt_finalize_executable(QtDragGridDemo)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(QTDRAGGRID_BUILD_TESTS)
    enable_testing()

    set(TEST_FILES
        tests/TestDragGridLayout.cpp
        tests/TestDragGridWidget.cpp
    )

    foreach(test_file ${TEST_FILES})
        get_filename_component(test_name ${test_file} NAME_WE)
        add_executable(${test_name} ${test_file})
        target_link_libraries(${test_name} PRIVATE DragGrid Qt${QT_VERSION_MAJOR}::Test)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Install / export
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS DragGrid
    EXPORT QtDragGridTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

if(QTDRAGGRID_BUILD_DEMO)
    install(TARGETS QtDragGridDemo
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
    )
endif()

install(FILES
    DragGridLayout.h
    DragGridWidget.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT QtDragGridTargets
    FILE QtDragGridTargets.cmake
    NAMESPACE QtDragGrid::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtDragGrid
)

include(CMakePackageConfigHelpers)
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
