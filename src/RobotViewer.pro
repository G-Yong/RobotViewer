QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
# CONFIG += console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    urdfparser.cpp \
    assimpmodelloader.cpp \
    robotentity.cpp \
    robotscene.cpp \
    orbitcameracontroller.cpp \
    trajectoryentity.cpp \
    jointcontrolwidget.cpp \
    opcuawidget.cpp \
    settingsmanager.cpp

HEADERS += \
    mainwindow.h \
    urdfparser.h \
    assimpmodelloader.h \
    robotentity.h \
    robotscene.h \
    orbitcameracontroller.h \
    trajectoryentity.h \
    jointcontrolwidget.h \
    opcuawidget.h \
    settingsmanager.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


QT += 3dcore 3drender 3dinput 3dextras
QT += widgets

win32 {
    # assimp动态库及其依赖的动态库所在目录
    INCLUDEPATH += $$PWD/../../../Assimp/include
    LIBS += -L$$PWD/../../../Assimp/lib -lassimp-vc142-mt
    LIBS += -L$$PWD/../../../Assimp/bin
}
win32: LIBS += -lws2_32 -liphlpapi
win32: DEFINES += WIN32_LEAN_AND_MEAN

include(./communication/communication.pri)
