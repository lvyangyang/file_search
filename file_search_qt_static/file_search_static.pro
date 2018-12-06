#-------------------------------------------------
#
# Project created by QtCreator 2018-11-30T13:46:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = file_search_static
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    db_search.cpp \
    get_drives.cpp \
    main.cpp \
    mainwindow.cpp \
    table_widget.cpp

HEADERS += \
        mainwindow.h \
    db_search.h \
    get_drives.h \
    mainwindow.h \
    sqlite3.h \
    table_widget.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: LIBS += -L'E:/Program Files/sqlite/' -lsqlite3

INCLUDEPATH += 'E:/Program Files/sqlite'
DEPENDPATH += 'E:/Program Files/sqlite'

RC_ICONS =search.ico
