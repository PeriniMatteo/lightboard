######################################################################
# Automatically generated by qmake (2.01a) mar nov 10 09:49:29 2015
######################################################################

TEMPLATE = app
LIBS += -ludev -lX11 -lXtst -lpthread -lbluetooth
CONFIG += warn_off
RESOURCES = systray.qrc

# Input
SOURCES += src/*.cpp lib/*
HEADERS += include/main.h include/QProgressIndicator.h

QT += gui widgets
TARGET = lightboard
TRANSLATIONS = resources/$${TARGET}_en.ts resources/$${TARGET}_it.ts
CODECFORSRC = UTF-8
