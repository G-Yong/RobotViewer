#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QIcon>
#include <QFont>
#include <QSurfaceFormat>
#include <QDebug>
#include <Qt3DCore/QEntity>

#include "robotbridge.h"
#include "orbitcameracontroller.h"

#pragma execution_character_set("utf-8")

int main(int argc, char *argv[])
{
    // 启用高DPI缩放
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    
    // 设置OpenGL格式 - 在创建QApplication之前设置
    QSurfaceFormat format;
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);
    
    QApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("RobotViewer");
    app.setApplicationVersion("0.0.5");
    app.setOrganizationName("RobotViewer");
    app.setOrganizationDomain("robotviewer.local");
    
    // 设置应用图标
    app.setWindowIcon(QIcon(":/icons/app_icon.png"));
    
    // 设置默认字体
    QFont defaultFont("Segoe UI", 10);
    app.setFont(defaultFont);
    
    // 设置QML样式
    QQuickStyle::setStyle("Basic");
    
    // 注册 Qt3D 类型用于 QML 参数传递
    qRegisterMetaType<Qt3DCore::QEntity*>("Qt3DCore::QEntity*");
    
    // 创建机器人桥接对象
    RobotBridge robotBridge;
    robotBridge.initialize();
    
    // 创建QML引擎
    QQmlApplicationEngine engine;
    
    // 添加QML导入路径
    engine.addImportPath("qrc:/qml");
    engine.addImportPath(":/qml");
    
    // 将robotBridge暴露给QML
    engine.rootContext()->setContextProperty("robotBridge", &robotBridge);
    
    // 注册自定义类型
    qmlRegisterUncreatableType<RobotBridge>("RobotViewer", 1, 0, "RobotBridge",
                                            "RobotBridge cannot be created in QML");
    
    // 注册自定义 OrbitCameraController
    qmlRegisterType<OrbitCameraController>("RobotViewer", 1, 0, "CustomOrbitCameraController");
    
    // 加载主QML文件
    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            qWarning() << "QML加载失败，程序将继续运行但可能不稳定";
            // 不退出程序，让用户看到可能的错误
            return;
        }
        
        qDebug() << "QML界面加载成功";
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    // 检查是否加载成功
    if (engine.rootObjects().isEmpty()) {
        qWarning() << "无法加载QML界面，请检查qml文件是否正确";
        // 仍然运行程序，让用户看到控制台输出
    }
    
    return app.exec();
}
