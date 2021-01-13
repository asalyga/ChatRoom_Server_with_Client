TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++0x -pthread
LIBS += -pthread

SOURCES += main.c \
    server.c \
    readTimeOut.c

HEADERS += \
    readtimeout.h \
    server.h \
    systemsettings.h
