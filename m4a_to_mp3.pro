QT += widgets

TARGET = m4a_to_mp3
TEMPLATE = app
CONFIG += c++17

SOURCES += main.cpp

QMAKE_CXXFLAGS += -Wall -Wextra

DISTFILES += \
    assets/sesdonustur.svg \
    packaging/sesdonustur.desktop
