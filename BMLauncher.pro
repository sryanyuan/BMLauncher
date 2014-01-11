#-------------------------------------------------
#
# Project created by QtCreator 2013-12-17T10:22:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BMLauncher
TEMPLATE = app


SOURCES += main.cpp\
        dialog.cpp \
    ConvertUTF.c

HEADERS  += dialog.h \
    SimpleIni.h \
    ConvertUTF.h

FORMS    += dialog.ui

RESOURCES += \
    Resource/Resource.qrc
