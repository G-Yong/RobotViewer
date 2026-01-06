#include "robotentity.h"
#include "assimpmodelloader.h"
#include "trajectoryentity.h"

#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <QtMath>
#include <QDebug>
#include <limits>

// ==================== LinkEntity ====================

LinkEntity::LinkEntity(const QString& name, Qt3DCore::QEntity* parent)
    : Qt3DCore::QEntity(parent)
    , m_linkName(name)
{
    m_transform = new Qt3DCore::QTransform(this);
    addComponent(m_transform);
}

// ==================== JointEntity ====================

JointEntity::JointEntity(std::shared_ptr<URDFJoint> joint, Qt3DCore::QEntity* parent)
    : Qt3DCore::QEntity(parent)
    , m_joint(joint)
{
    m_transform = new Qt3DCore::QTransform(this);
    addComponent(m_transform);
    
    // 设置初始变换（关节原点）
    m_transform->setMatrix(joint->origin.toMatrix());
    
    // 创建关节坐标轴
    createAxes();
}

void JointEntity::setJointValue(double value)
{
    // 限制关节值在范围内
    if (m_joint->type == JointType::Revolute || m_joint->type == JointType::Prismatic) {
        value = qBound(m_joint->limits.lower, value, m_joint->limits.upper);
    }
    
    if (qFuzzyCompare(m_jointValue, value)) {
        return;
    }

    m_jointValue = value;
    m_joint->currentValue = value;
    
    // 计算新的变换矩阵
    QMatrix4x4 matrix = m_joint->origin.toMatrix();
    matrix *= m_joint->getTransform(value);
    m_transform->setMatrix(matrix);
    
    emit jointValueChanged(m_joint->name, value);
}

void JointEntity::setAxesVisible(bool visible)
{
    m_axesVisible = visible;
    if (m_axesEntity) {
        m_axesEntity->setEnabled(visible);
    }
}

void JointEntity::createAxes()
{
    m_axesEntity = new Qt3DCore::QEntity(this);
    m_axesEntity->setEnabled(m_axesVisible);
    
    const float axisLength = 0.1f; // 关节坐标轴较短
    
    // 为每个轴创建单独的线条实体，这样可以设置不同的颜色
    // X轴 - 红色
    {
        Qt3DCore::QEntity* xAxisEntity = new Qt3DCore::QEntity(m_axesEntity);
        Qt3DRender::QGeometry* xGeometry = new Qt3DRender::QGeometry(xAxisEntity);
        
        QByteArray xVertexData;
        xVertexData.resize(2 * 3 * sizeof(float));
        float* xVerts = reinterpret_cast<float*>(xVertexData.data());
        xVerts[0] = 0; xVerts[1] = 0; xVerts[2] = 0;
        xVerts[3] = axisLength; xVerts[4] = 0; xVerts[5] = 0;
        
        Qt3DRender::QBuffer* xBuffer = new Qt3DRender::QBuffer(xGeometry);
        xBuffer->setData(xVertexData);
        
        Qt3DRender::QAttribute* xPosAttr = new Qt3DRender::QAttribute(xGeometry);
        xPosAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        xPosAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        xPosAttr->setVertexSize(3);
        xPosAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        xPosAttr->setBuffer(xBuffer);
        xPosAttr->setByteStride(3 * sizeof(float));
        xPosAttr->setCount(2);
        xGeometry->addAttribute(xPosAttr);
        
        Qt3DRender::QGeometryRenderer* xRenderer = new Qt3DRender::QGeometryRenderer(xAxisEntity);
        xRenderer->setGeometry(xGeometry);
        xRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        xAxisEntity->addComponent(xRenderer);
        
        Qt3DExtras::QPhongMaterial* xMat = new Qt3DExtras::QPhongMaterial(xAxisEntity);
        xMat->setAmbient(QColor(255, 0, 0));
        xMat->setDiffuse(QColor(255, 0, 0));
        xAxisEntity->addComponent(xMat);
    }
    
    // Y轴 - 绿色
    {
        Qt3DCore::QEntity* yAxisEntity = new Qt3DCore::QEntity(m_axesEntity);
        Qt3DRender::QGeometry* yGeometry = new Qt3DRender::QGeometry(yAxisEntity);
        
        QByteArray yVertexData;
        yVertexData.resize(2 * 3 * sizeof(float));
        float* yVerts = reinterpret_cast<float*>(yVertexData.data());
        yVerts[0] = 0; yVerts[1] = 0; yVerts[2] = 0;
        yVerts[3] = 0; yVerts[4] = axisLength; yVerts[5] = 0;
        
        Qt3DRender::QBuffer* yBuffer = new Qt3DRender::QBuffer(yGeometry);
        yBuffer->setData(yVertexData);
        
        Qt3DRender::QAttribute* yPosAttr = new Qt3DRender::QAttribute(yGeometry);
        yPosAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        yPosAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        yPosAttr->setVertexSize(3);
        yPosAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        yPosAttr->setBuffer(yBuffer);
        yPosAttr->setByteStride(3 * sizeof(float));
        yPosAttr->setCount(2);
        yGeometry->addAttribute(yPosAttr);
        
        Qt3DRender::QGeometryRenderer* yRenderer = new Qt3DRender::QGeometryRenderer(yAxisEntity);
        yRenderer->setGeometry(yGeometry);
        yRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        yAxisEntity->addComponent(yRenderer);
        
        Qt3DExtras::QPhongMaterial* yMat = new Qt3DExtras::QPhongMaterial(yAxisEntity);
        yMat->setAmbient(QColor(0, 255, 0));
        yMat->setDiffuse(QColor(0, 255, 0));
        yAxisEntity->addComponent(yMat);
    }
    
    // Z轴 - 蓝色
    {
        Qt3DCore::QEntity* zAxisEntity = new Qt3DCore::QEntity(m_axesEntity);
        Qt3DRender::QGeometry* zGeometry = new Qt3DRender::QGeometry(zAxisEntity);
        
        QByteArray zVertexData;
        zVertexData.resize(2 * 3 * sizeof(float));
        float* zVerts = reinterpret_cast<float*>(zVertexData.data());
        zVerts[0] = 0; zVerts[1] = 0; zVerts[2] = 0;
        zVerts[3] = 0; zVerts[4] = 0; zVerts[5] = axisLength;
        
        Qt3DRender::QBuffer* zBuffer = new Qt3DRender::QBuffer(zGeometry);
        zBuffer->setData(zVertexData);
        
        Qt3DRender::QAttribute* zPosAttr = new Qt3DRender::QAttribute(zGeometry);
        zPosAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        zPosAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        zPosAttr->setVertexSize(3);
        zPosAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        zPosAttr->setBuffer(zBuffer);
        zPosAttr->setByteStride(3 * sizeof(float));
        zPosAttr->setCount(2);
        zGeometry->addAttribute(zPosAttr);
        
        Qt3DRender::QGeometryRenderer* zRenderer = new Qt3DRender::QGeometryRenderer(zAxisEntity);
        zRenderer->setGeometry(zGeometry);
        zRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        zAxisEntity->addComponent(zRenderer);
        
        Qt3DExtras::QPhongMaterial* zMat = new Qt3DExtras::QPhongMaterial(zAxisEntity);
        zMat->setAmbient(QColor(0, 0, 255));
        zMat->setDiffuse(QColor(0, 0, 255));
        zAxisEntity->addComponent(zMat);
    }
}

// ==================== RobotEntity ====================

RobotEntity::RobotEntity(Qt3DCore::QEntity* parent)
    : Qt3DCore::QEntity(parent)
{
    // 创建整体变换组件用于缩放
    m_robotTransform = new Qt3DCore::QTransform(this);
    addComponent(m_robotTransform);
    
    m_trajectoryTimer = new QTimer(this);
    connect(m_trajectoryTimer, &QTimer::timeout, this, &RobotEntity::sampleTrajectory);
}

RobotEntity::~RobotEntity()
{
    clear();
}

void RobotEntity::clear()
{
    m_trajectoryTimer->stop();
    
    // 删除所有实体
    for (auto entity : m_linkEntities) {
        entity->deleteLater();
    }
    m_linkEntities.clear();
    
    for (auto entity : m_jointEntities) {
        entity->deleteLater();
    }
    m_jointEntities.clear();
    
    // 清除材质信息
    m_linkMaterials.clear();
    
    m_model.reset();
    m_endEffectorLink.clear();
    
    // 重置缩放
    m_scale = 1.0f;
    if (m_robotTransform) {
        m_robotTransform->setScale(1.0f);
    }
}

bool RobotEntity::loadFromURDF(const QString& urdfFile)
{
    clear();
    
    if (!m_parser.loadFromFile(urdfFile)) {
        m_errorMessage = m_parser.getErrorMessage();
        return false;
    }
    
    m_model = m_parser.getModel();
    
    if (!buildRobotTree()) {
        return false;
    }
    
    // 自动检测末端执行器
    findEndEffectorLink();
    
    // 启动轨迹采样
    if (m_trajectoryEnabled && m_trajectoryEntity) {
        m_trajectoryTimer->start(50); // 50ms采样间隔
    }
    
    emit robotLoaded();
    
    return true;
}

bool RobotEntity::buildRobotTree()
{
    if (!m_model || m_model->rootLink.isEmpty()) {
        m_errorMessage = "Invalid model or no root link";
        return false;
    }
    
    // 从根链接开始构建
    int linkIndex = 0;
    buildLinkEntity(m_model->rootLink, this, linkIndex);
    
    return true;
}

void RobotEntity::buildLinkEntity(const QString& linkName, Qt3DCore::QEntity* parent, int& linkIndex)
{
    auto linkIt = m_model->links.find(linkName);
    if (linkIt == m_model->links.end()) {
        qWarning() << "Link not found:" << linkName;
        return;
    }
    
    auto link = linkIt.value();
    
    // 创建链接实体
    LinkEntity* linkEntity = new LinkEntity(linkName, parent);
    m_linkEntities[linkName] = linkEntity;
    
    // 创建链接的视觉模型
    int currentLinkIndex = linkIndex++;
    Qt3DCore::QEntity* visualEntity = createLinkVisual(link, currentLinkIndex);
    if (visualEntity) {
        visualEntity->setParent(linkEntity);
        linkEntity->setVisualEntity(visualEntity);
    }
    
    // 获取子关节并创建子链接
    auto childJoints = m_model->getChildJoints(linkName);
    for (auto& joint : childJoints) {
        // 创建关节实体
        JointEntity* jointEntity = new JointEntity(joint, linkEntity);
        m_jointEntities[joint->name] = jointEntity;
        
        // 连接信号
        connect(jointEntity, &JointEntity::jointValueChanged,
                this, &RobotEntity::jointValueChanged);
        
        // 递归创建子链接
        buildLinkEntity(joint->childLink, jointEntity, linkIndex);
    }
}

Qt3DCore::QEntity* RobotEntity::createLinkVisual(std::shared_ptr<URDFLink> link, int linkIndex)
{
    if (link->visuals.isEmpty()) {
        return nullptr;
    }
    
    // 创建一个容器实体来容纳所有视觉元素
    Qt3DCore::QEntity* visualContainer = new Qt3DCore::QEntity();
    
    for (const auto& visual : link->visuals) {
        Qt3DCore::QEntity* visualEntity = nullptr;
        
        if (visual.geometry.type == GeometryType::Mesh) {
            // 使用Assimp加载网格文件
            QString meshPath = m_parser.resolveMeshPath(visual.geometry.meshFilename);
            
            QColor color = QColor::fromRgbF(
                visual.material.color[0],
                visual.material.color[1],
                visual.material.color[2],
                visual.material.color[3]
            );
            
            QVector3D scale(
                visual.geometry.meshScale[0],
                visual.geometry.meshScale[1],
                visual.geometry.meshScale[2]
            );
            
            AssimpModelLoader loader;
            visualEntity = loader.loadModel(meshPath, visualContainer, color, scale);
            
            if (!visualEntity) {
                qWarning() << "Failed to load mesh:" << meshPath;
                qWarning() << "Error:" << loader.getErrorMessage();
                continue;
            }
            
            // 收集加载的模型中的材质
            QList<Qt3DExtras::QPhongMaterial*> materials = visualEntity->findChildren<Qt3DExtras::QPhongMaterial*>();
            for (auto* mat : materials) {
                LinkMaterialInfo info;
                info.material = mat;
                info.originalColor = mat->diffuse();
                info.linkIndex = linkIndex;
                m_linkMaterials.append(info);
            }
        } else {
            // 创建基本几何体
            visualEntity = createPrimitiveGeometry(visual.geometry, visual.material);
            if (visualEntity) {
                visualEntity->setParent(visualContainer);
                
                // 收集基本几何体的材质
                Qt3DExtras::QPhongMaterial* mat = visualEntity->findChild<Qt3DExtras::QPhongMaterial*>();
                if (mat) {
                    LinkMaterialInfo info;
                    info.material = mat;
                    info.originalColor = mat->diffuse();
                    info.linkIndex = linkIndex;
                    m_linkMaterials.append(info);
                }
            }
        }
        
        if (visualEntity) {
            // 添加视觉原点变换
            Qt3DCore::QTransform* visualTransform = new Qt3DCore::QTransform(visualEntity);
            visualTransform->setMatrix(visual.origin.toMatrix());
            visualEntity->addComponent(visualTransform);
        }
    }
    
    return visualContainer;
}

Qt3DCore::QEntity* RobotEntity::createPrimitiveGeometry(const Geometry& geom, const Material& mat)
{
    Qt3DCore::QEntity* entity = new Qt3DCore::QEntity();
    
    // 材质
    Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(entity);
    QColor color = QColor::fromRgbF(mat.color[0], mat.color[1], mat.color[2], mat.color[3]);
    material->setDiffuse(color);
    material->setAmbient(color.darker(150));
    entity->addComponent(material);
    
    switch (geom.type) {
    case GeometryType::Box: {
        Qt3DExtras::QCuboidMesh* mesh = new Qt3DExtras::QCuboidMesh(entity);
        mesh->setXExtent(geom.boxSize[0]);
        mesh->setYExtent(geom.boxSize[1]);
        mesh->setZExtent(geom.boxSize[2]);
        entity->addComponent(mesh);
        break;
    }
    case GeometryType::Cylinder: {
        Qt3DExtras::QCylinderMesh* mesh = new Qt3DExtras::QCylinderMesh(entity);
        mesh->setRadius(geom.cylinderRadius);
        mesh->setLength(geom.cylinderLength);
        mesh->setSlices(32);
        entity->addComponent(mesh);
        
        // URDF圆柱体默认沿Z轴，Qt3D默认沿Y轴，需要旋转
        Qt3DCore::QTransform* rotateTransform = new Qt3DCore::QTransform(entity);
        rotateTransform->setRotationX(90);
        entity->addComponent(rotateTransform);
        break;
    }
    case GeometryType::Sphere: {
        Qt3DExtras::QSphereMesh* mesh = new Qt3DExtras::QSphereMesh(entity);
        mesh->setRadius(geom.sphereRadius);
        mesh->setSlices(32);
        mesh->setRings(32);
        entity->addComponent(mesh);
        break;
    }
    default:
        delete entity;
        return nullptr;
    }
    
    return entity;
}

void RobotEntity::findEndEffectorLink()
{
    if (!m_model) return;
    
    // 末端执行器通常是没有子关节的链接
    for (auto& link : m_model->links) {
        auto childJoints = m_model->getChildJoints(link->name);
        if (childJoints.isEmpty() && link->name != m_model->rootLink) {
            m_endEffectorLink = link->name;
            qDebug() << "End effector detected:" << m_endEffectorLink;
            return;
        }
    }
}

QVector<std::shared_ptr<URDFJoint>> RobotEntity::getMovableJoints() const
{
    if (!m_model) return {};
    return m_model->getMovableJoints();
}

double RobotEntity::getJointValue(const QString& jointName) const
{
    auto it = m_jointEntities.find(jointName);
    if (it != m_jointEntities.end()) {
        return it.value()->jointValue();
    }
    return 0;
}

QMap<QString, double> RobotEntity::getAllJointValues() const
{
    QMap<QString, double> values;
    for (auto it = m_jointEntities.begin(); it != m_jointEntities.end(); ++it) {
        if (it.value()->joint()->isMovable()) {
            values[it.key()] = it.value()->jointValue();
        }
    }
    return values;
}

void RobotEntity::setJointValue(const QString& jointName, double value)
{
    auto it = m_jointEntities.find(jointName);
    if (it != m_jointEntities.end()) {
        it.value()->setJointValue(value);
    }
}

void RobotEntity::setJointValues(const QMap<QString, double>& jointValues)
{
    for (auto it = jointValues.begin(); it != jointValues.end(); ++it) {
        setJointValue(it.key(), it.value());
    }
}

void RobotEntity::resetJoints()
{
    for (auto entity : m_jointEntities) {
        entity->setJointValue(0);
    }
}

QVector3D RobotEntity::getEndEffectorPosition(const QString& linkName) const
{
    QString targetLink = linkName.isEmpty() ? m_endEffectorLink : linkName;
    if (targetLink.isEmpty()) return QVector3D();
    
    // 使用computeLinkTransform计算相对于RobotEntity的变换
    QMatrix4x4 linkTransform = computeLinkTransform(targetLink);
    
    // 应用RobotEntity自身的变换（缩放等）
    QMatrix4x4 worldMatrix;
    worldMatrix.setToIdentity();
    if (m_robotTransform) {
        worldMatrix = m_robotTransform->matrix();
    }
    worldMatrix = worldMatrix * linkTransform;
    
    // 返回链接坐标系原点的世界坐标
    return worldMatrix.column(3).toVector3D();
}

QMatrix4x4 RobotEntity::getLinkWorldTransform(const QString& linkName) const
{
    return computeLinkTransform(linkName);
}

QVector3D RobotEntity::getLinkGeometryCenter(const QString& linkName) const
{
    if (!m_model) return QVector3D();
    
    auto linkIt = m_model->links.find(linkName);
    if (linkIt == m_model->links.end() || linkIt.value()->visuals.isEmpty()) {
        return QVector3D();
    }
    
    auto link = linkIt.value();
    
    // 如果只有一个视觉元素，使用其中心
    if (link->visuals.size() == 1) {
        const auto& visual = link->visuals.first();
        QVector3D visualOrigin(visual.origin.xyz[0], 
                               visual.origin.xyz[1], 
                               visual.origin.xyz[2]);
        
        // 根据几何类型计算中心偏移
        QVector3D geometryCenter;
        switch (visual.geometry.type) {
        case GeometryType::Box:
            // 盒子中心就是原点
            geometryCenter = visualOrigin;
            break;
        case GeometryType::Cylinder:
            // 圆柱中心沿Z轴偏移一半长度
            geometryCenter = visualOrigin + QVector3D(0, 0, visual.geometry.cylinderLength * 0.5f);
            break;
        case GeometryType::Sphere:
            // 球体中心就是原点
            geometryCenter = visualOrigin;
            break;
        case GeometryType::Mesh:
            // 网格使用原点（可以后续改进为读取mesh的包围盒）
            geometryCenter = visualOrigin;
            break;
        default:
            geometryCenter = visualOrigin;
            break;
        }
        
        return geometryCenter;
    }
    
    // 多个视觉元素，计算平均中心
    QVector3D centerSum;
    int count = 0;
    for (const auto& visual : link->visuals) {
        QVector3D visualOrigin(visual.origin.xyz[0], 
                               visual.origin.xyz[1], 
                               visual.origin.xyz[2]);
        centerSum += visualOrigin;
        count++;
    }
    
    if (count > 0) {
        return centerSum / count;
    }
    
    return QVector3D();
}

QMatrix4x4 RobotEntity::computeLinkTransform(const QString& linkName) const
{
    QMatrix4x4 transform;
    transform.setToIdentity();
    
    if (!m_model) return transform;
    
    // 从目标链接向上遍历到根链接
    QString currentLink = linkName;
    QVector<QMatrix4x4> transforms;
    
    while (!currentLink.isEmpty() && currentLink != m_model->rootLink) {
        auto parentJoint = m_model->getParentJoint(currentLink);
        if (!parentJoint) break;
        
        // 获取关节变换
        auto jointEntity = m_jointEntities.value(parentJoint->name);
        if (jointEntity) {
            transforms.prepend(jointEntity->transform()->matrix());
        }
        
        currentLink = parentJoint->parentLink;
    }
    
    // 组合所有变换
    for (const auto& t : transforms) {
        transform *= t;
    }
    
    return transform;
}

void RobotEntity::setEndEffectorLink(const QString& linkName)
{
    m_endEffectorLink = linkName;
}

void RobotEntity::addEndEffector(const QString& linkName, TrajectoryEntity* trajectory, const QString& name)
{
    if (linkName.isEmpty() || !trajectory) {
        qWarning() << "Invalid end effector parameters:" << linkName;
        return;
    }
    
    EndEffectorInfo info;
    info.linkName = linkName;
    info.displayName = name.isEmpty() ? linkName : name;
    info.trajectory = trajectory;
    
    m_endEffectors[linkName] = info;
    
    qDebug() << "Added end effector:" << info.displayName << "(" << linkName << ")";
}

void RobotEntity::removeEndEffector(const QString& linkName)
{
    m_endEffectors.remove(linkName);
}

void RobotEntity::clearEndEffectors()
{
    m_endEffectors.clear();
}

QStringList RobotEntity::getEndEffectorLinks() const
{
    return m_endEffectors.keys();
}

void RobotEntity::setTrajectorySampleInterval(int msec)
{
    if (m_trajectoryTimer->isActive()) {
        m_trajectoryTimer->setInterval(msec);
    }
}

void RobotEntity::setTrajectoryEnabled(bool enabled)
{
    m_trajectoryEnabled = enabled;
    
    // 处理单个轨迹（向后兼容）
    if (enabled && m_trajectoryEntity && m_model) {
        m_trajectoryTimer->start(50);
    } else if (!enabled && m_endEffectors.isEmpty()) {
        // 只有在没有多末端执行器时才停止定时器
        m_trajectoryTimer->stop();
    }
    
    // 处理多末端执行器
    if (enabled && !m_endEffectors.isEmpty() && m_model) {
        m_trajectoryTimer->start(50);
    } else if (!enabled && m_endEffectors.isEmpty() && !m_trajectoryEntity) {
        m_trajectoryTimer->stop();
    }
}

void RobotEntity::setJointAxesVisible(bool visible)
{
    m_jointAxesVisible = visible;
    for (auto jointEntity : m_jointEntities) {
        jointEntity->setAxesVisible(visible);
    }
}

void RobotEntity::sampleTrajectory()
{
    // 处理单个轨迹（向后兼容）
    if (m_trajectoryEntity && !m_endEffectorLink.isEmpty()) {
        QVector3D position = getEndEffectorPosition();
        m_trajectoryEntity->addPoint(position);
        emit endEffectorPositionChanged(position);
    }
    
    // 处理多末端执行器轨迹
    for (auto it = m_endEffectors.begin(); it != m_endEffectors.end(); ++it) {
        const EndEffectorInfo& info = it.value();
        if (info.trajectory) {
            QVector3D position = getEndEffectorPosition(info.linkName);
            info.trajectory->addPoint(position);
            // 可以为每个末端执行器发出信号（如果需要）
        }
    }
}

void RobotEntity::setColoredLinksEnabled(bool enabled)
{
    if (m_coloredLinksEnabled == enabled) return;

    if(applyLinkColors(enabled))
    {
        m_coloredLinksEnabled = enabled;
    }
}

bool RobotEntity::applyLinkColors(bool enable)
{
    for (auto& info : m_linkMaterials) {
        if (!info.material) continue;
        
        if (enable) {
            QColor linkColor = getLinkColor(info.linkIndex);
            info.material->setDiffuse(linkColor);
            info.material->setAmbient(linkColor.darker(150));
        } else {
            info.material->setDiffuse(info.originalColor);
            info.material->setAmbient(info.originalColor.darker(150));
        }
    }

    if(m_linkMaterials.length() > 0)
    {
        return true;
    }

    return false;
}

QColor RobotEntity::getLinkColor(int index) const
{
    // 使用HSV色彩空间生成不同颜色，保持饱和度和亮度一致
    // 这样可以生成视觉上均匀分布的颜色
    static const QList<QColor> predefinedColors = {
        QColor(255, 100, 100),  // 红
        QColor(100, 255, 100),  // 绿
        QColor(100, 100, 255),  // 蓝
        QColor(255, 255, 100),  // 黄
        QColor(255, 100, 255),  // 品红
        QColor(100, 255, 255),  // 青
        QColor(255, 180, 100),  // 橙
        QColor(180, 100, 255),  // 紫
        QColor(100, 255, 180),  // 青绿
        QColor(255, 100, 180),  // 粉红
        QColor(180, 255, 100),  // 黄绿
        QColor(100, 180, 255),  // 天蓝
    };
    
    if (index < predefinedColors.size()) {
        return predefinedColors[index];
    }
    
    // 超出预定义颜色后使用HSV生成
    int hue = (index * 67) % 360;  // 使用质数避免相邻颜色太相似
    return QColor::fromHsv(hue, 200, 220);
}

void RobotEntity::getBoundingBox(QVector3D& minPoint, QVector3D& maxPoint) const
{
    minPoint = QVector3D(std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max());
    maxPoint = QVector3D(std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest());
    
    if (!m_model) return;
    
    // 遍历所有链接，收集它们的位置来估算包围盒
    for (auto it = m_linkEntities.constBegin(); it != m_linkEntities.constEnd(); ++it) {
        QString linkName = it.key();
        
        // 获取链接的世界变换
        QMatrix4x4 worldTransform = computeLinkTransform(linkName);
        QVector3D linkPos = worldTransform.column(3).toVector3D();
        
        // 更新包围盒
        minPoint.setX(qMin(minPoint.x(), linkPos.x()));
        minPoint.setY(qMin(minPoint.y(), linkPos.y()));
        minPoint.setZ(qMin(minPoint.z(), linkPos.z()));
        
        maxPoint.setX(qMax(maxPoint.x(), linkPos.x()));
        maxPoint.setY(qMax(maxPoint.y(), linkPos.y()));
        maxPoint.setZ(qMax(maxPoint.z(), linkPos.z()));
        
        // 也考虑URDF中定义的几何体尺寸
        auto linkIt = m_model->links.find(linkName);
        if (linkIt != m_model->links.end()) {
            auto link = linkIt.value();
            for (const auto& visual : link->visuals) {
                QVector3D visualOffset(visual.origin.xyz[0], 
                                       visual.origin.xyz[1], 
                                       visual.origin.xyz[2]);
                QVector3D visualWorldPos = worldTransform.map(visualOffset);
                
                // 根据几何体类型估算尺寸
                float radius = 0.0f;
                switch (visual.geometry.type) {
                case GeometryType::Box:
                    radius = qMax(visual.geometry.boxSize[0], 
                                  qMax(visual.geometry.boxSize[1], 
                                       visual.geometry.boxSize[2])) * 0.5f;
                    break;
                case GeometryType::Cylinder:
                    radius = qMax(visual.geometry.cylinderRadius, 
                                  visual.geometry.cylinderLength * 0.5f);
                    break;
                case GeometryType::Sphere:
                    radius = visual.geometry.sphereRadius;
                    break;
                case GeometryType::Mesh:
                    // 网格文件，使用一个估计值
                    radius = 0.1f;
                    break;
                default:
                    break;
                }
                
                minPoint.setX(qMin(minPoint.x(), visualWorldPos.x() - radius));
                minPoint.setY(qMin(minPoint.y(), visualWorldPos.y() - radius));
                minPoint.setZ(qMin(minPoint.z(), visualWorldPos.z() - radius));
                
                maxPoint.setX(qMax(maxPoint.x(), visualWorldPos.x() + radius));
                maxPoint.setY(qMax(maxPoint.y(), visualWorldPos.y() + radius));
                maxPoint.setZ(qMax(maxPoint.z(), visualWorldPos.z() + radius));
            }
        }
    }
    
    // 如果没有有效数据，返回默认值
    if (minPoint.x() > maxPoint.x()) {
        minPoint = QVector3D(-0.5f, -0.5f, -0.5f);
        maxPoint = QVector3D(0.5f, 0.5f, 0.5f);
    }
}

float RobotEntity::getModelSize() const
{
    QVector3D minPoint, maxPoint;
    getBoundingBox(minPoint, maxPoint);
    
    QVector3D size = maxPoint - minPoint;
    return size.length();
}

void RobotEntity::setScale(float scale)
{
    m_scale = scale;
    if (m_robotTransform) {
        m_robotTransform->setScale(scale);
    }
}

float RobotEntity::getScale() const
{
    return m_scale;
}
