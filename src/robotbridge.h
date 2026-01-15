#ifndef ROBOTBRIDGE_H
#define ROBOTBRIDGE_H

#include <QObject>
#include <QVector3D>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QUrl>
#include <Qt3DCore/QEntity>

#include "commontypes.h"
#include "viewoptions.h"
#include "opcuabindingmodel.h"
#include "endeffectorconfigmodel.h"

#pragma execution_character_set("utf-8")

class RobotScene;
class RobotEntity;
class OPCUAConnector;
class QTimer;

/**
 * @brief RobotBridge - QML与C++机器人逻辑的桥接类
 * 
 * 这个类封装了所有机器人场景的属性和方法，
 * 使其能够被QML直接访问和控制。
 */
class RobotBridge : public QObject
{
    Q_OBJECT
    
    // 版本信息
    Q_PROPERTY(QString version READ version CONSTANT)
    
    // 3D场景根实体 - 用于将C++创建的Entity挂载到QML Scene3D
    Q_PROPERTY(Qt3DCore::QEntity* sceneRoot READ sceneRoot CONSTANT)
    
    // 机器人信息
    Q_PROPERTY(QString robotName READ robotName NOTIFY robotNameChanged)
    Q_PROPERTY(bool robotLoaded READ robotLoaded NOTIFY robotLoadedChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    
    // 末端执行器位置
    Q_PROPERTY(QVector3D endEffectorPosition READ endEffectorPosition NOTIFY endEffectorPositionChanged)
    
    // 关节信息列表
    Q_PROPERTY(QVariantList jointInfoList READ jointInfoList NOTIFY jointInfoListChanged)
    Q_PROPERTY(QStringList jointNames READ jointNames NOTIFY jointNamesChanged)
    
    // ???????
    Q_PROPERTY(QStringList linkNames READ linkNames NOTIFY linkNamesChanged)
    Q_PROPERTY(QVariantList endEffectorConfigs READ endEffectorConfigs NOTIFY endEffectorConfigsChanged)
    
    // 视图选项
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    Q_PROPERTY(bool showAxes READ showAxes WRITE setShowAxes NOTIFY showAxesChanged)
    Q_PROPERTY(bool showJointAxes READ showJointAxes WRITE setShowJointAxes NOTIFY showJointAxesChanged)
    Q_PROPERTY(bool coloredLinks READ coloredLinks WRITE setColoredLinks NOTIFY coloredLinksChanged)
    Q_PROPERTY(bool zUpEnabled READ zUpEnabled WRITE setZUpEnabled NOTIFY zUpEnabledChanged)
    Q_PROPERTY(bool autoScaleEnabled READ autoScaleEnabled WRITE setAutoScaleEnabled NOTIFY autoScaleEnabledChanged)
    Q_PROPERTY(bool showTrajectory READ showTrajectory WRITE setShowTrajectory NOTIFY showTrajectoryChanged)
    Q_PROPERTY(double trajectoryLifetime READ trajectoryLifetime WRITE setTrajectoryLifetime NOTIFY trajectoryLifetimeChanged)
    
    // OPC UA属性
    Q_PROPERTY(QString opcuaServerUrl READ opcuaServerUrl WRITE setOpcuaServerUrl NOTIFY opcuaServerUrlChanged)
    Q_PROPERTY(QString opcuaPrefix READ opcuaPrefix WRITE setOpcuaPrefix NOTIFY opcuaPrefixChanged)
    Q_PROPERTY(int opcuaSampleInterval READ opcuaSampleInterval WRITE setOpcuaSampleInterval NOTIFY opcuaSampleIntervalChanged)
    Q_PROPERTY(int opcuaNamespace READ opcuaNamespace WRITE setOpcuaNamespace NOTIFY opcuaNamespaceChanged)
    Q_PROPERTY(bool opcuaConnected READ opcuaConnected NOTIFY opcuaConnectedChanged)
    Q_PROPERTY(bool opcuaSampling READ opcuaSampling NOTIFY opcuaSamplingChanged)
    Q_PROPERTY(QVariantList opcuaBindings READ opcuaBindings NOTIFY opcuaBindingsChanged)
    
public:
    explicit RobotBridge(QObject *parent = nullptr);
    ~RobotBridge();
    
    // 初始化场景
    void initialize();
    
    // 获取场景对象（用于Qt3D集成）
    RobotScene* scene() const { return m_scene; }
    
    // 获取3D场景根实体（用于QML Scene3D挂载）
    Qt3DCore::QEntity* sceneRoot() const;
    
    // 版本
    QString version() const { return "0.1.0"; }
    
    // 机器人信息
    QString robotName() const { return m_robotName; }
    bool robotLoaded() const { return m_robotLoaded; }
    bool isLoading() const { return m_isLoading; }
    QString statusMessage() const { return m_statusMessage; }
    
    // 末端位置
    QVector3D endEffectorPosition() const { return m_endEffectorPosition; }
    
    // 关节信息
    QVariantList jointInfoList() const { return m_jointInfoList; }
    QStringList jointNames() const { return m_jointNames; }
    
    // 末端执行器配置
    QStringList linkNames() const { return m_linkNames; }
    QVariantList endEffectorConfigs() const { return m_endEffectorConfigs.toVariantList(); }
    
    // 视图选项 Getters
    bool showGrid() const { return m_viewOptions.state().showGrid; }
    bool showAxes() const { return m_viewOptions.state().showAxes; }
    bool showJointAxes() const { return m_viewOptions.state().showJointAxes; }
    bool coloredLinks() const { return m_viewOptions.state().coloredLinks; }
    bool zUpEnabled() const { return m_viewOptions.state().zUpEnabled; }
    bool autoScaleEnabled() const { return m_viewOptions.state().autoScaleEnabled; }
    bool showTrajectory() const { return m_viewOptions.state().showTrajectory; }
    double trajectoryLifetime() const { return m_viewOptions.state().trajectoryLifetime; }
    
    // 视图选项 Setters
    void setShowGrid(bool show);
    void setShowAxes(bool show);
    void setShowJointAxes(bool show);
    void setColoredLinks(bool colored);
    void setZUpEnabled(bool enabled);
    void setAutoScaleEnabled(bool enabled);
    void setShowTrajectory(bool show);
    void setTrajectoryLifetime(double seconds);
    
    // OPC UA Getters
    QString opcuaServerUrl() const { return m_opcuaServerUrl; }
    QString opcuaPrefix() const { return m_opcuaPrefix; }
    int opcuaSampleInterval() const { return m_opcuaSampleInterval; }
    int opcuaNamespace() const { return m_opcuaNamespace; }
    bool opcuaConnected() const { return m_opcuaConnected; }
    bool opcuaSampling() const { return m_opcuaSampling; }
    QVariantList opcuaBindings() const { return m_opcuaBindings.toVariantList(); }
    
    // OPC UA Setters
    void setOpcuaServerUrl(const QString& url);
    void setOpcuaPrefix(const QString& prefix);
    void setOpcuaSampleInterval(int ms);
    void setOpcuaNamespace(int ns);
    
public slots:
    // 文件操作
    void openURDF();
    void loadRobot(const QString& filePath);
    
    // 相机控制
    Q_INVOKABLE void resetCamera();
    Q_INVOKABLE void fitCamera();
    
    // 将C++场景挂载到QML的Scene3D根节点
    Q_INVOKABLE void attachToSceneRoot(Qt3DCore::QEntity* qmlSceneRoot);
    
    // 关节控制
    void setJointValue(const QString& jointName, double value);
    void resetAllJoints();
    
    // 末端执行器配置
    Q_INVOKABLE void addEndEffectorConfig(const QString& linkName, 
                                          const QString& displayName = QString(),
                                          const QString& colorHex = QString());
    Q_INVOKABLE void removeEndEffectorConfig(int index);
    Q_INVOKABLE void updateEndEffectorConfig(int index, const QString& linkName,
                                              const QString& displayName,
                                              const QString& colorHex, bool enabled);
    Q_INVOKABLE void applyEndEffectorConfigs();
    
    // OPC UA 操作
    void opcuaConnect();
    void opcuaDisconnect();
    void opcuaStartSampling();
    void opcuaStopSampling();
    void addOpcuaBinding();
    void removeOpcuaBinding(int index);
    void updateOpcuaBinding(int index, const QString& jointName, 
                           const QString& nodeId, bool enabled);
    
signals:
    // 机器人状态信号
    void robotNameChanged();
    void robotLoadedChanged();
    void isLoadingChanged();
    void statusMessageChanged();
    void endEffectorPositionChanged();
    void jointInfoListChanged();
    void jointNamesChanged();
    void jointValueUpdated(const QString& jointName, double value);  // 单个关节值更新
    
    // 末端执行器信号
    void linkNamesChanged();
    void endEffectorConfigsChanged();
    
    // 视图选项信号
    void showGridChanged();
    void showAxesChanged();
    void showJointAxesChanged();
    void coloredLinksChanged();
    void zUpEnabledChanged();
    void autoScaleEnabledChanged();
    void showTrajectoryChanged();
    void trajectoryLifetimeChanged();
    
    // OPC UA信号
    void opcuaServerUrlChanged();
    void opcuaPrefixChanged();
    void opcuaSampleIntervalChanged();
    void opcuaNamespaceChanged();
    void opcuaConnectedChanged();
    void opcuaSamplingChanged();
    void opcuaBindingsChanged();
    
    // 消息提示信号
    void showMessage(const QString& message, bool isError);
    
    // 相机控制信号（发送给QML）
    void resetCameraRequested();
    void fitCameraRequested(const QVector3D& center, const QVector3D& position);
    
private slots:
    void onRobotLoaded();
    void onLoadError(const QString& error);
    void onJointValueChanged(const QString& jointName, double value);
    void onEndEffectorPositionChanged(const QVector3D& position);
    void onSampleTimerTimeout();
    
private:
    void updateJointInfoList();
    void loadSettings();
    void saveSettings();
    void setupConnections();
    void updateLinkNames();
    RobotEntity* robot() const;
    
    // 场景
    RobotScene* m_scene = nullptr;
    
    // 状态
    QString m_robotName;
    bool m_robotLoaded = false;
    bool m_isLoading = false;
    QString m_statusMessage;
    QString m_lastUrdfPath;
    
    // 末端位置
    QVector3D m_endEffectorPosition;
    
    // 关节信息
    QVariantList m_jointInfoList;
    QStringList m_jointNames;
    
    // 末端执行器配置
    QStringList m_linkNames;
    EndEffectorConfigModel m_endEffectorConfigs;
    
    // 视图选项
    ViewOptions m_viewOptions;
    
    // OPC UA
    OPCUAConnector* m_opcuaConnector = nullptr;
    QTimer* m_sampleTimer = nullptr;
    QString m_opcuaServerUrl = "opc.tcp://localhost:4840";
    QString m_opcuaPrefix;
    int m_opcuaSampleInterval = 100;
    int m_opcuaNamespace = 2;
    bool m_opcuaConnected = false;
    bool m_opcuaSampling = false;
    OpcuaBindingModel m_opcuaBindings;
};

#endif // ROBOTBRIDGE_H
