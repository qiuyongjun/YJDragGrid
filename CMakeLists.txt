cmake_minimum_required(VERSION 3.16)

project(GridLayout VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Test)

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
    qt_add_executable(GridLayoutDemo
        MANUAL_FINALIZATION
        ${DEMO_SOURCES}
    )
else()
    if(ANDROID)
        add_library(GridLayoutDemo SHARED ${DEMO_SOURCES})
    else()
        add_executable(GridLayoutDemo ${DEMO_SOURCES})
    endif()
endif()

target_link_libraries(GridLayoutDemo PRIVATE DragGrid)

# Copy the stylesheet next to the executable so the demo can load it at runtime.
set(QSS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/style.qss")
add_custom_command(TARGET GridLayoutDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${QSS_FILE}" "$<TARGET_FILE_DIR:GridLayoutDemo>/style.qss"
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.GridLayoutDemo)
endif()
set_target_properties(GridLayoutDemo PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GridLayoutDemo)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# Install / export
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS DragGrid GridLayoutDemo
    EXPORT GridLayoutTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

install(FILES
    DragGridLayout.h
    DragGridWidget.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT GridLayoutTargets
    FILE GridLayoutTargets.cmake
    NAMESPACE GridLayout::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GridLayout
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GridLayoutConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/GridLayoutConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GridLayout
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/GridLayoutConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/GridLayoutConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/GridLayoutConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GridLayout
)
