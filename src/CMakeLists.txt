set(whitebox_sources
    #WhiteBox.aps
    Client.cpp
    ConfigHelper.cpp
    ip4validator.cpp
    LineEdit.cpp
    main.cpp
    MainWindow.cpp
    MsgDetailDialog.cpp
    MsgEditorDialog.cpp
    MsgIgnoreDialog.cpp
    NetManager.cpp
    ProtoManager.cpp
    Session.cpp
    SettingDialog.cpp
    WhiteBox.ico
    json.lua
    WhiteBox.qrc
    MainWindow.ui
    MessageDetail.ui
    MessageEditor.ui
    MessageIgnore.ui
    Setting.ui
    protobuf/protobuflib.c
    protobuf/protobuf.cpp
    Client.h
    ComboxItemFilter.h
    ConfigHelper.h
    define.h
    IClientNetEvent.h
    INetEvent.h
    ip4validator.h
    JsonHighlighter.h
    LineEdit.h
    LogHelper.h
    LuaInterface.h
    LuaScriptSystem.h
    MainWindow.h
    MsgDetailDialog.h
    MsgEditorDialog.h
    MsgIgnoreDialog.h
    NetManager.h
    ProtoManager.h
    Session.h
    SettingDialog.h
    Singleton.h
    protobuf/protobuf.h
    protobuf/protobuflib.h
)

if (WIN32)
    list(APPEND whitebox_sources WhiteBox.rc)
endif()

add_executable(${APP_NAME} ${whitebox_sources})
target_include_directories(${APP_NAME} PUBLIC ${protobuf_source_dir}/src)

target_link_libraries(${APP_NAME}
    libprotobuf
    fmt
    cjson
    liblua
    libluabridge
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
)

if(WIN32 AND MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
		WIN32_EXECUTABLE YES
		LINK_FLAGS "/ENTRY:mainCRTStartup"
	)
endif()

if (UNIX)
    install(TARGETS ${APP_NAME}
        RUNTIME DESTINATION bin)
endif()

#if (WIN32)
#    get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
#    get_filename_component(QT5_WINDEPLOYQT_EXECUTABLE ${QT5_QMAKE_EXECUTABLE} PATH)
#    set(QT5_WINDEPLOYQT_EXECUTABLE "${QT5_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
#
#    add_custom_command(TARGET ${APP_NAME} POST_BUILD
#       COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} --qmldir ${CMAKE_SOURCE_DIR} $<TARGET_FILE_DIR:${APP_NAME}>)
#endif(WIN32)

