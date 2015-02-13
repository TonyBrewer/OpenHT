QT       = core
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app
TARGET = app_sysc
SOURCES += src/Main.cpp src/Pers%UNIT_NAME%%MODULE_NAME%_src.cpp

OTHER_FILES += src/%ProjectName%.htd Makefile
