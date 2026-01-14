#include "settingsmanager.h"
#include <QCoreApplication>
#include <QDir>

SettingsManager& SettingsManager::instance()
{
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager()
    : m_settings(QCoreApplication::applicationDirPath() + "/RobotViewer.ini", QSettings::IniFormat)
{
}

void SettingsManager::setLastUrdfFile(const QString& filePath)
{
    m_settings.setValue("General/LastUrdfFile", filePath);
    m_settings.sync();
}

QString SettingsManager::getLastUrdfFile() const
{
    return m_settings.value("General/LastUrdfFile", "").toString();
}

void SettingsManager::setOpcuaServerUrl(const QString& url)
{
    m_settings.setValue("OPCUA/ServerUrl", url);
    m_settings.sync();
}

QString SettingsManager::getOpcuaServerUrl() const
{
    return m_settings.value("OPCUA/ServerUrl", "opc.tcp://localhost:4840").toString();
}

void SettingsManager::setOpcuaPrefix(const QString &info)
{
    m_settings.setValue("OPCUA/prefix", info);
    m_settings.sync();
}

QString SettingsManager::getOpcuaPrefix() const
{
    return m_settings.value("OPCUA/prefix", "").toString();
}

void SettingsManager::setOpcuaNamespaceIndex(int idx)
{
    m_settings.setValue("OPCUA/NamespaceIndex", idx);
    m_settings.sync();
}

int SettingsManager::getOpcuaNamespaceIndex() const
{
    return m_settings.value("OPCUA/NamespaceIndex", 4).toInt();
}

void SettingsManager::setOpcuaSampleInterval(int ms)
{
    m_settings.setValue("OPCUA/SampleInterval", ms);
    m_settings.sync();
}

int SettingsManager::getOpcuaSampleInterval() const
{
    return m_settings.value("OPCUA/SampleInterval", 100).toInt();
}

void SettingsManager::setOpcuaBindings(const QList<OpcuaBinding>& bindings)
{
    m_settings.beginWriteArray("OPCUA/Bindings");
    for (int i = 0; i < bindings.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("JointName", bindings[i].jointName);
        m_settings.setValue("NodeId", bindings[i].opcuaNodeId);
        m_settings.setValue("Enabled", bindings[i].enabled);
    }
    m_settings.endArray();
    m_settings.sync();
}

QList<OpcuaBinding> SettingsManager::getOpcuaBindings()
{
    QList<OpcuaBinding> bindings;
    int size = m_settings.beginReadArray("OPCUA/Bindings");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        OpcuaBinding binding;
        binding.jointName = m_settings.value("JointName").toString();
        binding.opcuaNodeId = m_settings.value("NodeId").toString();
        binding.enabled = m_settings.value("Enabled", true).toBool();
        bindings.append(binding);
    }
    m_settings.endArray();
    return bindings;
}

void SettingsManager::setShowGrid(bool show)
{
    m_settings.setValue("View/ShowGrid", show);
    m_settings.sync();
}

bool SettingsManager::getShowGrid() const
{
    return m_settings.value("View/ShowGrid", true).toBool();
}

void SettingsManager::setShowAxes(bool show)
{
    m_settings.setValue("View/ShowAxes", show);
    m_settings.sync();
}

bool SettingsManager::getShowAxes() const
{
    return m_settings.value("View/ShowAxes", true).toBool();
}

void SettingsManager::setShowJointAxes(bool show)
{
    m_settings.setValue("View/ShowJointAxes", show);
    m_settings.sync();
}

bool SettingsManager::getShowJointAxes() const
{
    return m_settings.value("View/ShowJointAxes", false).toBool();
}

void SettingsManager::setShowTrajectory(bool show)
{
    m_settings.setValue("View/ShowTrajectory", show);
    m_settings.sync();
}

bool SettingsManager::getShowTrajectory() const
{
    return m_settings.value("View/ShowTrajectory", true).toBool();
}

void SettingsManager::setColoredLinks(bool enabled)
{
    m_settings.setValue("View/ColoredLinks", enabled);
    m_settings.sync();
}

bool SettingsManager::getColoredLinks() const
{
    return m_settings.value("View/ColoredLinks", false).toBool();
}

void SettingsManager::setZUpEnabled(bool enabled)
{
    m_settings.setValue("View/ZUp", enabled);
    m_settings.sync();
}

bool SettingsManager::getZUpEnabled() const
{
    return m_settings.value("View/ZUp", true).toBool();
}

void SettingsManager::setAutoScaleEnabled(bool enabled)
{
    m_settings.setValue("View/AutoScale", enabled);
    m_settings.sync();
}

bool SettingsManager::getAutoScaleEnabled() const
{
    return m_settings.value("View/AutoScale", true).toBool();
}

void SettingsManager::setTrajectoryLifetime(double seconds)
{
    m_settings.setValue("View/TrajectoryLifetime", seconds);
    m_settings.sync();
}

double SettingsManager::getTrajectoryLifetime() const
{
    return m_settings.value("View/TrajectoryLifetime", 2.0).toDouble();
}

void SettingsManager::setEndEffectorConfigs(const QList<EndEffectorConfig>& configs)
{
    m_settings.beginWriteArray("EndEffectors");
    for (int i = 0; i < configs.size(); ++i) {
        m_settings.setArrayIndex(i);
        m_settings.setValue("LinkName", configs[i].linkName);
        m_settings.setValue("DisplayName", configs[i].displayName);
        m_settings.setValue("Color", configs[i].colorHex);
        m_settings.setValue("Enabled", configs[i].enabled);
    }
    m_settings.endArray();
    m_settings.sync();
}

QList<EndEffectorConfig> SettingsManager::getEndEffectorConfigs()
{
    QList<EndEffectorConfig> configs;
    int size = m_settings.beginReadArray("EndEffectors");
    for (int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        EndEffectorConfig config;
        config.linkName = m_settings.value("LinkName").toString();
        config.displayName = m_settings.value("DisplayName").toString();
        config.colorHex = m_settings.value("Color").toString();
        config.enabled = m_settings.value("Enabled", true).toBool();
        configs.append(config);
    }
    m_settings.endArray();
    return configs;
}

void SettingsManager::setWindowGeometry(const QByteArray& geometry)
{
    m_settings.setValue("Window/Geometry", geometry);
    m_settings.sync();
}

QByteArray SettingsManager::getWindowGeometry() const
{
    return m_settings.value("Window/Geometry").toByteArray();
}

void SettingsManager::setWindowState(const QByteArray& state)
{
    m_settings.setValue("Window/State", state);
    m_settings.sync();
}

QByteArray SettingsManager::getWindowState() const
{
    return m_settings.value("Window/State").toByteArray();
}
