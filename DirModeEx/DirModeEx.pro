QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
QMAKE_TARGET_COMPANY = TyText
QMAKE_TARGET_PRODUCT = DirModeEx
QMAKE_TARGET_DESCRIPTION = Windows 7 compatible Dir/File manager
QMAKE_TARGET_COPYRIGHT = (c) 2025 TyText

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    text_extractor.cpp \
    csv_parser.cpp \
    csv_lang_plugin.cpp \
    diff_utils.cpp \
    language_settings.cpp

HEADERS += \
    mainwindow.h \
    text_extractor.h \
    csv_parser.h \
    csv_lang_plugin.h \
    diff_utils.h \
    language_settings.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
