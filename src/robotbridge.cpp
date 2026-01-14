#include "robotbridge.h"
#include "robotscene.h"
#include "robotentity.h"
#include "settingsmanager.h"
#include "communication/opcua/opcuaconnector.h"
#include "viewoptions.h"

#include <QFileDialog>
#include <QTimer>
#include <QFileInfo>
#include <QFile>
#include <QtMath>

RobotBridge::RobotBridge(QObject *parent)
    : QObject(parent)
{
    m_statusMessage = tr("就绪，打开URDF文件以加载机器人。");
}

RobotBridge::~RobotBridge()
{
    saveSettings();
    
    if (m_sampleTimer) {
        m_sampleTimer->stop();
        delete m_sampleTimer;
    }
    
    if (m_opcuaConnector) {
        m_opcuaConnector->disconnect();
        delete m_opcuaConnector;
    }
}

void RobotBridge::initialize()
{
    // 创建3D场景（只创建Entity树，不创建窗口）
    m_scene = new RobotScene(this);
    m_scene->initialize();
    m_viewOptions.applyToScene(m_scene);
    
    // 创建OPC UA连接器
    m_opcuaConnector = new OPCUAConnector(this);
    
    // 创建采样定时器
    m_sampleTimer = new QTimer(this);
    connect(m_sampleTimer, &QTimer::timeout, this, &RobotBridge::onSampleTimerTimeout);
    
    // 设置连接
    setupConnections();
    
    // 延迟加载设置
    QTimer::singleShot(100, this, [this]() {
        loadSettings();
    });
}

Qt3DCore::QEntity* RobotBridge::sceneRoot() const
{
    return m_scene ? m_scene->rootEntity() : nullptr;
}

void RobotBridge::attachToSceneRoot(Qt3DCore::QEntity* qmlSceneRoot)
{
    if (m_scene && qmlSceneRoot) {
        m_scene->setSceneRoot(qmlSceneRoot);
        qDebug() << "RobotBridge: C++ scene attached to QML Scene3D";
    }
}

void RobotBridge::setupConnections()
{
    // 场景信号连接
    connect(m_scene, &RobotScene::robotLoaded, this, &RobotBridge::onRobotLoaded);
    connect(m_scene, &RobotScene::loadError, this, &RobotBridge::onLoadError);
    connect(m_scene, &RobotScene::fitCameraRequested, this, &RobotBridge::onFitCameraRequested);
}

void RobotBridge::openURDF()
{
    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        tr("打开URDF文件"),
        m_lastUrdfPath.isEmpty() ? QString() : QFileInfo(m_lastUrdfPath).absolutePath(),
        tr("URDF文件 (*.urdf *.URDF *.xacro);;所有文件 (*.*)")
    );
    
    if (!filename.isEmpty()) {
        loadRobot(filename);
    }
}

void RobotBridge::loadRobot(const QString& filePath)
{
    if (filePath.isEmpty()) return;
    
    m_isLoading = true;
    emit isLoadingChanged();
    
    m_statusMessage = tr("正在加载: %1").arg(filePath);
    emit statusMessageChanged();
    
    if (m_scene->loadRobot(filePath)) {
        m_lastUrdfPath = filePath;
        m_robotName = QFileInfo(filePath).baseName();
        emit robotNameChanged();
        
        m_statusMessage = tr("已加载: %1").arg(filePath);
        emit statusMessageChanged();
    }
    
    m_isLoading = false;
    emit isLoadingChanged();
}

void RobotBridge::onRobotLoaded()
{
    m_robotLoaded = true;
    emit robotLoadedChanged();
    
    updateJointInfoList();
    updateLinkNames();
    
    auto robot = m_scene->robotEntity();
    if (robot) {
        // 连接末端位置信号
        connect(robot, &RobotEntity::endEffectorPositionChanged,
                this, &RobotBridge::onEndEffectorPositionChanged);
        
        // 连接关节值变化信号
        connect(robot, &RobotEntity::jointValueChanged,
                this, &RobotBridge::onJointValueChanged);
    }
    
    // 应用已保存的末端执行器配置
    applyEndEffectorConfigs();
    
    emit showMessage(tr("机器人模型加载成功"), false);
}

void RobotBridge::onLoadError(const QString& error)
{
    m_isLoading = false;
    emit isLoadingChanged();
    
    m_statusMessage = tr("加载错误: %1").arg(error);
    emit statusMessageChanged();
    
    emit showMessage(error, true);
}

void RobotBridge::onFitCameraRequested(const QVector3D& center, const QVector3D& position)
{
    emit fitCameraRequested(center, position);
}

void RobotBridge::updateJointInfoList()
{
    m_jointInfoList.clear();
    m_jointNames.clear();
    
    auto robot = m_scene->robotEntity();
    if (!robot) return;
    
    auto joints = robot->getMovableJoints();
    
    for (const auto& joint : joints) {
        if (!joint->isMovable()) continue;
        
        QVariantMap info;
        info["name"] = joint->name;
        info["value"] = qRadiansToDegrees(robot->getJointValue(joint->name));
        
        // 设置范围
        double lower = joint->limits.lower;
        double upper = joint->limits.upper;
        
        QString type = "R";
        if (joint->type == JointType::Continuous) {
            type = "C";
            lower = -M_PI;
            upper = M_PI;
        } else if (joint->type == JointType::Prismatic) {
            type = "P";
        }
        
        // 对旋转关节转换为度
        if (type != "P") {
            info["min"] = qRadiansToDegrees(lower);
            info["max"] = qRadiansToDegrees(upper);
        } else {
            info["min"] = lower;
            info["max"] = upper;
        }
        
        info["type"] = type;
        
        m_jointInfoList.append(info);
        m_jointNames.append(joint->name);
    }
    
    emit jointInfoListChanged();
    emit jointNamesChanged();
}

void RobotBridge::onJointValueChanged(const QString& jointName, double value)
{
    qDebug() << "onJointValueChanged:" << jointName << value;
    // 更新关节信息列表中的值
    for (int i = 0; i < m_jointInfoList.size(); ++i) {
        QVariantMap info = m_jointInfoList[i].toMap();
        if (info["name"].toString() == jointName) {
            QString type = info["type"].toString();
            double displayValue;
            if (type != "P") {
                displayValue = qRadiansToDegrees(value);
            } else {
                displayValue = value;
            }
            info["value"] = displayValue;
            m_jointInfoList[i] = info;
            // 只发出单个关节更新信号，不重建UI
            qDebug() << "  emitting jointValueUpdated:" << jointName << displayValue;
            emit jointValueUpdated(jointName, displayValue);
            break;
        }
    }
}

void RobotBridge::onEndEffectorPositionChanged(const QVector3D& position)
{
    m_endEffectorPosition = position;
    emit endEffectorPositionChanged();
}

// 相机控制
void RobotBridge::resetCamera()
{
    if (m_scene) {
        m_scene->resetCamera();
    }
    emit resetCameraRequested();
}

void RobotBridge::fitCamera()
{
    if (m_scene) {
        m_scene->fitCameraToRobot();
    }
}

// 关节控制
void RobotBridge::setJointValue(const QString& jointName, double value)
{
    auto robot = this->robot();
    if (!robot) return;
    
    // 查找关节类型来决定是否需要转换
    auto joints = robot->getMovableJoints();
    for (const auto& joint : joints) {
        if (joint->name == jointName) {
            double radValue = value;
            if (joint->type != JointType::Prismatic) {
                radValue = qDegreesToRadians(value);
            }
            
            QMap<QString, double> vals;
            vals.insert(jointName, radValue);
            robot->setJointValues(vals);
            break;
        }
    }
}

void RobotBridge::resetAllJoints()
{
    auto robot = this->robot();
    if (!robot) return;
    
    QMap<QString, double> vals;
    auto joints = robot->getMovableJoints();
    for (const auto& joint : joints) {
        vals.insert(joint->name, 0.0);
    }
    robot->setJointValues(vals);
    
    updateJointInfoList();
}

// 末端执行器配置
void RobotBridge::updateLinkNames()
{
    m_linkNames.clear();
    
    auto robot = this->robot();
    if (!robot || !robot->getModel()) return;
    
    auto model = robot->getModel();
    for (auto it = model->links.constBegin(); it != model->links.constEnd(); ++it) {
        m_linkNames.append(it.key());
    }
    
    m_linkNames.sort();
    emit linkNamesChanged();
}

RobotEntity* RobotBridge::robot() const
{
    return m_scene ? m_scene->robotEntity() : nullptr;
}

void RobotBridge::addEndEffectorConfig(const QString& linkName,
                                        const QString& displayName,
                                        const QString& colorHex)
{
    QString error;
    if (!m_endEffectorConfigs.addConfig(linkName, displayName, colorHex, &error)) {
        emit showMessage(error, true);
        return;
    }
    emit endEffectorConfigsChanged();
}

void RobotBridge::removeEndEffectorConfig(int index)
{
    m_endEffectorConfigs.removeConfig(index);
    emit endEffectorConfigsChanged();
    
    // 立即应用更改
    applyEndEffectorConfigs();
}

void RobotBridge::updateEndEffectorConfig(int index, const QString& linkName,
                                           const QString& displayName,
                                           const QString& colorHex, bool enabled)
{
    m_endEffectorConfigs.updateConfig(index, linkName, displayName, colorHex, enabled);
    emit endEffectorConfigsChanged();
}

void RobotBridge::applyEndEffectorConfigs()
{
    if (!m_scene) return;
    
    // 清除现有的末端执行器轨迹
    m_scene->clearEndEffectorTrajectories();
    
    // 添加配置中启用的末端执行器
    for (const auto& config : m_endEffectorConfigs.rows()) {
        if (!config.enabled) continue;
        
        QColor color(config.colorHex);
        if (!color.isValid()) {
            color = Qt::yellow;
        }
        
        m_scene->addEndEffectorTrajectory(config.linkName, config.displayName, color);
    }
    
    emit showMessage(tr("末端执行器配置已应用"), false);
}

// 视图选项 Setters
void RobotBridge::setShowGrid(bool show)
{
    if (!m_viewOptions.setShowGrid(show, m_scene)) return;
    emit showGridChanged();
}

void RobotBridge::setShowAxes(bool show)
{
    if (!m_viewOptions.setShowAxes(show, m_scene)) return;
    emit showAxesChanged();
}

void RobotBridge::setShowJointAxes(bool show)
{
    if (!m_viewOptions.setShowJointAxes(show, m_scene)) return;
    emit showJointAxesChanged();
}

void RobotBridge::setColoredLinks(bool colored)
{
    if (!m_viewOptions.setColoredLinks(colored, m_scene)) return;
    emit coloredLinksChanged();
}

void RobotBridge::setZUpEnabled(bool enabled)
{
    if (!m_viewOptions.setZUpEnabled(enabled, m_scene)) return;
    emit zUpEnabledChanged();
}

void RobotBridge::setAutoScaleEnabled(bool enabled)
{
    if (!m_viewOptions.setAutoScaleEnabled(enabled, m_scene)) return;
    emit autoScaleEnabledChanged();
}

void RobotBridge::setShowTrajectory(bool show)
{
    if (!m_viewOptions.setShowTrajectory(show, m_scene)) return;
    emit showTrajectoryChanged();
}

void RobotBridge::setTrajectoryLifetime(double seconds)
{
    if (!m_viewOptions.setTrajectoryLifetime(seconds, m_scene)) return;
    emit trajectoryLifetimeChanged();
}

// OPC UA Setters
void RobotBridge::setOpcuaServerUrl(const QString& url)
{
    if (m_opcuaServerUrl == url) return;
    m_opcuaServerUrl = url;
    emit opcuaServerUrlChanged();
}

void RobotBridge::setOpcuaPrefix(const QString& prefix)
{
    if (m_opcuaPrefix == prefix) return;
    m_opcuaPrefix = prefix;
    emit opcuaPrefixChanged();
}

void RobotBridge::setOpcuaSampleInterval(int ms)
{
    if (m_opcuaSampleInterval == ms) return;
    m_opcuaSampleInterval = ms;
    if (m_sampleTimer && m_sampleTimer->isActive()) {
        m_sampleTimer->setInterval(ms);
    }
    emit opcuaSampleIntervalChanged();
}

void RobotBridge::setOpcuaNamespace(int ns)
{
    if (m_opcuaNamespace == ns) return;
    m_opcuaNamespace = ns;
    emit opcuaNamespaceChanged();
}

// OPC UA 操作
void RobotBridge::opcuaConnect()
{
    if (!m_opcuaConnector) return;
    
    QStringList paramList;
    paramList << m_opcuaServerUrl << m_opcuaPrefix;
    if (m_opcuaConnector->init(paramList) == 0) {
        m_opcuaConnected = true;
        emit opcuaConnectedChanged();
        emit showMessage(tr("OPC UA 连接成功"), false);
    } else {
        emit showMessage(tr("OPC UA 连接失败"), true);
    }
}

void RobotBridge::opcuaDisconnect()
{
    if (!m_opcuaConnector) return;
    
    opcuaStopSampling();
    m_opcuaConnector->disconnect();
    m_opcuaConnected = false;
    emit opcuaConnectedChanged();
    emit showMessage(tr("OPC UA 已断开"), false);
}

void RobotBridge::opcuaStartSampling()
{
    if (!m_opcuaConnected || !m_sampleTimer) return;
    
    m_sampleTimer->start(m_opcuaSampleInterval);
    m_opcuaSampling = true;
    emit opcuaSamplingChanged();
}

void RobotBridge::opcuaStopSampling()
{
    if (m_sampleTimer) {
        m_sampleTimer->stop();
    }
    m_opcuaSampling = false;
    emit opcuaSamplingChanged();
}

void RobotBridge::onSampleTimerTimeout()
{
    if (!m_opcuaConnector || !m_opcuaConnected) return;
    
    auto robot = this->robot();
    if (!robot) return;
    
    QMap<QString, double> jointValues;
    
    // qDebug() << "OPC UA sampling, bindings count:" << m_opcuaBindings.rows().length();
    
    for (const auto& binding : m_opcuaBindings.rows()) {
        if (!binding.enabled) {
            qDebug() << "  Binding disabled:" << binding.jointName;
            continue;
        }
        
        const QString& jointName = binding.jointName;
        const QString& nodeId = binding.nodeId;
        
        if (jointName.isEmpty() || nodeId.isEmpty()) {
            qDebug() << "  Binding empty:" << jointName << nodeId;
            continue;
        }
        
        m_opcuaConnector->setNamespaceIndex(m_opcuaNamespace);
        QVariant var;
        if (m_opcuaConnector->readValue(nodeId, var) == 0) {
            double degValue = var.toDouble();
            double radValue = qDegreesToRadians(degValue);
            jointValues[jointName] = radValue;
            // qDebug() << "  Read:" << nodeId << "=" << degValue << "deg (" << radValue << "rad)";
        } else {
            // qDebug() << "  Failed to read:" << nodeId;
        }
    }

    if (!jointValues.isEmpty()) {
        // setJointValues 会触发 jointValueChanged 信号
        // jointValueChanged 信号会被 onJointValueChanged 处理
        // onJointValueChanged 会发出 jointValueUpdated 信号给 QML
        robot->setJointValues(jointValues);
    }
}

void RobotBridge::addOpcuaBinding()
{
    m_opcuaBindings.addBinding(m_jointNames);
    emit opcuaBindingsChanged();
}

void RobotBridge::removeOpcuaBinding(int index)
{
    m_opcuaBindings.removeBinding(index);
    emit opcuaBindingsChanged();
}

void RobotBridge::updateOpcuaBinding(int index, const QString& jointName, 
                                     const QString& nodeId, bool enabled)
{
    m_opcuaBindings.updateBinding(index, jointName, nodeId, enabled);
    emit opcuaBindingsChanged();
}

// 设置管理
void RobotBridge::loadSettings()
{
    SettingsManager& settings = SettingsManager::instance();
    
    // 加载上次的URDF路径
    m_lastUrdfPath = settings.getLastUrdfFile();
    if (!m_lastUrdfPath.isEmpty() && QFile::exists(m_lastUrdfPath)) {
        loadRobot(m_lastUrdfPath);
    }
    
    // 加载视图选项
    setShowGrid(settings.getShowGrid());
    setShowAxes(settings.getShowAxes());
    setShowJointAxes(settings.getShowJointAxes());
    setColoredLinks(settings.getColoredLinks());
    setZUpEnabled(settings.getZUpEnabled());
    setAutoScaleEnabled(settings.getAutoScaleEnabled());
    setShowTrajectory(settings.getShowTrajectory());
    setTrajectoryLifetime(settings.getTrajectoryLifetime());
    
    // 加载OPC UA设置
    setOpcuaServerUrl(settings.getOpcuaServerUrl());
    setOpcuaPrefix(settings.getOpcuaPrefix());
    setOpcuaSampleInterval(settings.getOpcuaSampleInterval());
    setOpcuaNamespace(settings.getOpcuaNamespaceIndex());
    
    // 加载OPC UA绑定
    m_opcuaBindings.fromSettings(settings.getOpcuaBindings());
    emit opcuaBindingsChanged();
    
    // 加载末端执行器配置
    m_endEffectorConfigs.fromSettings(settings.getEndEffectorConfigs());
    emit endEffectorConfigsChanged();
}

void RobotBridge::saveSettings()
{
    SettingsManager& settings = SettingsManager::instance();
    
    // 保存上次的URDF路径
    settings.setLastUrdfFile(m_lastUrdfPath);
    
    // 保存视图选项
    m_viewOptions.saveToSettings(settings);
    
    // 保存OPC UA设置
    settings.setOpcuaServerUrl(m_opcuaServerUrl);
    settings.setOpcuaPrefix(m_opcuaPrefix);
    settings.setOpcuaSampleInterval(m_opcuaSampleInterval);
    settings.setOpcuaNamespaceIndex(m_opcuaNamespace);
    
    // 保存OPC UA绑定
    settings.setOpcuaBindings(m_opcuaBindings.toSettings());
    
    // 保存末端执行器配置
    settings.setEndEffectorConfigs(m_endEffectorConfigs.toSettings());
}
