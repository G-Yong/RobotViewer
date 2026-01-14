#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#pragma execution_character_set("utf-8")

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QList>
#include "commontypes.h"

/**
 * @brief 设置管理器
 * 负责保存和加载应用程序设置到INI文件
 */
class SettingsManager
{
public:
    static SettingsManager& instance();

    /**
     * @brief 保存上次打开的URDF文件路径
     */
    void setLastUrdfFile(const QString& filePath);
    QString getLastUrdfFile() const;

    /**
     * @brief 保存/加载OPC UA服务器设置
     */
    void setOpcuaServerUrl(const QString& url);
    QString getOpcuaServerUrl() const;

    void setOpcuaPrefix(const QString& info);
    QString getOpcuaPrefix() const;

    void setOpcuaNamespaceIndex(int idx);
    int getOpcuaNamespaceIndex() const;

    void setOpcuaSampleInterval(int ms);
    int getOpcuaSampleInterval() const;

    /**
     * @brief 保存/加载OPC UA变量绑定
     */
    void setOpcuaBindings(const QList<OpcuaBinding>& bindings);
    QList<OpcuaBinding> getOpcuaBindings();

    /**
     * @brief 保存/加载视图选项
     */
    void setShowGrid(bool show);
    bool getShowGrid() const;

    void setShowAxes(bool show);
    bool getShowAxes() const;

    void setShowJointAxes(bool show);
    bool getShowJointAxes() const;

    void setShowTrajectory(bool show);
    bool getShowTrajectory() const;

    void setColoredLinks(bool enabled);
    bool getColoredLinks() const;

    void setZUpEnabled(bool enabled);
    bool getZUpEnabled() const;

    void setAutoScaleEnabled(bool enabled);
    bool getAutoScaleEnabled() const;

    void setTrajectoryLifetime(double seconds);
    double getTrajectoryLifetime() const;

    /**
     * @brief 保存/加载末端执行器配置
     */
    void setEndEffectorConfigs(const QList<EndEffectorConfig>& configs);
    QList<EndEffectorConfig> getEndEffectorConfigs();

    /**
     * @brief 保存/加载窗口几何信息
     */
    void setWindowGeometry(const QByteArray& geometry);
    QByteArray getWindowGeometry() const;

    void setWindowState(const QByteArray& state);
    QByteArray getWindowState() const;

private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H
