find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick)

qt_policy(SET QTP0001 NEW)
qt_standard_project_setup()

qt_add_executable(framework_qt6_app
    main.cpp
)

qt_add_qml_module(framework_qt6_app
    URI Framework.Qt6App
    VERSION 1.0
    SOURCES
        adapters/runtime_bridge.cpp
        adapters/runtime_bridge.h
    QML_FILES
        Main.qml
)

target_include_directories(framework_qt6_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/adapters
)

target_link_libraries(framework_qt6_app PRIVATE
    framework_runtime
    framework_example_module
    Qt6::Quick
)

if(TARGET framework_example_plugin)
    add_dependencies(framework_qt6_app framework_example_plugin)
    add_custom_command(TARGET framework_qt6_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:framework_qt6_app>/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:framework_example_plugin>"
            "$<TARGET_FILE_DIR:framework_qt6_app>/plugins/"
        VERBATIM
    )
else()
    message(WARNING "framework_example_plugin is unavailable; the Qt6 app will report a missing plugin at runtime")
endif()

if(WIN32)
    find_program(QT_DEPLOY_TOOL
        NAMES windeployqt windeployqt.exe
        HINTS "${Qt6_DIR}/../../../bin"
    )

    if(QT_DEPLOY_TOOL)
        add_custom_command(TARGET framework_qt6_app POST_BUILD
            COMMAND "${QT_DEPLOY_TOOL}"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
                "$<TARGET_FILE:framework_qt6_app>"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found; the Qt6 app may not run without Qt DLLs on PATH")
    endif()
endif()
