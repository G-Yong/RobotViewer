#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "robotscene.h"
#include "robotentity.h"
#include "jointcontrolwidget.h"
#include "trajectoryentity.h"
#include "opcuawidget.h"
#include "settingsmanager.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QGroupBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QDockWidget>
#include <QVector3D>
#include <QTabWidget>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QDialog>
#include <QListWidget>
#include <QColorDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("RobotViewer") + QString(" V%1").arg(SOFTWARE_VERSION));
    resize(1200, 800);
    
    setupUI();
    setupMenus();
    setupConnections();
    
    // Auto-load last URDF after a short delay to ensure UI is ready
    QTimer::singleShot(100, this, [this]() {
        loadSettings();
    });
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::setupUI()
{
    // 创建中央Widget
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 创建分割器
    QSplitter* splitter = new QSplitter(Qt::Horizontal, centralWidget);
    
    // 创建3D场景
    m_scene = new RobotScene(this);
    m_scene->initialize();
    
    // 左侧：3D视图
    splitter->addWidget(m_scene->container());
    
    // 右侧：控制面板
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    rightLayout->setSpacing(10);
    
    // 视图控制组
    QGroupBox* viewGroup = new QGroupBox(tr("视图选项"), rightPanel);
    QVBoxLayout* viewLayout = new QVBoxLayout(viewGroup);
    
    QCheckBox* gridCheck = new QCheckBox(tr("显示网格"), viewGroup);
    gridCheck->setChecked(true);
    gridCheck->setObjectName("gridCheck");
    connect(gridCheck, &QCheckBox::toggled, this, &MainWindow::onToggleGrid);
    viewLayout->addWidget(gridCheck);
    
    QCheckBox* axesCheck = new QCheckBox(tr("显示坐标轴"), viewGroup);
    axesCheck->setChecked(true);
    axesCheck->setObjectName("axesCheck");
    connect(axesCheck, &QCheckBox::toggled, this, &MainWindow::onToggleAxes);
    viewLayout->addWidget(axesCheck);

    QCheckBox* zUpCheck = new QCheckBox(tr("Z轴朝上"), viewGroup);
    zUpCheck->setChecked(false);
    zUpCheck->setObjectName("zUpCheck");
    connect(zUpCheck, &QCheckBox::toggled, this, &MainWindow::onToggleZUp);
    viewLayout->addWidget(zUpCheck);
    
    QCheckBox* jointAxesCheck = new QCheckBox(tr("显示关节坐标轴"), viewGroup);
    jointAxesCheck->setChecked(false);
    jointAxesCheck->setObjectName("jointAxesCheck");
    connect(jointAxesCheck, &QCheckBox::toggled, this, &MainWindow::onToggleJointAxes);
    viewLayout->addWidget(jointAxesCheck);
    
    QCheckBox* coloredLinksCheck = new QCheckBox(tr("零件着色"), viewGroup);
    coloredLinksCheck->setChecked(false);
    coloredLinksCheck->setObjectName("coloredLinksCheck");
    connect(coloredLinksCheck, &QCheckBox::toggled, this, &MainWindow::onToggleColoredLinks);
    viewLayout->addWidget(coloredLinksCheck);
    
    QCheckBox* trajectoryCheck = new QCheckBox(tr("显示轨迹"), viewGroup);
    trajectoryCheck->setChecked(true);
    trajectoryCheck->setObjectName("trajectoryCheck");
    connect(trajectoryCheck, &QCheckBox::toggled, this, &MainWindow::onToggleTrajectory);
    viewLayout->addWidget(trajectoryCheck);
    
    QCheckBox* autoScaleCheck = new QCheckBox(tr("自动缩放模型"), viewGroup);
    autoScaleCheck->setChecked(true);
    autoScaleCheck->setObjectName("autoScaleCheck");
    connect(autoScaleCheck, &QCheckBox::toggled, this, &MainWindow::onToggleAutoScale);
    viewLayout->addWidget(autoScaleCheck);
    
    // 轨迹生命周期
    QHBoxLayout* trajectoryLayout = new QHBoxLayout();
    QLabel* trajectoryLabel = new QLabel(tr("轨迹生命周期:"), viewGroup);
    QDoubleSpinBox* trajectorySpinBox = new QDoubleSpinBox(viewGroup);
    trajectorySpinBox->setRange(0.5, 10.0);
    trajectorySpinBox->setValue(2.0);
    trajectorySpinBox->setSuffix(" s");
    trajectorySpinBox->setDecimals(1);
    connect(trajectorySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onTrajectoryLifetimeChanged);
    trajectoryLayout->addWidget(trajectoryLabel);
    trajectoryLayout->addWidget(trajectorySpinBox);
    viewLayout->addLayout(trajectoryLayout);
    
    // 配置末端执行器按钮
    QPushButton* configEndEffectorBtn = new QPushButton(tr("配置末端执行器..."), viewGroup);
    connect(configEndEffectorBtn, &QPushButton::clicked, this, &MainWindow::onConfigureEndEffectors);
    viewLayout->addWidget(configEndEffectorBtn);
    
    rightLayout->addWidget(viewGroup);
    
    // 末端位置显示
    QGroupBox* posGroup = new QGroupBox(tr("末端执行器位置"), rightPanel);
    QVBoxLayout* posLayout = new QVBoxLayout(posGroup);
    m_positionLabel = new QLabel("X: 0.000  Y: 0.000  Z: 0.000", posGroup);
    m_positionLabel->setStyleSheet("font-family: monospace;");
    posLayout->addWidget(m_positionLabel);
    rightLayout->addWidget(posGroup);
    
    // 创建Tab Widget来容纳关节控制和OPC UA
    QTabWidget* tabWidget = new QTabWidget(rightPanel);
    
    // 关节控制面板
    m_jointControl = new JointControlWidget(tabWidget);
    tabWidget->addTab(m_jointControl, tr("关节控制"));
    
    // OPC UA通信面板
    m_opcuaWidget = new OpcuaWidget(tabWidget);
    connect(m_opcuaWidget, &OpcuaWidget::settingsChanged, this, [this]() {
        // 保存设置
        saveSettings();
    });
    tabWidget->addTab(m_opcuaWidget, tr("OPC UA通信"));
    
    // 连接OPC UA值变化信号到关节控制
    connect(m_opcuaWidget, &OpcuaWidget::jointValueUpdated, 
            this, &MainWindow::onJointValueChanged);
    
    rightLayout->addWidget(tabWidget);
    
    // // 添加弹性空间
    // rightLayout->addStretch();
    
    splitter->addWidget(rightPanel);
    
    // 设置分割比例
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({900, 300});
    
    mainLayout->addWidget(splitter);
    setCentralWidget(centralWidget);
    
    // 状态栏
    m_statusLabel = new QLabel(tr("就绪。打开URDF文件以加载机器人。"), this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::setupMenus()
{
    // 文件菜单
    QMenu* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    
    QAction* openAction = new QAction(tr("打开URDF(&O)..."), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenURDF);
    fileMenu->addAction(openAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction(tr("退出(&X)"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);
    
    // 视图菜单
    QMenu* viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    
    QAction* resetCameraAction = new QAction(tr("重置相机(&R)"), this);
    resetCameraAction->setShortcut(QKeySequence(Qt::Key_R));
    connect(resetCameraAction, &QAction::triggered, this, &MainWindow::onResetCamera);
    viewMenu->addAction(resetCameraAction);
    
    // 工具栏
    QToolBar* toolbar = addToolBar(tr("主工具栏"));
    toolbar->setObjectName("myToolBar");
    toolbar->addAction(openAction);
    toolbar->addSeparator();
    toolbar->addAction(resetCameraAction);
}

void MainWindow::setupConnections()
{
    // 场景信号连接
    connect(m_scene, &RobotScene::robotLoaded, this, &MainWindow::onRobotLoaded);
    connect(m_scene, &RobotScene::loadError, this, &MainWindow::onLoadError);
    
    // 关节控制 -> 机器人实体
    connect(m_jointControl, &JointControlWidget::jointValueChanged,
            this, &MainWindow::onJointValueChanged);
}

void MainWindow::onOpenURDF()
{
    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("打开URDF文件"),
        m_lastUrdfPath.isEmpty() ? QString() : QFileInfo(m_lastUrdfPath).absolutePath(),
        tr("URDF文件 (*.urdf *.xacro);;所有文件 (*.*)")
    );
    
    if (filename.isEmpty()) return;
    
    m_statusLabel->setText(tr("正在加载: %1").arg(filename));
    
    if (m_scene->loadRobot(filename)) {
        m_lastUrdfPath = filename;
        m_statusLabel->setText(tr("已加载: %1").arg(filename));
    }
}

void MainWindow::onRobotLoaded()
{
    // 更新关节控制面板
    auto robot = m_scene->robotEntity();
    if (robot) {
        auto joints = robot->getMovableJoints();
        m_jointControl->setJoints(joints);
        
        // 更新OPC UA面板的关节列表
        if (m_opcuaWidget) {
            QStringList jointNames;
            for (const auto& joint : joints) {
                jointNames << joint->name;
            }
            m_opcuaWidget->setJointNames(jointNames);
        }
        
        // 连接末端位置信号
        connect(robot, &RobotEntity::endEffectorPositionChanged,
                this, &MainWindow::onEndEffectorPositionChanged);
        
        // 连接关节值变化信号（从机器人实体到控制面板）
        connect(robot, &RobotEntity::jointValueChanged,
                m_jointControl, &JointControlWidget::setJointValue);
    }
}

void MainWindow::onLoadError(const QString& error)
{
    m_statusLabel->setText(tr("加载错误: %1").arg(error));
    QMessageBox::warning(this, tr("加载错误"), error);
}

void MainWindow::onJointValueChanged(QMap<QString, double> vals)
{
    auto robot = m_scene->robotEntity();
    if (robot) {
        robot->setJointValues(vals);
    }
}

void MainWindow::onEndEffectorPositionChanged(const QVector3D& position)
{
    m_positionLabel->setText(QString("X: %1  Y: %2  Z: %3")
                                 .arg(position.x(), 7, 'f', 3)
                                 .arg(position.y(), 7, 'f', 3)
                                 .arg(position.z(), 7, 'f', 3));
}

void MainWindow::onResetCamera()
{
    m_scene->resetCamera();
}

void MainWindow::onToggleGrid(bool checked)
{
    m_scene->setGridVisible(checked);
}

void MainWindow::onToggleAxes(bool checked)
{
    m_scene->setAxesVisible(checked);
}

void MainWindow::onToggleJointAxes(bool checked)
{
    m_scene->setJointAxesVisible(checked);
}

void MainWindow::onToggleColoredLinks(bool checked)
{
    m_scene->setColoredLinksEnabled(checked);
}

void MainWindow::onToggleZUp(bool checked)
{
    m_scene->setZUpEnabled(checked);
}

void MainWindow::onToggleAutoScale(bool checked)
{
    m_scene->setAutoScaleEnabled(checked);
}

void MainWindow::onToggleTrajectory(bool checked)
{
    m_scene->setTrajectoryVisible(checked);
}

void MainWindow::onTrajectoryLifetimeChanged(double seconds)
{
    m_scene->setTrajectoryLifetime(seconds);
}

void MainWindow::onConfigureEndEffectors()
{
    if (!m_scene || !m_scene->robotEntity() || !m_scene->robotEntity()->getModel()) {
        QMessageBox::information(this, tr("配置末端执行器"), 
                                 tr("请先加载一个机器人模型。"));
        return;
    }
    
    // 创建对话框
    QDialog dialog(this);
    dialog.setWindowTitle(tr("配置末端执行器"));
    dialog.setMinimumSize(500, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    // 说明文本
    QLabel* infoLabel = new QLabel(tr("选择要显示轨迹的末端执行器链接："), &dialog);
    layout->addWidget(infoLabel);
    
    // 获取所有链接
    auto robot = m_scene->robotEntity();
    auto model = robot->getModel();
    QStringList allLinks;
    for (const auto& link : model->links) {
        allLinks << link->name;
    }
    allLinks.sort();
    
    // 创建列表（可多选）
    QListWidget* linkList = new QListWidget(&dialog);
    linkList->setSelectionMode(QAbstractItemView::MultiSelection);
    
    // 添加所有链接，并标记已选中的
    QStringList currentEndEffectors = robot->getEndEffectorLinks();
    for (const QString& linkName : allLinks) {
        QListWidgetItem* item = new QListWidgetItem(linkName, linkList);
        if (currentEndEffectors.contains(linkName)) {
            item->setSelected(true);
        }
    }
    
    layout->addWidget(linkList);
    
    // 快捷选择按钮
    QHBoxLayout* quickSelectLayout = new QHBoxLayout();
    QPushButton* selectAllBtn = new QPushButton(tr("全选"), &dialog);
    QPushButton* clearBtn = new QPushButton(tr("清空"), &dialog);
    QPushButton* autoDetectBtn = new QPushButton(tr("自动检测末端"), &dialog);
    
    connect(selectAllBtn, &QPushButton::clicked, linkList, &QListWidget::selectAll);
    connect(clearBtn, &QPushButton::clicked, linkList, &QListWidget::clearSelection);
    connect(autoDetectBtn, &QPushButton::clicked, [linkList, model]() {
        // 自动检测：选择没有子节点的链接（叶节点）
        linkList->clearSelection();
        for (int i = 0; i < linkList->count(); ++i) {
            QListWidgetItem* item = linkList->item(i);
            QString linkName = item->text();
            // 检查是否是叶节点（没有子关节）
            bool isLeaf = true;
            for (const auto& joint : model->joints) {
                if (joint->parentLink == linkName) {
                    isLeaf = false;
                    break;
                }
            }
            if (isLeaf) {
                item->setSelected(true);
            }
        }
    });
    
    quickSelectLayout->addWidget(autoDetectBtn);
    quickSelectLayout->addWidget(selectAllBtn);
    quickSelectLayout->addWidget(clearBtn);
    quickSelectLayout->addStretch();
    layout->addLayout(quickSelectLayout);
    
    // 对话框按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton(tr("确定"), &dialog);
    QPushButton* cancelBtn = new QPushButton(tr("取消"), &dialog);
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);
    
    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        // 清除现有的末端执行器轨迹
        m_scene->clearEndEffectorTrajectories();
        
        // 获取选中的链接
        QList<QListWidgetItem*> selectedItems = linkList->selectedItems();
        
        if (selectedItems.isEmpty()) {
            QMessageBox::information(this, tr("配置末端执行器"), 
                                     tr("未选择任何末端执行器。"));
            return;
        }
        
        // 为每个选中的链接创建轨迹
        for (QListWidgetItem* item : selectedItems) {
            QString linkName = item->text();
            m_scene->addEndEffectorTrajectory(linkName, linkName);
        }
        
        m_statusLabel->setText(tr("已配置 %1 个末端执行器").arg(selectedItems.count()));
    }
}

void MainWindow::loadSettings()
{
    SettingsManager& settings = SettingsManager::instance();
    
    // 恢复窗口几何
    restoreGeometry(settings.getWindowGeometry());
    restoreState(settings.getWindowState());
    
    // 加载上次的URDF路径
    m_lastUrdfPath = settings.getLastUrdfFile();
    if (!m_lastUrdfPath.isEmpty() && QFile::exists(m_lastUrdfPath)) {
        m_statusLabel->setText(tr("自动加载: %1").arg(m_lastUrdfPath));
        if (m_scene->loadRobot(m_lastUrdfPath)) {
            m_statusLabel->setText(tr("已加载: %1").arg(m_lastUrdfPath));
        }
    }
    
    // 加载视图选项
    if (QCheckBox* gridCheck = findChild<QCheckBox*>("gridCheck")) {
        gridCheck->setChecked(settings.getShowGrid());
    }
    if (QCheckBox* axesCheck = findChild<QCheckBox*>("axesCheck")) {
        axesCheck->setChecked(settings.getShowAxes());
    }
    if (QCheckBox* zUpCheck = findChild<QCheckBox*>("zUpCheck")) {
        zUpCheck->setChecked(settings.getZUpEnabled());
    }
    if (QCheckBox* jointAxesCheck = findChild<QCheckBox*>("jointAxesCheck")) {
        jointAxesCheck->setChecked(settings.getShowJointAxes());
    }
    if (QCheckBox* coloredLinksCheck = findChild<QCheckBox*>("coloredLinksCheck")) {
        coloredLinksCheck->setChecked(settings.getColoredLinks());
    }
    if (QCheckBox* trajectoryCheck = findChild<QCheckBox*>("trajectoryCheck")) {
        trajectoryCheck->setChecked(settings.getShowTrajectory());
    }
    if (QCheckBox* autoScaleCheck = findChild<QCheckBox*>("autoScaleCheck")) {
        autoScaleCheck->setChecked(settings.getAutoScaleEnabled());
        m_scene->setAutoScaleEnabled(settings.getAutoScaleEnabled());
    }
    
    // 加载OPC UA设置
    if (m_opcuaWidget) {
        m_opcuaWidget->setServerUrl(settings.getOpcuaServerUrl());
        m_opcuaWidget->setPrefix(settings.getOpcuaPrefix());
        m_opcuaWidget->setSampleInterval(settings.getOpcuaSampleInterval());
        m_opcuaWidget->setBindings(settings.getOpcuaBindings());
    }
}

void MainWindow::saveSettings()
{
    SettingsManager& settings = SettingsManager::instance();
    
    // 保存窗口几何
    settings.setWindowGeometry(saveGeometry());
    settings.setWindowState(saveState());
    
    // 保存上次的URDF路径
    settings.setLastUrdfFile(m_lastUrdfPath);
    
    // 保存视图选项
    if (QCheckBox* gridCheck = findChild<QCheckBox*>("gridCheck")) {
        settings.setShowGrid(gridCheck->isChecked());
    }
    if (QCheckBox* axesCheck = findChild<QCheckBox*>("axesCheck")) {
        settings.setShowAxes(axesCheck->isChecked());
    }
    if (QCheckBox* zUpCheck = findChild<QCheckBox*>("zUpCheck")) {
        settings.setZUpEnabled(zUpCheck->isChecked());
    }
    if (QCheckBox* jointAxesCheck = findChild<QCheckBox*>("jointAxesCheck")) {
        settings.setShowJointAxes(jointAxesCheck->isChecked());
    }
    if (QCheckBox* coloredLinksCheck = findChild<QCheckBox*>("coloredLinksCheck")) {
        settings.setColoredLinks(coloredLinksCheck->isChecked());
    }
    if (QCheckBox* trajectoryCheck = findChild<QCheckBox*>("trajectoryCheck")) {
        settings.setShowTrajectory(trajectoryCheck->isChecked());
    }
    if (QCheckBox* autoScaleCheck = findChild<QCheckBox*>("autoScaleCheck")) {
        settings.setAutoScaleEnabled(autoScaleCheck->isChecked());
    }
    
    // 保存OPC UA设置
    if (m_opcuaWidget) {
        settings.setOpcuaServerUrl(m_opcuaWidget->getServerUrl());
        settings.setOpcuaPrefix(m_opcuaWidget->getPrefix());
        settings.setOpcuaSampleInterval(m_opcuaWidget->getSampleInterval());
        settings.setOpcuaBindings(m_opcuaWidget->getBindings());
    }
}
