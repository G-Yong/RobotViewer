#ifndef URDFPARSER_H
#define URDFPARSER_H

#include <QString>
#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QMap>
#include <QVector>
#include <memory>

/**
 * @brief URDF几何体类型
 */
enum class GeometryType {
    None,
    Box,
    Cylinder,
    Sphere,
    Mesh
};

/**
 * @brief 几何体结构
 */
struct Geometry {
    GeometryType type = GeometryType::None;
    QString meshFilename;       // 网格文件路径
    double meshScale[3] = {1.0, 1.0, 1.0};
    double boxSize[3] = {0, 0, 0};      // 盒子尺寸
    double cylinderRadius = 0;  // 圆柱半径
    double cylinderLength = 0;  // 圆柱长度
    double sphereRadius = 0;    // 球体半径
};

/**
 * @brief 材质结构
 */
struct Material {
    QString name;
    double color[4] = {0.8, 0.8, 0.8, 1.0}; // RGBA
    QString textureFilename;
};

/**
 * @brief 原点（位姿）结构
 */
struct Origin {
    double xyz[3] = {0, 0, 0};
    double rpy[3] = {0, 0, 0}; // roll, pitch, yaw
    
    QMatrix4x4 toMatrix() const;
    QVector3D position() const { return QVector3D(xyz[0], xyz[1], xyz[2]); }
    QQuaternion rotation() const;
};

/**
 * @brief 视觉元素结构
 */
struct Visual {
    QString name;
    Origin origin;
    Geometry geometry;
    Material material;
};

/**
 * @brief 碰撞元素结构
 */
struct Collision {
    QString name;
    Origin origin;
    Geometry geometry;
};

/**
 * @brief 惯性结构
 */
struct Inertial {
    Origin origin;
    double mass = 0;
    double ixx = 0, ixy = 0, ixz = 0;
    double iyy = 0, iyz = 0, izz = 0;
};

/**
 * @brief 链接（Link）结构
 */
struct URDFLink {
    QString name;
    Inertial inertial;
    QVector<Visual> visuals;
    QVector<Collision> collisions;
};

/**
 * @brief 关节类型
 */
enum class JointType {
    Unknown,
    Fixed,
    Revolute,
    Continuous,
    Prismatic,
    Floating,
    Planar
};

/**
 * @brief 关节限制
 */
struct JointLimits {
    double lower = 0;
    double upper = 0;
    double effort = 0;
    double velocity = 0;
};

/**
 * @brief 关节动力学参数
 */
struct JointDynamics {
    double damping = 0;
    double friction = 0;
};

/**
 * @brief 关节（Joint）结构
 */
struct URDFJoint {
    QString name;
    JointType type = JointType::Unknown;
    Origin origin;
    QString parentLink;
    QString childLink;
    double axis[3] = {1, 0, 0};
    JointLimits limits;
    JointDynamics dynamics;
    double currentValue = 0; // 当前关节值
    
    /**
     * @brief 获取关节变换矩阵
     * @param value 关节值（角度或位移）
     * @return 变换矩阵
     */
    QMatrix4x4 getTransform(double value) const;
    
    /**
     * @brief 检查关节是否可动
     */
    bool isMovable() const {
        return type == JointType::Revolute || 
               type == JointType::Continuous || 
               type == JointType::Prismatic;
    }
};

/**
 * @brief URDF机器人模型
 */
struct URDFModel {
    QString name;
    QMap<QString, std::shared_ptr<URDFLink>> links;
    QMap<QString, std::shared_ptr<URDFJoint>> joints;
    QString rootLink; // 根链接名称
    
    /**
     * @brief 获取可动关节列表
     */
    QVector<std::shared_ptr<URDFJoint>> getMovableJoints() const;
    
    /**
     * @brief 获取链接的子关节
     */
    QVector<std::shared_ptr<URDFJoint>> getChildJoints(const QString& linkName) const;
    
    /**
     * @brief 获取链接的父关节
     */
    std::shared_ptr<URDFJoint> getParentJoint(const QString& linkName) const;
};

/**
 * @brief URDF解析器类
 */
class URDFParser
{
public:
    URDFParser();
    ~URDFParser();
    
    /**
     * @brief 从文件加载URDF
     * @param filename URDF文件路径
     * @return 是否成功
     */
    bool loadFromFile(const QString& filename);
    
    /**
     * @brief 从字符串加载URDF
     * @param content URDF内容
     * @param basePath 基础路径（用于解析相对路径）
     * @return 是否成功
     */
    bool loadFromString(const QString& content, const QString& basePath = "");
    
    /**
     * @brief 获取解析后的模型
     */
    std::shared_ptr<URDFModel> getModel() const { return m_model; }
    
    /**
     * @brief 获取错误信息
     */
    QString getErrorMessage() const { return m_errorMessage; }
    
    /**
     * @brief 获取URDF文件的基础路径
     */
    QString getBasePath() const { return m_basePath; }
    
    /**
     * @brief 解析网格文件路径
     * @param meshPath 原始路径（可能包含package://）
     * @return 实际文件路径
     */
    QString resolveMeshPath(const QString& meshPath) const;

private:
    bool parseRobot(const class QDomElement& element);
    bool parseLink(const class QDomElement& element);
    bool parseJoint(const class QDomElement& element);
    Origin parseOrigin(const class QDomElement& element);
    Geometry parseGeometry(const class QDomElement& element);
    Material parseMaterial(const class QDomElement& element);
    void findRootLink();
    QString expandXacroMacros(const QString& value) const;
    
    std::shared_ptr<URDFModel> m_model;
    QString m_basePath;
    QString m_errorMessage;
    QMap<QString, Material> m_materials; // 全局材质定义
};

#endif // URDFPARSER_H
