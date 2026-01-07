#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMap>

#pragma execution_character_set("utf-8")

#define SOFTWARE_VERSION QString("0.0.2")

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class RobotScene;
class JointControlWidget;
class OpcuaWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenURDF();
    void onRobotLoaded();
    void onLoadError(const QString& error);
    void onJointValueChanged(QMap<QString, double> vals);
    void onEndEffectorPositionChanged(const QVector3D& position);
    void onResetCamera();
    void onToggleGrid(bool checked);
    void onToggleAxes(bool checked);
    void onToggleJointAxes(bool checked);
    void onToggleColoredLinks(bool checked);
    void onToggleZUp(bool checked);
    void onToggleAutoScale(bool checked);
    void onToggleTrajectory(bool checked);
    void onTrajectoryLifetimeChanged(double seconds);
    void onConfigureEndEffectors();  // 新增：配置多个末端执行器

private:
    void setupUI();
    void setupMenus();
    void setupConnections();
    void loadSettings();
    void saveSettings();

    Ui::MainWindow *ui;
    RobotScene* m_scene = nullptr;
    JointControlWidget* m_jointControl = nullptr;
    OpcuaWidget* m_opcuaWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_positionLabel = nullptr;
    QString m_lastUrdfPath;
};
#endif // MAINWINDOW_H
