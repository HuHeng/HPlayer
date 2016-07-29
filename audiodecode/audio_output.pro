QT += core gui
#QT -= gui
QT += multimedia
QT += widgets

CONFIG += c++11

TARGET = audio_output
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    audioplayer.cpp

HEADERS += \
    audioplayer.h
