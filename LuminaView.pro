QT += core gui widgets svg concurrent printsupport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
QMAKE_CXXFLAGS += -O3 -funroll-loops
TARGET = LuminaView
TEMPLATE = app
SOURCES += main.cpp
