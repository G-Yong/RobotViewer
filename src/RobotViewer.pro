QT       += core gui xml
QT       += 3dcore 3drender 3dinput 3dextras 3dlogic
QT       += qml quick quickcontrols2

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
# CONFIG += console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    urdfparser.cpp \
    assimpmodelloader.cpp \
    robotentity.cpp \
    robotscene.cpp \
    trajectoryentity.cpp \
    settingsmanager.cpp \
    viewoptions.cpp \
    opcuabindingmodel.cpp \
    endeffectorconfigmodel.cpp \
    orbitcameracontroller.cpp

HEADERS += \
    commontypes.h \
    urdfparser.h \
    assimpmodelloader.h \
    robotentity.h \
    robotscene.h \
    trajectoryentity.h \
    settingsmanager.h \
    viewoptions.h \
    opcuabindingmodel.h \
    endeffectorconfigmodel.h \
    orbitcameracontroller.h


    SOURCES += \
        main.cpp \
        robotbridge.cpp
    
    HEADERS += \
        robotbridge.h
    
    RESOURCES += \
        resources.qrc


# Default rules for deployment.

win32 {
    # assimp动态库及其依赖的动态库所在目录
    INCLUDEPATH += $$PWD/../../../Assimp/include
    LIBS += -L$$PWD/../../../Assimp/lib -lassimp-vc142-mt
    LIBS += -L$$PWD/../../../Assimp/bin


    # 这是使用vcpkg安装assimp后的配置
    INCLUDEPATH += F:/reposities/vcpkg/installed/x64-windows/include
    LIBS += -LF:/reposities/vcpkg/installed/x64-windows/lib -lassimp-vc142-mt
    LIBS += -LF:/reposities/vcpkg/installed\x64-windows/bin

}
win32: LIBS += -lws2_32 -liphlpapi
win32: DEFINES += WIN32_LEAN_AND_MEAN

include(./communication/communication.pri)
