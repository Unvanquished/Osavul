#-------------------------------------------------
#
# Project created by QtCreator 2012-02-25T21:10:51
#
#-------------------------------------------------

QT += core gui network

TARGET = osavul
TEMPLATE = app
CONFIG += qt debug

SOURCES += main.cpp\
    mainwindow.cpp \
    connectiondialog.cpp \
    unv.cpp \
    ircclient.cpp \
    richtextdelegate.cpp \
    settingsdialog.cpp \
    channel.cpp

HEADERS  += mainwindow.h \
    connectiondialog.h \
    unv.h \
    ircclient.h \
    richtextdelegate.h \
    settingsdialog.h \
    channel.h

FORMS += mainwindow.ui \
    connectiondialog.ui \
    settingsdialog.ui \
    channel.ui

TRANSLATIONS = osavul_ua.ts

QMAKE_CXXFLAGS += -std=c++0x -Wall

RESOURCES += osavul.qrc

OTHER_FILES += \
    LICENSE.txt
