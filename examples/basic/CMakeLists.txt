set(QTDRAGGRID_BASIC_EXAMPLE_SOURCES
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
        ${QTDRAGGRID_BASIC_EXAMPLE_SOURCES}
    )
else()
    if(ANDROID)
        add_library(QtDragGridDemo SHARED ${QTDRAGGRID_BASIC_EXAMPLE_SOURCES})
    else()
        add_executable(QtDragGridDemo ${QTDRAGGRID_BASIC_EXAMPLE_SOURCES})
    endif()
endif()

target_link_libraries(QtDragGridDemo PRIVATE QtDragGrid::QtDragGrid Qt${QT_VERSION_MAJOR}::Widgets)

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

if(QTDRAGGRID_INSTALL)
    install(TARGETS QtDragGridDemo
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .
    )
endif()
