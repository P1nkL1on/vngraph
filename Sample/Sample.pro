QT -= gui
CONFIG -= console
CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17
QMAKE_CXXFLAGS += -Werror=return-type

SOURCES += main.cpp

HEADERS += \
    headers.h \
    utils.h
