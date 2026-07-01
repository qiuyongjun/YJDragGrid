cmake_minimum_required(VERSION 3.16)

project(QtDragGridConsumer LANGUAGES CXX)

find_package(QtDragGrid CONFIG REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(QtDragGridConsumer main.cpp)
target_link_libraries(QtDragGridConsumer PRIVATE QtDragGrid::QtDragGrid Qt${QT_VERSION_MAJOR}::Widgets)
