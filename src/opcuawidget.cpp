#include "opcuawidget.h"
#include "robotentity.h"
#include "opcua/opcuaconnector.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>
#include <QtMath>
#include <QElapsedTimer>
#include <QtConcurrentRun>

OpcuaWidget::OpcuaWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    // 创建OPC UA连接器
    m_connector = new OPCUAConnector(this);
    connect(m_connector, &OPCUAConnector::stateChanged, this, [=](){
        if(m_connector->state() == 1)
        {
            m_connected = true;
            return;
        }

        m_connected = false;
        m_statusLabel->setText(tr("状态: 连接断开"));
        m_statusLabel->setStyleSheet("color: red;");
        updateButtonStates();
    });

    // 创建采样定时器
    m_sampleTimer = new QTimer(this);
    connect(m_sampleTimer, &QTimer::timeout,
            this, &OpcuaWidget::onSampleTimerTimeout);

    updateButtonStates();

    qRegisterMetaType<QMap<QString,double>>("QMap<QString,double>");
}

OpcuaWidget::~OpcuaWidget()
{
    stopSampling();
    if (m_connector) {
        m_connector->release();
    }
}

void OpcuaWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 连接设置组
    QGroupBox* connectionGroup = new QGroupBox(tr("OPC UA连接"), this);
    QVBoxLayout* connectionLayout = new QVBoxLayout(connectionGroup);

    // 服务器地址
    QHBoxLayout* serverLayout = new QHBoxLayout();
    QLabel* serverLabel = new QLabel(tr("服务器地址:"), this);
    m_serverUrlEdit = new QLineEdit(this);
    m_serverUrlEdit->setPlaceholderText("opc.tcp://localhost:4840");
    m_serverUrlEdit->setText("opc.tcp://localhost:4840");
    serverLayout->addWidget(serverLabel);
    serverLayout->addWidget(m_serverUrlEdit, 1);
    connectionLayout->addLayout(serverLayout);

    // 前缀
    QHBoxLayout* prefixLayout = new QHBoxLayout();
    QLabel* prefixLabel = new QLabel(tr("前缀:"), this);
    m_prefixEdit = new QLineEdit(this);
    m_prefixEdit->setPlaceholderText("");
    m_prefixEdit->setText("");
    prefixLayout->addWidget(prefixLabel);
    prefixLayout->addWidget(m_prefixEdit, 1);
    connectionLayout->addLayout(prefixLayout);

    // 命名空间索引
    QHBoxLayout* nsLayout = new QHBoxLayout();
    QLabel* nsLabel = new QLabel(tr("命名空间索引:"), this);
    m_namespaceSpinBox = new QSpinBox(this);
    m_namespaceSpinBox->setRange(0, 100);
    m_namespaceSpinBox->setValue(4);
    nsLayout->addWidget(nsLabel);
    nsLayout->addWidget(m_namespaceSpinBox);
    nsLayout->addStretch();
    connectionLayout->addLayout(nsLayout);

    // 连接按钮
    QHBoxLayout* connectButtonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton(tr("连接"), this);
    m_disconnectButton = new QPushButton(tr("断开"), this);
    connect(m_connectButton, &QPushButton::clicked, this, &OpcuaWidget::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &OpcuaWidget::onDisconnectClicked);
    connectButtonLayout->addWidget(m_connectButton);
    connectButtonLayout->addWidget(m_disconnectButton);
    connectButtonLayout->addStretch();
    connectionLayout->addLayout(connectButtonLayout);

    // 状态标签
    m_statusLabel = new QLabel(tr("状态: 未连接"), this);
    m_statusLabel->setStyleSheet("color: gray;");
    connectionLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(connectionGroup);

    // 采样设置组
    QGroupBox* samplingGroup = new QGroupBox(tr("采样"), this);
    QHBoxLayout* samplingLayout = new QHBoxLayout(samplingGroup);

    QLabel* intervalLabel = new QLabel(tr("间隔:"), this);
    m_sampleIntervalSpinBox = new QSpinBox(this);
    m_sampleIntervalSpinBox->setRange(10, 10000);
    m_sampleIntervalSpinBox->setValue(100);
    m_sampleIntervalSpinBox->setSuffix(" ms");

    m_startStopButton = new QPushButton(tr("开始"), this);
    connect(m_startStopButton, &QPushButton::clicked, this, &OpcuaWidget::onStartStopClicked);

    samplingLayout->addWidget(intervalLabel);
    samplingLayout->addWidget(m_sampleIntervalSpinBox);
    samplingLayout->addWidget(m_startStopButton);
    samplingLayout->addStretch();

    mainLayout->addWidget(samplingGroup);

    // 变量绑定组
    QGroupBox* bindingGroup = new QGroupBox(tr("变量绑定"), this);
    QVBoxLayout* bindingLayout = new QVBoxLayout(bindingGroup);

    // 绑定表格
    m_bindingTable = new QTableWidget(this);
    m_bindingTable->setColumnCount(3);
    m_bindingTable->setHorizontalHeaderLabels({tr("启用"), tr("关节名"), tr("OPC UA节点ID")});
    m_bindingTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_bindingTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_bindingTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_bindingTable->setColumnWidth(0, 60);
    m_bindingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bindingTable->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_bindingTable, &QTableWidget::cellChanged, this, &OpcuaWidget::onBindingCellChanged);
    bindingLayout->addWidget(m_bindingTable);

    // 绑定操作按钮
    QHBoxLayout* bindingButtonLayout = new QHBoxLayout();
    m_addBindingButton = new QPushButton(tr("添加绑定"), this);
    m_removeBindingButton = new QPushButton(tr("移除绑定"), this);
    connect(m_addBindingButton, &QPushButton::clicked, this, &OpcuaWidget::onAddBindingClicked);
    connect(m_removeBindingButton, &QPushButton::clicked, this, &OpcuaWidget::onRemoveBindingClicked);
    bindingButtonLayout->addWidget(m_addBindingButton);
    bindingButtonLayout->addWidget(m_removeBindingButton);
    bindingButtonLayout->addStretch();
    bindingLayout->addLayout(bindingButtonLayout);

    mainLayout->addWidget(bindingGroup, 1);
}

// void OpcuaWidget::setRobotEntity(RobotEntity* robot)
// {
//     m_robotEntity = robot;
// }

void OpcuaWidget::setJointNames(const QStringList& jointNames)
{
    m_jointNames = jointNames;
}

QString OpcuaWidget::getServerUrl() const
{
    return m_serverUrlEdit->text();
}

void OpcuaWidget::setServerUrl(const QString& url)
{
    m_serverUrlEdit->setText(url);
}

QString OpcuaWidget::getPrefix() const
{
    return m_prefixEdit->text();
}

void OpcuaWidget::setPrefix(const QString &info)
{
    m_prefixEdit->setText(info);
}

int OpcuaWidget::getSampleInterval() const
{
    return m_sampleIntervalSpinBox->value();
}

void OpcuaWidget::setSampleInterval(int ms)
{
    m_sampleIntervalSpinBox->setValue(ms);
}

int OpcuaWidget::getNamespaceIndex() const
{
    return m_namespaceSpinBox->value();
}

void OpcuaWidget::setNamespaceIndex(int idx)
{
    m_namespaceSpinBox->setValue(idx);
}

QList<OpcuaBinding> OpcuaWidget::getBindings() const
{
    QList<OpcuaBinding> bindings;
    for (int row = 0; row < m_bindingTable->rowCount(); ++row) {
        OpcuaBinding binding;

        // Enabled checkbox
        QWidget* checkWidget = m_bindingTable->cellWidget(row, 0);
        if (checkWidget) {
            QCheckBox* checkBox = checkWidget->findChild<QCheckBox*>();
            if (checkBox) {
                binding.enabled = checkBox->isChecked();
            }
        }

        // Joint name (from combo box)
        QComboBox* jointCombo = qobject_cast<QComboBox*>(m_bindingTable->cellWidget(row, 1));
        if (jointCombo) {
            binding.jointName = jointCombo->currentText();
        }

        // OPC UA node ID
        QTableWidgetItem* nodeItem = m_bindingTable->item(row, 2);
        if (nodeItem) {
            binding.opcuaNodeId = nodeItem->text();
        }

        if (!binding.jointName.isEmpty()) {
            bindings.append(binding);
        }
    }
    return bindings;
}

void OpcuaWidget::setBindings(const QList<OpcuaBinding>& bindings)
{
    m_bindingTable->blockSignals(true);
    m_bindingTable->setRowCount(0);

    for (const auto& binding : bindings) {
        int row = m_bindingTable->rowCount();
        m_bindingTable->insertRow(row);

        // Enabled checkbox
        QWidget* checkWidget = new QWidget();
        QHBoxLayout* checkLayout = new QHBoxLayout(checkWidget);
        checkLayout->setAlignment(Qt::AlignCenter);
        checkLayout->setContentsMargins(0, 0, 0, 0);
        QCheckBox* checkBox = new QCheckBox();
        checkBox->setChecked(binding.enabled);
        checkLayout->addWidget(checkBox);
        m_bindingTable->setCellWidget(row, 0, checkWidget);

        // Joint name combo box
        QComboBox* jointCombo = new QComboBox();
        jointCombo->addItems(m_jointNames);
        int idx = jointCombo->findText(binding.jointName);
        if (idx >= 0) {
            jointCombo->setCurrentIndex(idx);
        }
        m_bindingTable->setCellWidget(row, 1, jointCombo);

        // OPC UA node ID
        QTableWidgetItem* nodeItem = new QTableWidgetItem(binding.opcuaNodeId);
        m_bindingTable->setItem(row, 2, nodeItem);
    }

    m_bindingTable->blockSignals(false);
}

void OpcuaWidget::updateButtonStates()
{
    m_connectButton->setEnabled(!m_connected);
    m_disconnectButton->setEnabled(m_connected);
    m_serverUrlEdit->setEnabled(!m_connected);
    m_namespaceSpinBox->setEnabled(!m_connected);
    m_startStopButton->setEnabled(m_connected);
    m_sampleIntervalSpinBox->setEnabled(!m_sampling);

    if (m_sampling) {
        m_startStopButton->setText(tr("停止"));
    } else {
        m_startStopButton->setText(tr("开始"));
    }
}

void OpcuaWidget::onConnectClicked()
{
    QString serverUrl = m_serverUrlEdit->text().trimmed();
    if (serverUrl.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请输入服务器地址。"));
        return;
    }
    QString prefix = m_prefixEdit->text().trimmed();
    if (prefix.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请输入前缀。"));
        return;
    }


    m_statusLabel->setText(tr("状态: 正在连接..."));
    m_statusLabel->setStyleSheet("color: orange;");

    // 初始化连接
    QStringList params;
    params << serverUrl << prefix;  // URL和前缀

    int result = m_connector->init(params);
    if (result == 0) {
        m_connector->setNamespaceIndex(m_namespaceSpinBox->value());
        m_connected = true;
        m_statusLabel->setText(tr("状态: 已连接"));
        m_statusLabel->setStyleSheet("color: green;");
        emit connectionStatusChanged(true, tr("已连接到 ") + serverUrl);
    } else {
        m_statusLabel->setText(tr("状态: 连接失败"));
        m_statusLabel->setStyleSheet("color: red;");
        emit connectionStatusChanged(false, tr("连接失败"));
    }

    updateButtonStates();
}

void OpcuaWidget::onDisconnectClicked()
{
    stopSampling();

    if (m_connector) {
        m_connector->release();
    }

    m_connected = false;
    m_statusLabel->setText(tr("状态: 未连接"));
    m_statusLabel->setStyleSheet("color: gray;");
    emit connectionStatusChanged(false, tr("已断开连接"));

    updateButtonStates();
}

void OpcuaWidget::onStartStopClicked()
{
    if (m_sampling) {
        stopSampling();
    } else {
        startSampling();
    }
}

void OpcuaWidget::startSampling()
{
    if (!m_connected) {
        QMessageBox::warning(this, tr("错误"), tr("请先连接到OPC UA服务器。"));
        return;
    }

    int interval = m_sampleIntervalSpinBox->value();
    m_sampleTimer->start(interval);
    m_sampling = true;
    updateButtonStates();
}

void OpcuaWidget::stopSampling()
{
    m_sampleTimer->stop();
    m_sampling = false;
    updateButtonStates();
}

void OpcuaWidget::onAddBindingClicked()
{
    int row = m_bindingTable->rowCount();
    m_bindingTable->insertRow(row);

    // Enabled checkbox
    QWidget* checkWidget = new QWidget();
    QHBoxLayout* checkLayout = new QHBoxLayout(checkWidget);
    checkLayout->setAlignment(Qt::AlignCenter);
    checkLayout->setContentsMargins(0, 0, 0, 0);
    QCheckBox* checkBox = new QCheckBox();
    checkBox->setChecked(true);
    checkLayout->addWidget(checkBox);
    m_bindingTable->setCellWidget(row, 0, checkWidget);

    // Joint name combo box
    QComboBox* jointCombo = new QComboBox();
    jointCombo->addItems(m_jointNames);
    m_bindingTable->setCellWidget(row, 1, jointCombo);

    // OPC UA node ID
    QTableWidgetItem* nodeItem = new QTableWidgetItem("");
    m_bindingTable->setItem(row, 2, nodeItem);
}

void OpcuaWidget::onRemoveBindingClicked()
{
    int currentRow = m_bindingTable->currentRow();
    if (currentRow >= 0) {
        m_bindingTable->removeRow(currentRow);
    }
}

void OpcuaWidget::onSampleTimerTimeout()
{
    // readOpcuaValues();

    QtConcurrent::run([=](){
        readOpcuaValues();
    });
}

void OpcuaWidget::onBindingCellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    // 可以在这里添加验证逻辑
}

void OpcuaWidget::readOpcuaValues()
{
    if (!m_connected || !m_connector) {
        return;
    }

    QElapsedTimer timer;
    timer.start();

    QMap<QString, double> valMap;
    QList<BaseConnector::DataUnit> unitList;

    QList<OpcuaBinding> bindings = getBindings();
    for (const auto& binding : bindings) {
        if (!binding.enabled || binding.opcuaNodeId.isEmpty()) {
            continue;
        }

        // QVariant value;
        // QString errorString;
        // int result = m_connector->readValue(binding.opcuaNodeId, value, &errorString);

        // // qDebug() << "read value:" << binding.opcuaNodeId << value << errorString;

        // if (result == 0 && value.isValid()) {
        //     double jointValue = value.toDouble();
        //     // 取到的值要转成弧度值
        //     jointValue = qDegreesToRadians(jointValue);
        //     valMap.insert(binding.jointName, jointValue);
        // }

        valMap.insert(binding.jointName, 0.0);
        BaseConnector::DataUnit unit;
        unit.path = binding.opcuaNodeId;
        unitList << unit;
    }

    if(unitList.length() > 0)
    {
        QString errorString;
        int ret = m_connector->readValueList(unitList);
        if(ret == 0)
        {
            QStringList nameList = valMap.keys();
            for(int i = 0; i < nameList.length(); i++)
            {
                QString jointName = nameList.at(i);

                double jointValue = unitList.at(i).value.toDouble();
                // 取到的值要转成弧度值
                jointValue = qDegreesToRadians(jointValue);
                valMap.insert(jointName, jointValue);
            }
        }
    }

    // qDebug() << "read plc interval:" << timer.elapsed() << valMap;

    // 读取完之后再一次性刷新
    emit jointValueUpdated(valMap);
}
