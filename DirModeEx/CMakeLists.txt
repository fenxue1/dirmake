cmake_minimum_required(VERSION 3.16)
project(DirModeEx LANGUAGES CXX)

# Windows 7 兼容建议：使用 Qt 5.15.x + MSVC v141 或 MinGW 7.x
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets Concurrent)

set(SOURCES
    main.cpp
    mainwindow.cpp
    text_extractor.cpp
    csv_parser.cpp
    csv_lang_plugin.cpp
    diff_utils.cpp
    language_settings.cpp
)
set(HEADERS
    mainwindow.h
    text_extractor.h
    csv_parser.h
    csv_lang_plugin.h
    diff_utils.h
    language_settings.h
)

qt5_wrap_ui(UI_FILES mainwindow.ui)

add_executable(DirModeEx
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

target_link_libraries(DirModeEx PRIVATE Qt5::Widgets Qt5::Concurrent)

if (WIN32)
    # 指定 Win7 兼容的子系统与资源设置可按需添加
    target_compile_definitions(DirModeEx PRIVATE QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII)
endif()

install(TARGETS DirModeEx RUNTIME DESTINATION bin)