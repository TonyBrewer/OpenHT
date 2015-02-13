#-------------------------------------------------
#
# Project created by QtCreator 2012-10-17T08:25:19
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mem_view
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    data.cpp

HEADERS  += MainWindow.h \
    data.h \
    ../../../ht_lib/sysc/HtMemTrace.h

FORMS    += MainWindow.ui
