QT += network concurrent

DEFINES += COM_OPCUA

SOURCES += \
    $$PWD/baseconnector.cpp

HEADERS += \
    $$PWD/baseconnector.h

contains(DEFINES, COM_OPCUA){
SOURCES += \
    $$PWD/opcua/opcuaconnector.cpp \
    $$PWD/opcua/open62541.c

HEADERS += \
    $$PWD/opcua/opcuaconnector.h \
    $$PWD/opcua/open62541.h
}

INCLUDEPATH += $$PWD
