#ifndef ROBOTENTITY_H
#define ROBOTENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QPhongMaterial>
#include <QMap>
#include <QVector>
#include <QVector3D>
#include <QColor>
#include <QTimer>
#include <memory>

#include "urdfparser.h"

class TrajectoryEntity;

/**
 * @brief 链接实体
 * 包含链接的3D模型和变换
 */
class LinkEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    
public:
    explicit LinkEntity(const QString& name, Qt3DCore::QEntity* parent = nullptr);
    
    QString linkName() const { return m_linkName; }
    Qt3DCore::QTransform* transform() const { return m_transform; }
    
    void setVisualEntity(Qt3DCore::QEntity* entity) { m_visualEntity = entity; }
    Qt3DCore::QEntity* visualEntity() const { return m_visualEntity; }
    
private:
    QString m_linkName;
    Qt3DCore::QTransform* m_transform;
    Qt3DCore::QEntity* m_visualEntity = nullptr;
};

/**
 * @brief 关节实体
 * 包含关节的变换，用于控制子链接的运动
 */
class JointEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    
public:
    explicit JointEntity(std::shared_ptr<URDFJoint> joint, Qt3DCore::QEntity* parent = nullptr);
    
    QString jointName() const { return m_joint->name; }
    std::shared_ptr<URDFJoint> joint() const { return m_joint; }
    Qt3DCore::QTransform* transform() const { return m_transform; }
    
    double jointValue() const { return m_jointValue; }
    void setJointValue(double value);
    
    void setAxesVisible(bool visible);
    bool isAxesVisible() const { return m_axesVisible; }
    
signals:
    void jointValueChanged(const QString& jointName, double value);
    
private:
    void createAxes();
    
    std::shared_ptr<URDFJoint> m_joint;
    Qt3DCore::QTransform* m_transform;
    double m_jointValue = 0;
    Qt3DCore::QEntity* m_axesEntity = nullptr;
    bool m_axesVisible = false;
};

/**
 * @brief 机器人3D实体
 * 根据URDF模型创建完整的机器人3D显示
 */
class RobotEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    
public:
    explicit RobotEntity(Qt3DCore::QEntity* parent = nullptr);
    ~RobotEntity();
    
    /**
     * @brief 从URDF文件加载机器人
     * @param urdfFile URDF文件路径
     * @return 是否成功
     */
    bool loadFromURDF(const QString& urdfFile);
    
    /**
     * @brief 获取URDF模型
     */
    std::shared_ptr<URDFModel> getModel() const { return m_model; }
    
    /**
     * @brief 获取所有可动关节
     */
    QVector<std::shared_ptr<URDFJoint>> getMovableJoints() const;
    
    /**
     * @brief 获取关节值
     * @param jointName 关节名称
     * @return 关节值（弧度或米）
     */
    double getJointValue(const QString& jointName) const;
    
    /**
     * @brief 获取所有关节值
     * @return 关节名称到值的映射
     */
    QMap<QString, double> getAllJointValues() const;
    
    /**
     * @brief 获取末端执行器位置
     * @param linkName 末端链接名称（如果为空则自动检测）
     * @return 世界坐标系下的位置
     */
    QVector3D getEndEffectorPosition(const QString& linkName = "") const;
    
    /**
     * @brief 获取链接的世界变换矩阵
     */
    QMatrix4x4 getLinkWorldTransform(const QString& linkName) const;
    
    /**
     * @brief 获取指定名称的链接实体
     * @param linkName 链接名称
     * @return 链接实体指针，如果不存在则返回nullptr
     */
    LinkEntity* getLinkEntity(const QString& linkName) const;
    
    /**
     * @brief 设置末端链接名称（单个末端执行器，保留向后兼容）
     */
    void setEndEffectorLink(const QString& linkName);
    QString getEndEffectorLink() const { return m_endEffectorLink; }
    
    /**
     * @brief 添加末端执行器及其对应的轨迹
     * @param linkName 末端链接名称
     * @param trajectory 该末端的轨迹实体
     * @param name 末端执行器的显示名称（可选，默认使用linkName）
     */
    void addEndEffector(const QString& linkName, TrajectoryEntity* trajectory, const QString& name = QString());
    
    /**
     * @brief 移除指定的末端执行器
     */
    void removeEndEffector(const QString& linkName);
    
    /**
     * @brief 清除所有末端执行器
     */
    void clearEndEffectors();
    
    /**
     * @brief 获取所有末端执行器名称列表
     */
    QStringList getEndEffectorLinks() const;
    
    /**
     * @brief 设置轨迹实体（单个轨迹，保留向后兼容）
     */
    void setTrajectoryEntity(TrajectoryEntity* trajectory) { m_trajectoryEntity = trajectory; }
    
    /**
     * @brief 设置轨迹采样间隔（毫秒）
     */
    void setTrajectorySampleInterval(int msec);
    
    /**
     * @brief 启用/禁用轨迹显示
     */
    void setTrajectoryEnabled(bool enabled);
    
    /**
     * @brief 设置关节坐标轴可见性
     */
    void setJointAxesVisible(bool visible);
    bool isJointAxesVisible() const { return m_jointAxesVisible; }
    
    /**
     * @brief 设置Link着色模式
     * @param enabled true: 不同Link使用不同颜色, false: 使用原始颜色
     */
    void setColoredLinksEnabled(bool enabled);
    bool isColoredLinksEnabled() const { return m_coloredLinksEnabled; }
    
    /**
     * @brief 计算模型包围盒
     * @param minPoint 输出最小点
     * @param maxPoint 输出最大点
     */
    void getBoundingBox(QVector3D& minPoint, QVector3D& maxPoint) const;
    
    /**
     * @brief 获取模型大小（包围盒对角线长度）
     */
    float getModelSize() const;
    
    /**
     * @brief 设置模型缩放比例
     */
    void setScale(float scale);
    float getScale() const;
    
    /**
     * @brief 获取错误信息
     */
    QString getErrorMessage() const { return m_errorMessage; }
    
public slots:
    /**
     * @brief 设置关节值
     * @param jointName 关节名称
     * @param value 关节值（弧度或米）
     */
    void setJointValue(const QString& jointName, double value);
    
    /**
     * @brief 设置多个关节值
     * @param jointValues 关节名称到值的映射
     */
    void setJointValues(const QMap<QString, double>& jointValues);
    
    /**
     * @brief 重置所有关节到初始位置
     */
    void resetJoints();
    
signals:
    /**
     * @brief 关节值改变信号
     */
    void jointValueChanged(const QString& jointName, double value);
    
    /**
     * @brief 机器人加载完成信号
     */
    void robotLoaded();
    
    /**
     * @brief 末端位置改变信号
     */
    void endEffectorPositionChanged(const QVector3D& position);
    
private slots:
    void sampleTrajectory();
    
private:
    void clear();
    bool buildRobotTree();
    void buildLinkEntity(const QString& linkName, Qt3DCore::QEntity* parent, int& linkIndex);
    Qt3DCore::QEntity* createLinkVisual(std::shared_ptr<URDFLink> link, int linkIndex);
    Qt3DCore::QEntity* createPrimitiveGeometry(const Geometry& geom, const Material& mat);
    void findEndEffectorLink();
    void updateJointTransform(JointEntity* jointEntity);
    QMatrix4x4 computeLinkTransform(const QString& linkName) const;
    QVector3D getLinkGeometryCenter(const QString& linkName) const;  // 计算链接几何中心
    bool applyLinkColors(bool enable);
    QColor getLinkColor(int index) const;
    
    std::shared_ptr<URDFModel> m_model;
    URDFParser m_parser;
    QString m_errorMessage;
    
    // 整体变换（用于缩放）
    Qt3DCore::QTransform* m_robotTransform = nullptr;
    float m_scale = 1.0f;
    
    // 实体映射
    QMap<QString, LinkEntity*> m_linkEntities;
    QMap<QString, JointEntity*> m_jointEntities;
    
    // Link材质映射（用于着色模式切换）
    struct LinkMaterialInfo {
        Qt3DExtras::QPhongMaterial* material = nullptr;
        QColor originalColor;
        int linkIndex = 0;
    };
    QList<LinkMaterialInfo> m_linkMaterials;
    
    // 末端执行器
    QString m_endEffectorLink;  // 单个末端执行器（向后兼容）
    TrajectoryEntity* m_trajectoryEntity = nullptr;  // 单个轨迹（向后兼容）
    
    // 多末端执行器支持
    struct EndEffectorInfo {
        QString linkName;           // 链接名称
        QString displayName;        // 显示名称
        TrajectoryEntity* trajectory; // 轨迹实体
    };
    QMap<QString, EndEffectorInfo> m_endEffectors;  // linkName -> EndEffectorInfo
    
    QTimer* m_trajectoryTimer = nullptr;
    bool m_trajectoryEnabled = true;
    bool m_jointAxesVisible = false;
    bool m_coloredLinksEnabled = false;
};

#endif // ROBOTENTITY_H
