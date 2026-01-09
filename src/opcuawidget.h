#ifndef OPCUAWIDGET_H
#define OPCUAWIDGET_H

#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTimer>
#include <QComboBox>
#include <QLabel>
#include <QMap>
#include <memory>

class OPCUAConnector;
class RobotEntity;

/**
 * @brief OPC UA变量绑定项
 */
struct OpcuaBinding {
    QString jointName;      // 关节名称
    QString opcuaNodeId;    // OPC UA节点ID
    bool enabled = true;    // 是否启用
};

/**
 * @brief OPC UA通讯控制面板
 * 用于配置OPC UA连接和变量绑定
 */
class OpcuaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OpcuaWidget(QWidget* parent = nullptr);
    ~OpcuaWidget();

    /**
     * @brief 设置关节列表
     */
    void setJointNames(const QStringList& jointNames);

    /**
     * @brief 获取所有绑定配置
     */
    QList<OpcuaBinding> getBindings() const;

    /**
     * @brief 设置绑定配置
     */
    void setBindings(const QList<OpcuaBinding>& bindings);

    /**
     * @brief 获取服务器地址
     */
    QString getServerUrl() const;

    /**
     * @brief 设置服务器地址
     */
    void setServerUrl(const QString& url);

    /**
     * @brief 获取前缀
     */
    QString getPrefix() const;

    /**
     * @brief 前缀
     */
    void setPrefix(const QString& info);

    /**
     * @brief 获取采样周期
     */
    int getSampleInterval() const;

    /**
     * @brief 设置采样周期
     */
    void setSampleInterval(int ms);

    /**
     * @brief 获取命名空间索引
     */
    int getNamespaceIndex() const;

    /**
     * @brief 设置命名空间索引
     */
    void setNamespaceIndex(int idx);

signals:
    /**
     * @brief 设置变化信号
     */
    void settingsChanged();

    /**
     * @brief 连接状态变化信号
     */
    void connectionStatusChanged(bool connected, const QString& message);

    /**
     * @brief 关节值更新信号
     */
    void jointValueUpdated(QMap<QString, double> vals);

public slots:
    /**
     * @brief 开始采样
     */
    void startSampling();

    /**
     * @brief 停止采样
     */
    void stopSampling();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onStartStopClicked();
    void onAddBindingClicked();
    void onRemoveBindingClicked();
    void onSampleTimerTimeout();
    void onBindingCellChanged(int row, int column);

private:
    void setupUI();
    void updateButtonStates();
    void readOpcuaValues();

    // UI组件
    QLineEdit* m_serverUrlEdit = nullptr;
    QLineEdit* m_prefixEdit = nullptr;
    QSpinBox* m_namespaceSpinBox = nullptr;
    QSpinBox* m_sampleIntervalSpinBox = nullptr;
    QPushButton* m_connectButton = nullptr;
    QPushButton* m_disconnectButton = nullptr;
    QPushButton* m_startStopButton = nullptr;
    QPushButton* m_addBindingButton = nullptr;
    QPushButton* m_removeBindingButton = nullptr;
    QTableWidget* m_bindingTable = nullptr;
    QLabel* m_statusLabel = nullptr;

    // OPC UA连接器
    OPCUAConnector* m_connector = nullptr;

    // 采样定时器
    QTimer* m_sampleTimer = nullptr;

    // 关节名称列表
    QStringList m_jointNames;

    // 连接状态
    bool m_connected = false;
    bool m_sampling = false;

    QDialog *mBindingDialog = nullptr;
};

#endif // OPCUAWIDGET_H
