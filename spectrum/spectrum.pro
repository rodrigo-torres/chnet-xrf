######################################################################
# Automatically generated by qmake (3.0) Mon May 15 10:55:49 2017
######################################################################

TEMPLATE = app
TARGET = spectrum
INCLUDEPATH += /usr/local/qwt-6.1.4/
INCLUDEPATH+= /usr/local/qwt-6.1.4/include/

LIBS += -L/usr/local/qwt-6.1.4/lib -lqwt

QT += widgets
QT += printsupport
CONFIG += qwt
CONFIG += console


# Input
HEADERS += complexnumber.h mainwindow.h pixmaps.h plot.h ../Shm.h
SOURCES += main.cpp mainwindow.cpp plot.cpp
RESOURCES += Resource.qrc