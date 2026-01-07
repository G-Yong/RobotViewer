#ifndef ROBOTSCENE_H
#define ROBOTSCENE_H

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/Qt3DWindow>
#include <QWidget>

class RobotEntity;
class OrbitCameraController;
class TrajectoryEntity;

/**
 * @brief 机器人3D场景
 * 包含场景设置、网格、坐标轴、灯光等
 */
class RobotScene : public QObject
{
    Q_OBJECT
    
public:
    explicit RobotScene(QObject* parent = nullptr);
    ~RobotScene();
    
    /**
     * @brief 初始化场景
     */
    void initialize();
    
    /**
     * @brief 获取3D窗口的Widget容器
     */
    QWidget* container() const { return m_container; }
    
    /**
     * @brief 获取根实体
     */
    Qt3DCore::QEntity* rootEntity() const { return m_rootEntity; }
    
    /**
     * @brief 获取相机
     */
    Qt3DRender::QCamera* camera() const { return m_camera; }
    
    /**
     * @brief 获取相机控制器
     */
    OrbitCameraController* cameraController() const { return m_cameraController; }
    
    /**
     * @brief 获取机器人实体
     */
    RobotEntity* robotEntity() const { return m_robotEntity; }
    
    /**
     * @brief 获取轨迹实体
     */
    TrajectoryEntity* trajectoryEntity() const { return m_trajectoryEntity; }
    
    /**
     * @brief 加载URDF机器人
     */
    bool loadRobot(const QString& urdfFile);
    
    /**
     * @brief 设置网格可见性
     */
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }
    
    /**
     * @brief 设置坐标轴可见性
     */
    void setAxesVisible(bool visible);
    bool isAxesVisible() const { return m_axesVisible; }
    
    /**
     * @brief 设置关节坐标轴可见性
     */
    void setJointAxesVisible(bool visible);
    bool isJointAxesVisible() const { return m_jointAxesVisible; }
    
    /**
     * @brief 设置Link着色模式
     */
    void setColoredLinksEnabled(bool enabled);
    bool isColoredLinksEnabled() const { return m_coloredLinksEnabled; }

    /**
     * @brief 切换Z轴朝上模式（默认Y轴朝上）
     */
    void setZUpEnabled(bool enabled);
    bool isZUpEnabled() const { return m_zUpEnabled; }
    
    /**
     * @brief 设置自动缩放启用
     */
    void setAutoScaleEnabled(bool enabled);
    bool isAutoScaleEnabled() const { return m_autoScaleEnabled; }
    
    /**
     * @brief 设置目标模型大小（缩放后的目标包围盒对角线长度）
     */
    void setTargetModelSize(float size);
    float getTargetModelSize() const { return m_targetModelSize; }
    
    /**
     * @brief 设置轨迹可见性
     */
    void setTrajectoryVisible(bool visible);
    
    /**
     * @brief 设置轨迹生命周期（秒）
     */
    void setTrajectoryLifetime(float seconds);
    
    /**
     * @brief 添加末端执行器轨迹
     * @param linkName 末端链接名称
     * @param name 显示名称（可选）
     * @param color 轨迹颜色（可选，默认使用预定义颜色）
     * @return 创建的轨迹实体指针
     */
    TrajectoryEntity* addEndEffectorTrajectory(const QString& linkName, 
                                                const QString& name = QString(),
                                                const QColor& color = QColor());
    
    /**
     * @brief 移除末端执行器轨迹
     */
    void removeEndEffectorTrajectory(const QString& linkName);
    
    /**
     * @brief 清除所有末端执行器轨迹
     */
    void clearEndEffectorTrajectories();
    
    /**
     * @brief 获取所有末端执行器轨迹信息
     */
    QMap<QString, TrajectoryEntity*> getEndEffectorTrajectories() const;
    
    /**
     * @brief 获取实体的世界变换矩阵（从局部坐标到世界坐标）
     * @param entity 目标实体
     * @return 世界变换矩阵
     */
    QMatrix4x4 getWorldMatrix(Qt3DCore::QEntity *entity) const;
    
    /**
     * @brief 重置相机视角
     */
    void resetCamera();
    
    /**
     * @brief 适配视角到机器人
     */
    void fitCameraToRobot();
    
signals:
    void robotLoaded();
    void loadError(const QString& error);
    
private:
    void createGrid();
    void createAxes();
    void createLights();
    void setupRenderSettings();
    
    Qt3DExtras::Qt3DWindow* m_view = nullptr;
    QWidget* m_container = nullptr;
    Qt3DCore::QEntity* m_rootEntity = nullptr;
    Qt3DCore::QEntity* m_worldEntity = nullptr; // 受坐标系切换控制的场景节点
    Qt3DCore::QTransform* m_sceneTransform = nullptr; // 控制Y-up / Z-up
    Qt3DRender::QCamera* m_camera = nullptr;
    OrbitCameraController* m_cameraController = nullptr;
    
    RobotEntity* m_robotEntity = nullptr;
    TrajectoryEntity* m_trajectoryEntity = nullptr;  // 单个轨迹（向后兼容）
    QMap<QString, TrajectoryEntity*> m_endEffectorTrajectories;  // 多末端执行器轨迹
    float m_trajectoryLifetime = 2.0f;  // 轨迹生命周期（秒）
    
    Qt3DCore::QEntity* m_gridEntity = nullptr;
    Qt3DCore::QEntity* m_axesEntity = nullptr;
    
    bool m_gridVisible = true;
    bool m_axesVisible = true;
    bool m_jointAxesVisible = false;
    bool m_coloredLinksEnabled = false;
    bool m_zUpEnabled = false; // 默认Y轴朝上
    bool m_autoScaleEnabled = true; // 默认启用自动缩放
    float m_targetModelSize = 2.0f; // 目标模型大小
};

#endif // ROBOTSCENE_H
